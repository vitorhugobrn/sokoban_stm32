/**
 * @file sprites.h
 * @brief Sprite and image asset declarations for the Sokoban game.
 *
 * All pixel data is stored as RGB565 (16-bit, big-endian) arrays in sprites.c.
 * Every sprite is 16×16 pixels unless otherwise noted.
 * Magenta (0xFFFF) is the transparency key used by the renderer.
 */

#ifndef __SPRITES_H
#define __SPRITES_H

#include "stdint.h"

/** @name Tile sprites (16×16 px each)
 *  @{
 */
extern const uint16_t box_var1[16][16];   /**< Box sprite (variant 1). */
extern const uint16_t box_spot[16][16];   /**< Box target-spot overlay sprite. */

extern const uint16_t steel_side[16][16]; /**< Side-facing wall tile sprite. */
extern const uint16_t steel_top[16][16];  /**< Top-facing wall tile sprite. */
/** @} */

/** @name Full-screen UI images
 *  @{
 */
extern const uint16_t game_title[162][181];       /**< Game title image (162 rows × 181 cols). */
extern const uint16_t level_complete[80][174];    /**< "Level Complete" banner (80×174). */
extern const uint16_t game_complete[80][193];     /**< "Game Complete" banner (80×193). */
/** @} */

/**
 * @name Floor tile variants
 * Seven randomised 16×16 floor tile sprites.  The renderer selects a variant
 * per tile using a positional hash so the floor looks non-uniform.
 * @{
 */
extern const uint16_t *floorTiles[]; /**< Array of pointers to the 7 floor tile sprites. */
/** @} */

/**
 * @name Player animation frames
 * Each array holds 4 frames (16×16 px) for the corresponding direction and state.
 * @{
 */
extern const uint16_t *idleDownFrames[];  /**< Idle-down animation frames. */
extern const uint16_t *idleUpFrames[];   /**< Idle-up animation frames. */
extern const uint16_t *idleRightFrames[];/**< Idle-right animation frames. */
extern const uint16_t *idleLeftFrames[]; /**< Idle-left animation frames. */

extern const uint16_t *walkDownFrames[];  /**< Walk-down animation frames. */
extern const uint16_t *walkUpFrames[];   /**< Walk-up animation frames. */
extern const uint16_t *walkRightFrames[];/**< Walk-right animation frames. */
extern const uint16_t *walkLeftFrames[]; /**< Walk-left animation frames. */
/** @} */

#endif /* __SPRITES_H */
