/**
 * @file renderer.c
 * @brief Rendering pipeline for the Sokoban game.
 *
 * Provides functions that composite tile backgrounds, box sprites, and the
 * player sprite into pixel-strip buffers and push them to the ST7789 display
 * via a single DMA burst per draw call, minimising SPI overhead.
 *
 * Transparency key: pixels with value @c 0xFFFF (magenta) are treated as
 * transparent when blending sprites over background tiles.
 */

#include "renderer.h"
#include "sprites.h"
#include "st7789.h"

#include <string.h>

/* -----------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------- */

/**
 * @brief Simple integer hash function.
 *
 * Used to generate a pseudo-random but deterministic value from a tile
 * coordinate pair and a seed, producing spatially varied floor variants.
 *
 * @param x    X coordinate input.
 * @param y    Y coordinate input.
 * @param seed Additional seed value.
 * @return 32-bit hash result.
 */
static uint32_t hash(uint32_t x, uint32_t y, uint32_t seed) {
    uint32_t h = seed;
    h ^= x * 374761393;
    h ^= y * 668265263;
    h = (h ^ (h >> 13)) * 1274126177;
    return h;
}

/**
 * @brief Selects a floor tile variant index (0–6) for a given pixel position.
 *
 * @param x    Pixel column of the tile's top-left corner.
 * @param y    Pixel row of the tile's top-left corner.
 * @param seed Seed forwarded to hash(); use a constant per-level value for
 *             stable floor appearance.
 * @return Variant index in the range [0, 6].
 */
static uint8_t GetFloorVariant(uint32_t x, uint32_t y, uint32_t seed) {
    return hash(x, y, seed) % 7;
}

/**
 * @brief Returns the background sprite for the tile that contains pixel (@p px, @p py).
 *
 * Converts the pixel coordinate to a tile index, then returns the matching
 * sprite pointer.  Floor tiles use GetFloorVariant() for variety.
 *
 * @param tiles The 15×15 tile map (row-major).
 * @param px    Pixel column (must be aligned to a 16-pixel tile boundary).
 * @param py    Pixel row (must be aligned to a 16-pixel tile boundary).
 * @return Pointer to the 16×16 RGB565 sprite, or @c NULL for VOID.
 */
static const uint16_t *GetTileSprite(const Level tiles[15][15], uint16_t px, uint16_t py) {
    uint8_t col = px / 16;
    uint8_t row = py / 16;

    switch (tiles[row][col]) {
        case FLOOR: {
            uint8_t sel = GetFloorVariant(px, py, 1);
            return floorTiles[sel];
        }
        case BOX_SPOT:  return box_spot;
        case WALL_SIDE: return steel_top;
        case WALL_TOP:  return steel_side;
        default:        return NULL;
    }
}

/**
 * @brief Blends the box sprite into a horizontal line buffer at column @p bufX.
 *
 * Iterates the box sprite row-by-row, skipping pixels outside the buffer
 * bounds and writing non-transparent pixels (value ≠ @c 0xFFFF) into the
 * 16-pixel-tall section of @p lineBuf.
 *
 * @param lineBuf  Flat pixel buffer with layout [row * bufWidth + col].
 * @param bufX     Left edge of the box in buffer-local column coordinates.
 * @param bufWidth Width of @p lineBuf in pixels.
 */
static void BlendBoxAt(uint16_t *lineBuf, int16_t bufX, uint8_t bufWidth) {
    const uint16_t *boxSpr = (const uint16_t *)box_var1;
    for (uint8_t row = 0; row < 16; row++) {
        for (uint8_t col = 0; col < 16; col++) {
            int16_t bc = bufX + col;
            if (bc < 0 || bc >= bufWidth) continue;
            uint16_t px = boxSpr[row * 16 + col];
            if (px != 0xFFFF)
                lineBuf[row * bufWidth + bc] = px;
        }
    }
}

/**
 * @brief Blends the box sprite into a vertical line buffer at row @p bufY.
 *
 * Iterates the box sprite row-by-row with bounds clamping, writing
 * non-transparent pixels into the 16-pixel-wide section of @p lineBuf.
 *
 * @param lineBuf   Flat pixel buffer with layout [row * 16 + col].
 * @param bufY      Top edge of the box in buffer-local row coordinates.
 * @param bufHeight Height of @p lineBuf in pixels.
 */
static void BlendBoxAtV(uint16_t *lineBuf, int16_t bufY, uint8_t bufHeight) {
    const uint16_t *boxSpr = (const uint16_t *)box_var1;
    for (uint8_t row = 0; row < 16; row++) {
        int16_t br = bufY + row;
        if (br < 0 || br >= bufHeight) continue;
        for (uint8_t col = 0; col < 16; col++) {
            uint16_t px = boxSpr[row * 16 + col];
            if (px != 0xFFFF)
                lineBuf[br * 16 + col] = px;
        }
    }
}

/* -----------------------------------------------------------------------
 * Renderer_DrawLevel
 * --------------------------------------------------------------------- */

/**
 * @brief Draws the entire level tile map to the display.
 *
 * Iterates every cell of the 15×15 grid.  VOID tiles are skipped.  For
 * BOX_SPOT tiles a randomised floor tile is drawn first, then the box-spot
 * overlay is alpha-blended on top.
 *
 * @param tiles 15×15 tile map (row-major: [row][col]).
 */
void Renderer_DrawLevel(const Level tiles[15][15]) {
    for (uint8_t i = 0; i < 15; i++) {
        for (uint8_t j = 0; j < 15; j++) {
            switch (tiles[i][j]) {
                case VOID:
                    break;
                case WALL_SIDE:
                    ST7789_DrawImage(16 * j, 16 * i, 16, 16, steel_top);
                    break;
                case WALL_TOP:
                    ST7789_DrawImage(16 * j, 16 * i, 16, 16, steel_side);
                    break;
                case FLOOR: {
                    uint16_t x = 16 * j;
                    uint16_t y = 16 * i;
                    uint8_t sel = GetFloorVariant(x, y, 1);
                    ST7789_DrawImage(x, y, 16, 16, floorTiles[sel]);
                    break;
                }
                case BOX_SPOT: {
                    uint16_t x = 16 * j;
                    uint16_t y = 16 * i;
                    uint8_t sel = GetFloorVariant(x, y, 1);
                    ST7789_DrawImage(x, y, 16, 16, floorTiles[sel]);
                    ST7789_DrawImage_Alpha(x, y, 16, 16, box_spot[0], 0xFFFF);
                    break;
                }
            }
        }
    }
}

/* -----------------------------------------------------------------------
 * Renderer_DrawBoxes — composite each resting box over its floor tile
 * --------------------------------------------------------------------- */

/**
 * @brief Draws all active boxes composited over their floor backgrounds.
 *
 * For each active box, copies the floor/spot sprite into a 16×16 scratch
 * buffer, blends the box sprite on top, and blits the result.
 *
 * @param tiles    15×15 tile map used to determine floor backgrounds.
 * @param boxes    Array of boxes to render.
 * @param boxCount Number of entries in @p boxes.
 */
void Renderer_DrawBoxes(const Level tiles[15][15], const Box boxes[], uint8_t boxCount) {
    static uint16_t tmp[16 * 16];
    const uint16_t *boxSpr = (const uint16_t *)box_var1;

    for (uint8_t i = 0; i < boxCount; i++) {
        if (!boxes[i].active) continue;

        uint16_t px = boxes[i].tileX * 16;
        uint16_t py = boxes[i].tileY * 16;

        /* Floor background */
        const uint16_t *bg = GetTileSprite(tiles, px, py);
        if (bg) memcpy(tmp, bg, 16 * 16 * sizeof(uint16_t));
        else    memset(tmp, 0,  16 * 16 * sizeof(uint16_t));

        /* Box sprite on top */
        for (uint8_t row = 0; row < 16; row++)
            for (uint8_t col = 0; col < 16; col++) {
                uint16_t p2 = boxSpr[row * 16 + col];
                if (p2 != 0xFFFF) tmp[row * 16 + col] = p2;
            }

        ST7789_DrawImage(px, py, 16, 16, tmp);
    }
}

/* -----------------------------------------------------------------------
 * Renderer_DrawPlayerStrip
 * Composites: floor tiles + any resting boxes in the strip + player sprite.
 * --------------------------------------------------------------------- */

/**
 * @brief Renders a 3-tile animation strip centred on the player.
 *
 * The strip orientation (horizontal vs. vertical) is determined by whether
 * the player is moving along the X or Y axis:
 *
 * - **Horizontal**: 48×16 px strip — tiles [tileX-1, tileX, tileX+1] at tileY.
 * - **Vertical**:   16×48 px strip — tiles at tileX for rows [tileY-1, tileY, tileY+1].
 *
 * For each orientation:
 *  1. The floor background is copied tile-by-tile into a static frame buffer.
 *  2. Resting boxes whose tile position falls within the strip are blended in.
 *  3. The player sprite is blended at its interpolated sub-pixel position.
 *  4. The buffer is transferred to the display in a single call.
 *
 * @param tiles    15×15 tile map.
 * @param p        Pointer to the Player.
 * @param boxes    Full box array for compositing resting boxes.
 * @param boxCount Number of entries in @p boxes.
 */
void Renderer_DrawPlayerStrip(const Level tiles[15][15], const Player *p,
                               const Box boxes[], uint8_t boxCount) {
    bool isVertical = (p->tileY != p->targetTileY);

    if (isVertical) {
        static uint16_t lineBuf[16 * 48];

        int16_t stripScreenX = p->tileX * 16;
        int16_t stripScreenY = (p->tileY - 1) * 16;

        int16_t tileTop    = p->tileY - 1;
        int16_t tileMid    = p->tileY;
        int16_t tileBottom = p->tileY + 1;
        int16_t sprOffY    = p->y - stripScreenY;

        const uint16_t *bgTop    = GetTileSprite(tiles, stripScreenX, tileTop    * 16);
        const uint16_t *bgMiddle = GetTileSprite(tiles, stripScreenX, tileMid    * 16);
        const uint16_t *bgBottom = GetTileSprite(tiles, stripScreenX, tileBottom * 16);
        const uint16_t *spr      = Player_GetCurrentFrame(p);

        for (uint8_t row = 0; row < 48; row++) {
            uint16_t *line = &lineBuf[row * 16];
            const uint16_t *bg;
            uint8_t bgRow;

            if (row < 16)      { bg = bgTop;    bgRow = row;      }
            else if (row < 32) { bg = bgMiddle; bgRow = row - 16; }
            else               { bg = bgBottom; bgRow = row - 32; }

            if (bg) memcpy(line, &bg[bgRow * 16], 16 * sizeof(uint16_t));
            else    memset(line, 0,               16 * sizeof(uint16_t));
        }

        /* Composite resting boxes that fall in this strip */
        for (uint8_t i = 0; i < boxCount; i++) {
            if (!boxes[i].active) continue;
            if (boxes[i].tileX != p->tileX) continue;
            int16_t bufY = boxes[i].tileY * 16 - stripScreenY;
            if (bufY < 0 || bufY >= 48) continue;
            BlendBoxAtV(lineBuf, bufY, 48);
        }

        /* Player sprite */
        for (uint8_t row = 0; row < 16; row++) {
            int16_t bufRow = sprOffY + row;
            if (bufRow < 0 || bufRow >= 48) continue;
            for (uint8_t col = 0; col < 16; col++) {
                uint16_t px2 = spr[row * 16 + col];
                if (px2 != 0xFFFF)
                    lineBuf[bufRow * 16 + col] = px2;
            }
        }

        ST7789_DrawImage(stripScreenX, stripScreenY, 16, 48, lineBuf);

    } else {
        static uint16_t lineBuf[48 * 16];

        int16_t stripScreenX = (p->tileX - 1) * 16;
        int16_t stripScreenY =  p->tileY      * 16;
        int16_t sprOffX      =  p->x - stripScreenX;

        const uint16_t *bgLeft   = GetTileSprite(tiles, (p->tileX - 1) * 16, stripScreenY);
        const uint16_t *bgMiddle = GetTileSprite(tiles,  p->tileX      * 16, stripScreenY);
        const uint16_t *bgRight  = GetTileSprite(tiles, (p->tileX + 1) * 16, stripScreenY);
        const uint16_t *spr      = Player_GetCurrentFrame(p);

        for (uint8_t row = 0; row < 16; row++) {
            uint16_t *line = &lineBuf[row * 48];

            if (bgLeft)   memcpy(&line[0],  &bgLeft[row   * 16], 16 * sizeof(uint16_t));
            else          memset(&line[0],  0,                   16 * sizeof(uint16_t));
            if (bgMiddle) memcpy(&line[16], &bgMiddle[row * 16], 16 * sizeof(uint16_t));
            else          memset(&line[16], 0,                   16 * sizeof(uint16_t));
            if (bgRight)  memcpy(&line[32], &bgRight[row  * 16], 16 * sizeof(uint16_t));
            else          memset(&line[32], 0,                   16 * sizeof(uint16_t));
        }

        /* Composite resting boxes in this strip */
        for (uint8_t i = 0; i < boxCount; i++) {
            if (!boxes[i].active) continue;
            if (boxes[i].tileY != p->tileY) continue;
            int16_t bufX = boxes[i].tileX * 16 - stripScreenX;
            if (bufX < 0 || bufX >= 48) continue;
            BlendBoxAt(lineBuf, bufX, 48);
        }

        /* Player sprite */
        for (uint8_t row = 0; row < 16; row++) {
            for (uint8_t col = 0; col < 16; col++) {
                int16_t bufCol = sprOffX + col;
                if (bufCol < 0 || bufCol >= 48) continue;
                uint16_t px2 = spr[row * 16 + col];
                if (px2 != 0xFFFF)
                    lineBuf[row * 48 + bufCol] = px2;
            }
        }

        ST7789_DrawImage(stripScreenX, stripScreenY, 48, 16, lineBuf);
    }
}

/* -----------------------------------------------------------------------
 * Renderer_DrawPushStrip
 * 3-tile composite: [player-origin tile | box-origin tile | tile-ahead-of-box]
 * Player and box sprites are blended at their interpolated pixel positions.
 * Any OTHER resting boxes that happen to fall in the strip are also composited.
 * --------------------------------------------------------------------- */

/**
 * @brief Renders the 3-tile strip covering both the pushing player and the pushed box.
 *
 * Unlike Renderer_DrawPlayerStrip(), this function always covers three
 * consecutive tiles: the player's current tile, the box's current tile, and
 * the tile the box is moving into.  Both the player and the pushed box are
 * drawn at their sub-pixel interpolated positions.  Other resting boxes that
 * overlap the strip are composited before them.
 *
 * Handles horizontal and vertical push directions independently.
 *
 * @param tiles     15×15 tile map.
 * @param p         Pointer to the Player.
 * @param pushedBox Pointer to the box being pushed.
 * @param boxes     Full box array (the pushed entry is skipped for resting compositing).
 * @param boxCount  Number of entries in @p boxes.
 */
void Renderer_DrawPushStrip(const Level tiles[15][15], const Player *p, const Box *pushedBox,
                             const Box boxes[], uint8_t boxCount) {
    bool isVertical = (p->tileY != p->targetTileY);

    if (!isVertical) {
        /* Horizontal push ------------------------------------------------- */
        static uint16_t lineBuf[16 * 48];

        /* Strip always starts at the leftmost of the 3 tiles */
        int16_t minTileX    = (p->tileX < pushedBox->targetTileX)
                              ? p->tileX : pushedBox->targetTileX;
        int16_t stripScreenX = minTileX * 16;
        int16_t stripScreenY = p->tileY * 16;

        const uint16_t *bg0 = GetTileSprite(tiles, (minTileX + 0) * 16, stripScreenY);
        const uint16_t *bg1 = GetTileSprite(tiles, (minTileX + 1) * 16, stripScreenY);
        const uint16_t *bg2 = GetTileSprite(tiles, (minTileX + 2) * 16, stripScreenY);

        for (uint8_t row = 0; row < 16; row++) {
            uint16_t *line = &lineBuf[row * 48];

            if (bg0) memcpy(&line[0],  &bg0[row * 16], 16 * sizeof(uint16_t));
            else     memset(&line[0],  0,              16 * sizeof(uint16_t));
            if (bg1) memcpy(&line[16], &bg1[row * 16], 16 * sizeof(uint16_t));
            else     memset(&line[16], 0,              16 * sizeof(uint16_t));
            if (bg2) memcpy(&line[32], &bg2[row * 16], 16 * sizeof(uint16_t));
            else     memset(&line[32], 0,              16 * sizeof(uint16_t));
        }

        /* Other resting boxes in the strip (not the pushed one) */
        for (uint8_t i = 0; i < boxCount; i++) {
            if (!boxes[i].active) continue;
            if (&boxes[i] == pushedBox) continue;
            if (boxes[i].tileY != p->tileY) continue;
            int16_t bufX = boxes[i].tileX * 16 - stripScreenX;
            if (bufX < 0 || bufX >= 48) continue;
            BlendBoxAt(lineBuf, bufX, 48);
        }

        /* Box sprite at interpolated position */
        int16_t boxOffX = pushedBox->x - stripScreenX;
        BlendBoxAt(lineBuf, boxOffX, 48);

        /* Player sprite at interpolated position */
        const uint16_t *spr = Player_GetCurrentFrame(p);
        int16_t playerOffX  = p->x - stripScreenX;
        for (uint8_t row = 0; row < 16; row++) {
            for (uint8_t col = 0; col < 16; col++) {
                int16_t bufCol = playerOffX + col;
                if (bufCol < 0 || bufCol >= 48) continue;
                uint16_t px2 = spr[row * 16 + col];
                if (px2 != 0xFFFF)
                    lineBuf[row * 48 + bufCol] = px2;
            }
        }

        ST7789_DrawImage(stripScreenX, stripScreenY, 48, 16, lineBuf);

    } else {
        /* Vertical push --------------------------------------------------- */
        static uint16_t lineBuf[48 * 16];

        int16_t minTileY     = (p->tileY < pushedBox->targetTileY)
                               ? p->tileY : pushedBox->targetTileY;
        int16_t stripScreenX = p->tileX  * 16;
        int16_t stripScreenY = minTileY  * 16;

        const uint16_t *bg0 = GetTileSprite(tiles, stripScreenX, (minTileY + 0) * 16);
        const uint16_t *bg1 = GetTileSprite(tiles, stripScreenX, (minTileY + 1) * 16);
        const uint16_t *bg2 = GetTileSprite(tiles, stripScreenX, (minTileY + 2) * 16);

        for (uint8_t row = 0; row < 48; row++) {
            uint16_t *line = &lineBuf[row * 16];
            const uint16_t *bg;
            uint8_t bgRow;

            if (row < 16)      { bg = bg0; bgRow = row;      }
            else if (row < 32) { bg = bg1; bgRow = row - 16; }
            else               { bg = bg2; bgRow = row - 32; }

            if (bg) memcpy(line, &bg[bgRow * 16], 16 * sizeof(uint16_t));
            else    memset(line, 0,               16 * sizeof(uint16_t));
        }

        /* Other resting boxes in the strip */
        for (uint8_t i = 0; i < boxCount; i++) {
            if (!boxes[i].active) continue;
            if (&boxes[i] == pushedBox) continue;
            if (boxes[i].tileX != p->tileX) continue;
            int16_t bufY = boxes[i].tileY * 16 - stripScreenY;
            if (bufY < 0 || bufY >= 48) continue;
            BlendBoxAtV(lineBuf, bufY, 48);
        }

        /* Box sprite */
        int16_t boxOffY = pushedBox->y - stripScreenY;
        BlendBoxAtV(lineBuf, boxOffY, 48);

        /* Player sprite */
        const uint16_t *spr = Player_GetCurrentFrame(p);
        int16_t playerOffY  = p->y - stripScreenY;
        for (uint8_t row = 0; row < 16; row++) {
            int16_t bufRow = playerOffY + row;
            if (bufRow < 0 || bufRow >= 48) continue;
            for (uint8_t col = 0; col < 16; col++) {
                uint16_t px2 = spr[row * 16 + col];
                if (px2 != 0xFFFF)
                    lineBuf[bufRow * 16 + col] = px2;
            }
        }

        ST7789_DrawImage(stripScreenX, stripScreenY, 16, 48, lineBuf);
    }
}

/**
 * @brief Clears the display and draws the game title image.
 *
 * Fills the screen with the background colour then centres the
 * 181×162-pixel title sprite.
 */
void Renderer_DrawTitleScreen(void) {
    ST7789_Fill_Color(0x2200);
    ST7789_DrawImage(30, 39, 181, 162, game_title);
}

/**
 * @brief Clears the display and draws the "Level Complete" overlay.
 *
 * Fills the screen with the background colour then draws the
 * 174×80-pixel banner centred vertically.
 */
void Renderer_DrawLevelCompleteScreen(void) {
    ST7789_Fill_Color(0x2200);
    ST7789_DrawImage(33, 85, 174, 80, level_complete);
}

/**
 * @brief Clears the display and draws the "Game Complete" overlay.
 *
 * Fills the screen with the background colour then draws the
 * 193×80-pixel banner centred vertically.
 */
void Renderer_DrawGameCompleteScreen(void) {
    ST7789_Fill_Color(0x2200);
    ST7789_DrawImage(23, 85, 193, 80, game_complete);
}