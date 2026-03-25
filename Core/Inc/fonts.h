/**
 * @file fonts.h
 * @brief Bitmap font definitions for the ST7789 display driver.
 *
 * Declares the FontDef structure and the pre-built font instances that can
 * be passed to the display driver for text rendering.  Also exposes optional
 * RGB565 image assets that can be loaded into the display framebuffer.
 *
 * @warning Large image arrays may exceed on-chip flash capacity on
 *          memory-constrained MCUs.  Enable only the assets you need.
 */

#ifndef __FONT_H
#define __FONT_H

#include "stdint.h"

/**
 * @brief Describes a single bitmap font face.
 *
 * Font data is stored as a flat array of 16-bit RGB565 pixels,
 * where each character occupies @c width × @c height pixels.
 */
typedef struct {
    const uint8_t  width;  /**< Glyph width in pixels. */
    uint8_t        height; /**< Glyph height in pixels. */
    const uint16_t *data;  /**< Pointer to the raw RGB565 glyph bitmap data. */
} FontDef;

/** @name Built-in font library
 *  @{
 */
extern FontDef Font_7x10;  /**< 7×10-pixel font. */
extern FontDef Font_11x18; /**< 11×18-pixel font. */
extern FontDef Font_16x26; /**< 16×26-pixel font. */
/** @} */

/**
 * @name 16-bit (RGB565) image assets
 *
 * @caution If the MCU on-chip flash cannot store such large image data,
 *          do not enable these assets.  They are provided for test purposes.
 * @{
 */
/** @brief 128×128 pixel RGB565 test image. */
extern const uint16_t saber[][128];

/*
 * 240×240 pixel RGB565 test images (disabled by default):
 * extern const uint16_t knky[][240];
 * extern const uint16_t tek[][240];
 * extern const uint16_t adi1[][240];
 */
/** @} */

#endif /* __FONT_H */
