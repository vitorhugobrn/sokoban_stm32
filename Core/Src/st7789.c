#include "st7789.h"
#include "sprites.h"

#ifdef USE_DMA
#include <string.h>
/* Minimum byte count to use DMA (below this, blocking SPI is faster due to
 * DMA setup overhead). */
#define DMA_MIN_SIZE 16
/* Number of display rows to buffer. */
#define HOR_LEN 16
uint16_t disp_buf[ST7789_WIDTH * HOR_LEN];

/* =========================================================================
 * Async DMA engine
 * =========================================================================
 * Two transfer modes driven by HAL_SPI_TxCpltCallback:
 *
 *  STREAM  – large arbitrary buffer split into ≤64 KB DMA chunks.
 *  REPEAT  – same disp_buf sent N times (solid-color fill). No CPU round-trip
 *            between each chunk; next DMA fires from inside the ISR.
 *
 * External API: ST7789_IsDMABusy() / ST7789_WaitDMA()
 * ========================================================================= */
volatile uint8_t st7789_dma_busy = 0;

/* STREAM state */
static uint8_t  *dma_stream_ptr  = NULL;
static uint32_t  dma_stream_rem  = 0;

/* REPEAT state */
static uint32_t  dma_rep_left    = 0;   /* full-buffer repeats still queued   */
static uint32_t  dma_rep_tail    = 0;   /* leftover bytes after last repeat   */

/* -------------------------------------------------------------------------
 * ISR callback — fires after every DMA transfer completes.
 * Chains the next chunk immediately; CS is released only when fully done.
 * ------------------------------------------------------------------------- */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi != &ST7789_SPI_PORT) return;

    if (dma_stream_rem > 0) {
        /* STREAM mode: send next chunk of the arbitrary buffer */
        uint16_t c = (dma_stream_rem > 65535u) ? 65535u : (uint16_t)dma_stream_rem;
        uint8_t *p  = dma_stream_ptr;
        dma_stream_rem -= c;
        dma_stream_ptr += c;
        HAL_SPI_Transmit_DMA(hspi, p, c);
    } else if (dma_rep_left > 0) {
        /* REPEAT mode: re-send disp_buf once more */
        dma_rep_left--;
        HAL_SPI_Transmit_DMA(hspi, (uint8_t *)disp_buf, (uint16_t)sizeof(disp_buf));
    } else if (dma_rep_tail > 0) {
        /* REPEAT mode: send leftover partial buffer */
        uint16_t tail = (uint16_t)dma_rep_tail;
        dma_rep_tail  = 0;
        HAL_SPI_Transmit_DMA(hspi, (uint8_t *)disp_buf, tail);
    } else {
        /* All done */
        st7789_dma_busy = 0;
    }
}

/* Block until any async transfer finishes (used internally + publicly). */
static inline void ST7789_DMAWait(void) { while (st7789_dma_busy) {} }

uint8_t ST7789_IsDMABusy(void) { return st7789_dma_busy; }
void    ST7789_WaitDMA   (void) { ST7789_DMAWait(); }

/* Start an async STREAM transfer. DC must already be set; CS held low. */
static void ST7789_StartStream(uint8_t *data, uint32_t bytes)
{
    ST7789_DMAWait();
    st7789_dma_busy  = 1;
    dma_rep_left     = 0;
    dma_rep_tail     = 0;
    uint16_t first   = (bytes > 65535u) ? 65535u : (uint16_t)bytes;
    dma_stream_rem   = bytes - first;
    dma_stream_ptr   = data  + first;
    HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, data, first);
}

/* Start an async REPEAT fill: send disp_buf for total_bytes worth of pixels.
 * disp_buf must already be filled with the desired color. DC must be set. */
static void ST7789_StartRepeat(uint32_t total_bytes)
{
    ST7789_DMAWait();
    uint32_t buf_sz  = (uint32_t)sizeof(disp_buf);
    uint32_t reps    = total_bytes / buf_sz;
    uint32_t tail    = total_bytes % buf_sz;
    st7789_dma_busy  = 1;
    dma_stream_rem   = 0;
    dma_stream_ptr   = NULL;
    if (reps > 0) {
        dma_rep_left = reps - 1;   /* callback handles reps-1 more after first */
        dma_rep_tail = tail;
        HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, (uint8_t *)disp_buf, (uint16_t)buf_sz);
    } else {
        dma_rep_left = 0;
        dma_rep_tail = 0;
        HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, (uint8_t *)disp_buf, (uint16_t)tail);
    }
}
#endif

/**
 * @brief Write command to ST7789 controller
 * @param cmd -> command to write
 * @return none
 */
static void ST7789_WriteCommand(uint8_t cmd)
{
	ST7789_DC_Clr();
	HAL_SPI_Transmit(&ST7789_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
}

/**
 * @brief Write data to ST7789 controller
 * @param buff -> pointer of data buffer
 * @param buff_size -> size of the data buffer
 * @return none
 */
__attribute__((always_inline)) static void ST7789_WriteData(uint8_t *buff, size_t buff_size)
{
	ST7789_DC_Set();
#ifdef USE_DMA
	if (buff_size >= DMA_MIN_SIZE) {
		/* Use async stream path; block here for backward-compatible API. */
		ST7789_StartStream(buff, (uint32_t)buff_size);
		ST7789_DMAWait();
	} else {
		HAL_SPI_Transmit(&ST7789_SPI_PORT, buff, (uint16_t)buff_size, HAL_MAX_DELAY);
	}
#else
	while (buff_size > 0) {
		uint16_t chunk = buff_size > 65535 ? 65535 : (uint16_t)buff_size;
		HAL_SPI_Transmit(&ST7789_SPI_PORT, buff, chunk, HAL_MAX_DELAY);
		buff      += chunk;
		buff_size -= chunk;
	}
#endif
}
/**
 * @brief Write data to ST7789 controller, simplify for 8bit data.
 * data -> data to write
 * @return none
 */
static void ST7789_WriteSmallData(uint8_t data)
{
	ST7789_DC_Set();
	HAL_SPI_Transmit(&ST7789_SPI_PORT, &data, sizeof(data), HAL_MAX_DELAY);
}

/**
 * @brief Set the rotation direction of the display
 * @param m -> rotation parameter(please refer it in st7789.h)
 * @return none
 */
void ST7789_SetRotation(uint8_t m)
{
	ST7789_WriteCommand(ST7789_MADCTL);	// MADCTL
	switch (m) {
	case 0:
		ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
		break;
	case 1:
		ST7789_WriteSmallData(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	case 2:
		ST7789_WriteSmallData(ST7789_MADCTL_RGB);
		break;
	case 3:
		ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	default:
		break;
	}
}

/**
 * @brief Set address of DisplayWindow
 * @param xi&yi -> coordinates of window
 * @return none
 */
__attribute__((always_inline)) static void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	ST7789_DMAWait();

	uint16_t x_start = x0 + X_SHIFT, x_end = x1 + X_SHIFT;
	uint16_t y_start = y0 + Y_SHIFT, y_end = y1 + Y_SHIFT;

	uint8_t data[4];
	uint8_t cmd;

	/* Column Address Set */
	cmd = ST7789_CASET;
	ST7789_DC_Clr();
	HAL_SPI_Transmit(&ST7789_SPI_PORT, &cmd, 1, HAL_MAX_DELAY);
	data[0] = x_start >> 8; data[1] = x_start & 0xFF;
	data[2] = x_end   >> 8; data[3] = x_end   & 0xFF;
	ST7789_DC_Set();
	HAL_SPI_Transmit(&ST7789_SPI_PORT, data, 4, HAL_MAX_DELAY);

	/* Row Address Set */
	cmd = ST7789_RASET;
	ST7789_DC_Clr();
	HAL_SPI_Transmit(&ST7789_SPI_PORT, &cmd, 1, HAL_MAX_DELAY);
	data[0] = y_start >> 8; data[1] = y_start & 0xFF;
	data[2] = y_end   >> 8; data[3] = y_end   & 0xFF;
	ST7789_DC_Set();
	HAL_SPI_Transmit(&ST7789_SPI_PORT, data, 4, HAL_MAX_DELAY);

	/* Write to RAM – leave DC high so the caller's first pixel byte goes straight in */
	cmd = ST7789_RAMWR;
	ST7789_DC_Clr();
	HAL_SPI_Transmit(&ST7789_SPI_PORT, &cmd, 1, HAL_MAX_DELAY);
	ST7789_DC_Set();
}

/**
 * @brief Initialize ST7789 controller
 * @param none
 * @return none
 */
void ST7789_Init(void)
{
	#ifdef USE_DMA
		memset(disp_buf, 0, sizeof(disp_buf));
	#endif
	HAL_Delay(10);
    ST7789_RST_Clr();
    HAL_Delay(10);
    ST7789_RST_Set();
    HAL_Delay(20);

    ST7789_WriteCommand(ST7789_COLMOD);		//	Set color mode
    ST7789_WriteSmallData(ST7789_COLOR_MODE_16bit);
  	ST7789_WriteCommand(0xB2);				//	Porch control
	{
		uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
		ST7789_WriteData(data, sizeof(data));
	}
	ST7789_SetRotation(ST7789_ROTATION);	//	MADCTL (Display Rotation)

	/* Internal LCD Voltage generator settings */
    ST7789_WriteCommand(0XB7);				//	Gate Control
    ST7789_WriteSmallData(0x35);			//	Default value
    ST7789_WriteCommand(0xBB);				//	VCOM setting
    ST7789_WriteSmallData(0x19);			//	0.725v (default 0.75v for 0x20)
    ST7789_WriteCommand(0xC0);				//	LCMCTRL
    ST7789_WriteSmallData (0x2C);			//	Default value
    ST7789_WriteCommand (0xC2);				//	VDV and VRH command Enable
    ST7789_WriteSmallData (0x01);			//	Default value
    ST7789_WriteCommand (0xC3);				//	VRH set
    ST7789_WriteSmallData (0x12);			//	+-4.45v (defalut +-4.1v for 0x0B)
    ST7789_WriteCommand (0xC4);				//	VDV set
    ST7789_WriteSmallData (0x20);			//	Default value
    ST7789_WriteCommand (0xC6);				//	Frame rate control in normal mode
    ST7789_WriteSmallData (0x0F);			//	Default value (60HZ)
    ST7789_WriteCommand (0xD0);				//	Power control
    ST7789_WriteSmallData (0xA4);			//	Default value
    ST7789_WriteSmallData (0xA1);			//	Default value
	/**************** Division line ****************/

	ST7789_WriteCommand(0xE0);
	{
		uint8_t data[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
		ST7789_WriteData(data, sizeof(data));
	}

    ST7789_WriteCommand(0xE1);
	{
		uint8_t data[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
		ST7789_WriteData(data, sizeof(data));
	}
    ST7789_WriteCommand (ST7789_INVON);		//	Inversion ON
	ST7789_WriteCommand (ST7789_SLPOUT);	//	Out of sleep mode
  	ST7789_WriteCommand (ST7789_NORON);		//	Normal Display on
  	ST7789_WriteCommand (ST7789_DISPON);	//	Main screen turned on

	HAL_Delay(50);
	ST7789_Fill_Color(BLACK);				//	Fill with Black.
}

/**
 * @brief Fill the DisplayWindow with single color
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_Fill_Color(uint16_t color)
{
	ST7789_SetAddressWindow(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
#ifdef USE_DMA
	/* Pre-fill disp_buf once; StartRepeat re-fires it from the ISR for every
	 * subsequent chunk — zero CPU round-trip between chunks. */
	for (uint32_t i = 0; i < (uint32_t)ST7789_WIDTH * HOR_LEN; i++) disp_buf[i] = color;
	ST7789_DC_Set();
	ST7789_StartRepeat((uint32_t)ST7789_WIDTH * ST7789_HEIGHT * 2u);
	ST7789_DMAWait(); /* blocking — remove this line to make it non-blocking */
#else
	for (uint16_t i = 0; i < ST7789_WIDTH; i++)
		for (uint16_t j = 0; j < ST7789_HEIGHT; j++) {
			uint8_t data[] = {color >> 8, color & 0xFF};
			ST7789_WriteData(data, sizeof(data));
		}
#endif
}

/**
 * @brief Draw a Pixel
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x < 0) || (x >= ST7789_WIDTH) ||
		 (y < 0) || (y >= ST7789_HEIGHT))	return;

	ST7789_SetAddressWindow(x, y, x, y);
	uint8_t data[] = {color >> 8, color & 0xFF};
	ST7789_WriteData(data, sizeof(data));
}

/**
 * @brief Fill an Area with single color
 * @param xSta&ySta -> coordinate of the start point
 * @param xEnd&yEnd -> coordinate of the end point
 * @param color -> color to Fill with
 * @return none
 */
// void ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
// {
// 	if ((xEnd < 0) || (xEnd >= ST7789_WIDTH) ||
// 		 (yEnd < 0) || (yEnd >= ST7789_HEIGHT))	return;
// 	ST7789_Select();
// 	uint16_t i, j;
// 	ST7789_SetAddressWindow(xSta, ySta, xEnd, yEnd);
// 	for (i = ySta; i <= yEnd; i++)
// 		for (j = xSta; j <= xEnd; j++) {
// 			uint8_t data[] = {color >> 8, color & 0xFF};
// 			ST7789_WriteData(data, sizeof(data));
// 		}
// 	ST7789_UnSelect();
// }

void ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
    uint32_t total = (uint32_t)(xEnd - xSta + 1) * (yEnd - ySta + 1);
#ifdef USE_DMA
    uint32_t buf_px = (uint32_t)ST7789_WIDTH * HOR_LEN;
    uint32_t fill   = (total < buf_px) ? total : buf_px;
    for (uint32_t i = 0; i < fill; i++) disp_buf[i] = color;
    ST7789_SetAddressWindow(xSta, ySta, xEnd, yEnd);
    ST7789_DC_Set();
    ST7789_StartRepeat(total * 2u);
    ST7789_DMAWait();
#else
    for (uint32_t i = 0; i < (uint32_t)ST7789_WIDTH * HOR_LEN; i++) disp_buf[i] = color;
    ST7789_SetAddressWindow(xSta, ySta, xEnd, yEnd);
    while (total > 0) {
        uint32_t n = total > (uint32_t)ST7789_WIDTH * HOR_LEN ? (uint32_t)ST7789_WIDTH * HOR_LEN : total;
        ST7789_WriteData((uint8_t*)disp_buf, n * 2);
        total -= n;
    }
#endif
}

/**
 * @brief Draw a big Pixel at a point
 * @param x&y -> coordinate of the point
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel_4px(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x <= 0) || (x > ST7789_WIDTH) ||
		 (y <= 0) || (y > ST7789_HEIGHT))	return;
	ST7789_Fill(x - 1, y - 1, x + 1, y + 1, color);
}

/**
 * @brief Draw a line with single color
 * @param x1&y1 -> coordinate of the start point
 * @param x2&y2 -> coordinate of the end point
 * @param color -> color of the line to Draw
 * @return none
 */
void ST7789_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
        uint16_t color) {

	/* --- Fast paths for axis-aligned lines --------------------------------
	 * Horizontal/vertical lines go through ST7789_Fill which issues a single
	 * SetAddressWindow + bulk DMA fill instead of one DrawPixel (= one
	 * SetAddressWindow) per pixel.  DrawFilledCircle and DrawTriangle use
	 * DrawLine heavily for these cases, so they benefit automatically. */
	if (y0 == y1) {
		if (x0 > x1) { uint16_t t = x0; x0 = x1; x1 = t; }
		ST7789_Fill(x0, y0, x1, y0, color);
		return;
	}
	if (x0 == x1) {
		if (y0 > y1) { uint16_t t = y0; y0 = y1; y1 = t; }
		ST7789_Fill(x0, y0, x0, y1, color);
		return;
	}

	/* --- Bresenham for true diagonals ------------------------------------- */
	uint16_t swap;
    uint16_t steep = ABS(y1 - y0) > ABS(x1 - x0);
    if (steep) {
		swap = x0; x0 = y0; y0 = swap;
		swap = x1; x1 = y1; y1 = swap;
    }
    if (x0 > x1) {
		swap = x0; x0 = x1; x1 = swap;
		swap = y0; y0 = y1; y1 = swap;
    }

    int16_t dx = x1 - x0;
    int16_t dy = ABS(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; x0++) {
        if (steep)
            ST7789_DrawPixel(y0, x0, color);
        else
            ST7789_DrawPixel(x0, y0, color);
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

/**
 * @brief Draw a Rectangle with single color
 * @param xi&yi -> 2 coordinates of 2 top points.
 * @param color -> color of the Rectangle line
 * @return none
 */
void ST7789_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	ST7789_DrawLine(x1, y1, x2, y1, color);
	ST7789_DrawLine(x1, y1, x1, y2, color);
	ST7789_DrawLine(x1, y2, x2, y2, color);
	ST7789_DrawLine(x2, y1, x2, y2, color);
}

/**
 * @brief Draw a circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle line
 * @return  none
 */
void ST7789_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	ST7789_DrawPixel(x0, y0 + r, color);
	ST7789_DrawPixel(x0, y0 - r, color);
	ST7789_DrawPixel(x0 + r, y0, color);
	ST7789_DrawPixel(x0 - r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		ST7789_DrawPixel(x0 + x, y0 + y, color);
		ST7789_DrawPixel(x0 - x, y0 + y, color);
		ST7789_DrawPixel(x0 + x, y0 - y, color);
		ST7789_DrawPixel(x0 - x, y0 - y, color);

		ST7789_DrawPixel(x0 + y, y0 + x, color);
		ST7789_DrawPixel(x0 - y, y0 + x, color);
		ST7789_DrawPixel(x0 + y, y0 - x, color);
		ST7789_DrawPixel(x0 - y, y0 - x, color);
	}
}

/**
 * @brief Draw an Image on the screen
 * @param x&y -> start point of the Image
 * @param w&h -> width & height of the Image to Draw
 * @param data -> pointer of the Image array
 * @return none
 */
void ST7789_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
	if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT))
		return;
	if ((x + w - 1) >= ST7789_WIDTH)
		return;
	if ((y + h - 1) >= ST7789_HEIGHT)
		return;

	ST7789_SetAddressWindow(x, y, x + w - 1, y + h - 1);
#ifdef USE_DMA
	ST7789_DC_Set();
	ST7789_StartStream((uint8_t *)data, (uint32_t)w * h * 2u);
	ST7789_DMAWait(); /* remove to make async: let Sokoban do work during DMA */
#else
	ST7789_WriteData((uint8_t *)data, (uint32_t)w * h * 2u);
#endif
}

/**
 * @brief Draw an image asynchronously — returns immediately while DMA runs.
 *        Call ST7789_WaitDMA() before any subsequent draw call.
 * @param x,y   top-left corner
 * @param w,h   image dimensions
 * @param data  RGB565 pixel array (must remain valid until ST7789_IsDMABusy()==0)
 */
void ST7789_DrawImage_Async(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                             const uint16_t *data)
{
	if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT)) return;
	if ((x + w - 1) >= ST7789_WIDTH)  return;
	if ((y + h - 1) >= ST7789_HEIGHT) return;
#ifdef USE_DMA
	ST7789_SetAddressWindow(x, y, x + w - 1, y + h - 1);
	ST7789_DC_Set();
	ST7789_StartStream((uint8_t *)data, (uint32_t)w * h * 2u);
	/* Returns immediately — DMA running in background */
#else
	ST7789_DrawImage(x, y, w, h, data); /* fallback to blocking */
#endif
}

/**
 * @brief Draw an image with alpha testing (color-key transparency).
 *        Any pixel whose value equals `color_key` is skipped (not drawn),
 *        so the existing background shows through — cheap transparency for
 *        RGB565 sprites that have no native alpha channel.
 *
 *        Use a colour that never appears naturally in the sprite art, e.g.
 *        magenta (0xF81F) or pure green (0x07E0).
 *
 * @param x,y        top-left corner on screen
 * @param w,h        image dimensions in pixels
 * @param data       RGB565 pixel array, row-major, w*h entries
 * @param color_key  RGB565 value treated as fully transparent
 */
void ST7789_DrawImage_Alpha(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                             const uint16_t *data, uint16_t color_key) {
    if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT)) return;
    if ((x + w - 1) >= ST7789_WIDTH)  return;
    if ((y + h - 1) >= ST7789_HEIGHT) return;

    for (uint16_t row = 0; row < h; row++) {
        const uint16_t *src = &data[row * w];
        uint16_t col = 0;

        while (col < w) {
            /* Skip transparent pixels */
            if (src[col] == color_key) { col++; continue; }

            /* Collect the opaque run into disp_buf */
            uint16_t run_start = col;
            uint16_t run_len   = 0;

            while (col < w && src[col] != color_key) {
                /* Sprites are already byte-swapped — copy directly */
                disp_buf[run_len++] = src[col++];
            }

            /* One SetAddressWindow + one SPI burst for the whole run */
            ST7789_SetAddressWindow(x + run_start, y + row,
                                    x + run_start + run_len - 1, y + row);
            ST7789_DC_Set();
		#ifdef USE_DMA
            ST7789_StartStream((uint8_t *)disp_buf, (uint32_t)run_len * 2u);
            /* Don't wait — let DMA run while we scan for the next run */
		#else
            HAL_SPI_Transmit(&ST7789_SPI_PORT, (uint8_t *)disp_buf,
                             (uint16_t)(run_len * 2u), HAL_MAX_DELAY);
		#endif
        }
    }

    ST7789_DMAWait(); /* ensure last transfer completes before returning */
}


/**
 * @brief Invert Fullscreen color
 * @param invert -> Whether to invert
 * @return none
 */
void ST7789_InvertColors(uint8_t invert)
{
	ST7789_WriteCommand(invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
}

/**
 * @brief Write a char
 * @param  x&y -> cursor of the start point.
 * @param ch -> char to write
 * @param font -> fontstyle of the string
 * @param color -> color of the char
 * @param bgcolor -> background color of the char
 * @return  none
 */
void ST7789_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor)
{
	ST7789_SetAddressWindow(x, y, x + font.width - 1, y + font.height - 1);

#ifdef USE_DMA
	/* Render the entire glyph into disp_buf pixel-by-pixel, then blast it
	 * to the display in a single DMA transfer.  The largest font (16x26)
	 * is 416 pixels = 832 bytes, well within the disp_buf size. */
	uint32_t total = (uint32_t)font.width * font.height;
	for (uint32_t i = 0; i < font.height; i++) {
		uint32_t b = font.data[(ch - 32) * font.height + i];
		for (uint32_t j = 0; j < font.width; j++) {
			/* Store MSB first to match the byte order the display expects. */
			uint16_t px = ((b << j) & 0x8000) ? color : bgcolor;
			disp_buf[i * font.width + j] = (px >> 8) | (px << 8);
		}
	}
	ST7789_DC_Set();
	/* Send as one DMA burst (or blocking for very small fonts) */
	uint32_t bytes = total * 2;
	if (bytes >= DMA_MIN_SIZE) {
		HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, (uint8_t *)disp_buf, (uint16_t)bytes);
		while (ST7789_SPI_PORT.hdmatx->State != HAL_DMA_STATE_READY) {}
	} else {
		HAL_SPI_Transmit(&ST7789_SPI_PORT, (uint8_t *)disp_buf, (uint16_t)bytes, HAL_MAX_DELAY);
	}
#else
	/* Non-DMA fallback: send two bytes per pixel */
	for (uint32_t i = 0; i < font.height; i++) {
		uint32_t b = font.data[(ch - 32) * font.height + i];
		for (uint32_t j = 0; j < font.width; j++) {
			uint16_t px = ((b << j) & 0x8000) ? color : bgcolor;
			uint8_t data[2] = {px >> 8, px & 0xFF};
			ST7789_WriteData(data, sizeof(data));
		}
	}
#endif
}

/**
 * @brief Write a string
 * @param  x&y -> cursor of the start point.
 * @param str -> string to write
 * @param font -> fontstyle of the string
 * @param color -> color of the string
 * @param bgcolor -> background color of the string
 * @return  none
 */
void ST7789_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor)
{
	while (*str) {
		if (x + font.width >= ST7789_WIDTH) {
			x = 0;
			y += font.height;
			if (y + font.height >= ST7789_HEIGHT) {
				break;
			}

			if (*str == ' ') {
				// skip spaces in the beginning of the new line
				str++;
				continue;
			}
		}
		ST7789_WriteChar(x, y, *str, font, color, bgcolor);
		x += font.width;
		str++;
	}
}

/**
 * @brief Draw a filled Rectangle with single color
 * @param  x&y -> coordinates of the starting point
 * @param w&h -> width & height of the Rectangle
 * @param color -> color of the Rectangle
 * @return  none
 */
void ST7789_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;

	/* Clamp to screen edge */
	if ((x + w) > ST7789_WIDTH)  w = ST7789_WIDTH  - x;
	if ((y + h) > ST7789_HEIGHT) h = ST7789_HEIGHT - y;
	if (w == 0 || h == 0) return;

	/* One SetAddressWindow covers the entire rectangle; ST7789_Fill streams
	 * the pixel data in disp_buf-sized DMA chunks.  Previously this called
	 * ST7789_Fill once per row, paying the SetAddressWindow SPI overhead
	 * H times instead of once. */
	ST7789_Fill(x, y, x + w - 1, y + h - 1, color);
}

/**
 * @brief Draw a Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the lines
 * @return  none
 */
void ST7789_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
	/* Draw lines */
	ST7789_DrawLine(x1, y1, x2, y2, color);
	ST7789_DrawLine(x2, y2, x3, y3, color);
	ST7789_DrawLine(x3, y3, x1, y1, color);
}

/**
 * @brief Draw a filled Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the triangle
 * @return  none
 */
void ST7789_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
			yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
			curpixel = 0;

	deltax = ABS(x2 - x1);
	deltay = ABS(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1) {
		xinc1 = 1;
		xinc2 = 1;
	}
	else {
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1) {
		yinc1 = 1;
		yinc2 = 1;
	}
	else {
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay) {
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	}
	else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++) {
		ST7789_DrawLine(x, y, x3, y3, color);

		num += numadd;
		if (num >= den) {
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}
}

/**
 * @brief Draw a Filled circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle
 * @return  none
 */
void ST7789_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	ST7789_DrawPixel(x0, y0 + r, color);
	ST7789_DrawPixel(x0, y0 - r, color);
	ST7789_DrawPixel(x0 + r, y0, color);
	ST7789_DrawPixel(x0 - r, y0, color);
	ST7789_DrawLine(x0 - r, y0, x0 + r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		ST7789_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
		ST7789_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, color);

		ST7789_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, color);
		ST7789_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, color);
	}
}


/**
 * @brief Open/Close tearing effect line
 * @param tear -> Whether to tear
 * @return none
 */
void ST7789_TearEffect(uint8_t tear) {
	ST7789_WriteCommand(tear ? 0x35 /* TEON */ : 0x34 /* TEOFF */);
}


/**
 * @brief A Simple test function for ST7789
 * @param  none
 * @return  none
 */
void ST7789_Test(void)
{
	ST7789_Fill_Color(WHITE);
	HAL_Delay(1000);
	ST7789_WriteString(10, 20, "Speed Test", Font_11x18, RED, WHITE);
	HAL_Delay(1000);
	ST7789_Fill_Color(CYAN);
    HAL_Delay(500);
	ST7789_Fill_Color(RED);
    HAL_Delay(500);
	ST7789_Fill_Color(BLUE);
    HAL_Delay(500);
	ST7789_Fill_Color(GREEN);
    HAL_Delay(500);
	ST7789_Fill_Color(YELLOW);
    HAL_Delay(500);
	ST7789_Fill_Color(BROWN);
    HAL_Delay(500);
	ST7789_Fill_Color(DARKBLUE);
    HAL_Delay(500);
	ST7789_Fill_Color(MAGENTA);
    HAL_Delay(500);
	ST7789_Fill_Color(LIGHTGREEN);
    HAL_Delay(500);
	ST7789_Fill_Color(LGRAY);
    HAL_Delay(500);
	ST7789_Fill_Color(LBBLUE);
    HAL_Delay(500);
	ST7789_Fill_Color(WHITE);
	HAL_Delay(500);

	ST7789_WriteString(10, 10, "Font test.", Font_16x26, GBLUE, WHITE);
	ST7789_WriteString(10, 50, "Hello Steve!", Font_7x10, RED, WHITE);
	ST7789_WriteString(10, 75, "Hello Steve!", Font_11x18, YELLOW, WHITE);
	ST7789_WriteString(10, 100, "Hello Steve!", Font_16x26, MAGENTA, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Rect./Line.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawRectangle(30, 30, 100, 100, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Filled Rect.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawFilledRectangle(30, 30, 50, 50, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Circle.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawCircle(60, 60, 25, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Filled Cir.", Font_11x18, YELLOW, BLACK);
	ST7789_DrawFilledCircle(60, 60, 25, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Triangle", Font_11x18, YELLOW, BLACK);
	ST7789_DrawTriangle(30, 30, 30, 70, 60, 40, WHITE);
	HAL_Delay(1000);

	ST7789_Fill_Color(RED);
	ST7789_WriteString(10, 10, "Filled Tri", Font_11x18, YELLOW, BLACK);
	ST7789_DrawFilledTriangle(30, 30, 30, 70, 60, 40, WHITE);
	HAL_Delay(1000);

	//	If FLASH cannot storage anymore datas, please delete codes below.
	ST7789_Fill_Color(0x2200);

	ST7789_DrawImage(96, 48, 16, 16, (uint16_t *)steel_top);
	ST7789_DrawImage(112, 48, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(128, 48, 16, 16, (uint16_t *)steel_top);

	ST7789_DrawImage(48, 64, 16, 16, (uint16_t *)steel_top);
	ST7789_DrawImage(64, 64, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(80, 64, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(96, 64, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(128, 64, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(144, 64, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(160, 64, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(176, 64, 16, 16, (uint16_t *)steel_top);

	ST7789_DrawImage(32, 80, 16, 16, (uint16_t *)steel_top);
	ST7789_DrawImage(48, 80, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(176, 80, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(192, 80, 16, 16, (uint16_t *)steel_top);

	ST7789_DrawImage(32, 96, 16, 16, (uint16_t *)steel_top);
	ST7789_DrawImage(192, 96, 16, 16, (uint16_t *)steel_top);

	ST7789_DrawImage(32, 112, 16, 16, (uint16_t *)steel_top);
	ST7789_DrawImage(192, 112, 16, 16, (uint16_t *)steel_top);

	ST7789_DrawImage(32, 128, 16, 16, (uint16_t *)steel_top);
	ST7789_DrawImage(192, 128, 16, 16, (uint16_t *)steel_top);

	ST7789_DrawImage(32, 144, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(48, 144, 16, 16, (uint16_t *)steel_top);

	ST7789_DrawImage(176, 144, 16, 16, (uint16_t *)steel_top);
	ST7789_DrawImage(192, 144, 16, 16, (uint16_t *)steel_side);

	ST7789_DrawImage(48, 160, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(64, 160, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(80, 160, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(96, 160, 16, 16, (uint16_t *)steel_top);
	ST7789_DrawImage(128, 160, 16, 16, (uint16_t *)steel_top);
	ST7789_DrawImage(144, 160, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(160, 160, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(176, 160, 16, 16, (uint16_t *)steel_top);

	ST7789_DrawImage(96, 176, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(112, 176, 16, 16, (uint16_t *)steel_side);
	ST7789_DrawImage(128, 176, 16, 16, (uint16_t *)steel_side);

	ST7789_DrawImage(80, 96, 16, 16, (uint16_t *)box_var1);
	ST7789_DrawImage(128, 96, 16, 16, (uint16_t *)box_var1);

	ST7789_DrawImage(96, 112, 16, 16, (uint16_t *)box_var1);

	ST7789_DrawImage(80, 128, 16, 16, (uint16_t *)box_var1);
	ST7789_DrawImage(112, 128, 16, 16, (uint16_t *)box_var1);

	HAL_Delay(3000);
}
