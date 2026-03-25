/**
 * @file renderer.h
 * @brief High-level rendering interface for the Sokoban game.
 *
 * Provides functions that composite tile maps, boxes, and the player sprite
 * into pixel strips and push them to the ST7789 display, as well as functions
 * that draw full-screen UI overlays (title screen, level-complete, game-complete).
 */

#ifndef __RENDERER_H
#define __RENDERER_H

#include <stdint.h>
#include "levels.h"
#include "player.h"
#include "box.h"

/**
 * @brief Draws the entire level tile map to the display.
 *
 * Iterates over every tile in the 15×15 grid and blits the appropriate sprite.
 * VOID tiles are skipped.  BOX_SPOT tiles draw a floor tile first, then the
 * box-spot overlay with alpha masking.
 *
 * @param tiles The 15×15 tile map to render (row-major: [row][col]).
 */
void Renderer_DrawLevel(const Level tiles[15][15]);

/**
 * @brief Composites each active box over its floor tile and blits it.
 *
 * For every active box in @p boxes, fetches the underlying floor sprite,
 * blends the box sprite on top (magenta @c 0xFFFF is treated as transparent),
 * and draws the resulting 16×16 pixel tile.
 *
 * @param tiles    The 15×15 tile map used to determine the floor background.
 * @param boxes    Array of boxes to render.
 * @param boxCount Number of entries in @p boxes.
 */
void Renderer_DrawBoxes(const Level tiles[15][15], const Box boxes[], uint8_t boxCount);

/**
 * @brief Renders the player's 3-tile animation strip during idle or solo movement.
 *
 * Builds a 48×16 (horizontal movement) or 16×48 (vertical movement) buffer
 * containing the three floor tiles centred on the player, blends any resting
 * boxes that overlap the strip, adds the player sprite at its interpolated
 * pixel offset, and blits the buffer in a single DMA transfer.
 *
 * @param tiles    The 15×15 tile map.
 * @param p        Pointer to the Player.
 * @param boxes    Full box array (used to composite resting boxes in the strip).
 * @param boxCount Number of entries in @p boxes.
 */
void Renderer_DrawPlayerStrip(const Level tiles[15][15], const Player *p, const Box boxes[], uint8_t boxCount);

/**
 * @brief Renders the 3-tile strip covering both the player and a pushed box.
 *
 * Covers the player origin tile, the box origin tile, and the tile ahead of the
 * box.  Both the player and the box are drawn at their interpolated pixel
 * positions.  Any other resting boxes that fall inside the strip are also
 * composited before the blit.
 *
 * @param tiles      The 15×15 tile map.
 * @param p          Pointer to the Player.
 * @param pushedBox  Pointer to the box currently being pushed.
 * @param boxes      Full box array (used to composite other resting boxes).
 * @param boxCount   Number of entries in @p boxes.
 */
void Renderer_DrawPushStrip(const Level tiles[15][15], const Player *p, const Box *pushedBox, const Box boxes[], uint8_t boxCount);

/**
 * @brief Clears the screen and draws the game title image.
 */
void Renderer_DrawTitleScreen(void);

/**
 * @brief Clears the screen and draws the "Level Complete" overlay.
 */
void Renderer_DrawLevelCompleteScreen(void);

/**
 * @brief Clears the screen and draws the "Game Complete" overlay.
 */
void Renderer_DrawGameCompleteScreen(void);

#endif /* __RENDERER_H */