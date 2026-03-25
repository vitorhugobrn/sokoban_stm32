/**
 * @file box.h
 * @brief Movable box entity for the Sokoban game.
 *
 * Defines the Box data structure and the functions that drive box
 * movement and collision detection. A box can be pushed by the player
 * and slides smoothly to its target tile using fixed-point interpolation.
 */

#ifndef __BOX_H
#define __BOX_H

#include <stdint.h>
#include <stdbool.h>
#include "levels.h"

/**
 * @brief Represents a pushable box on the game board.
 *
 * Each box tracks both its current sub-pixel position (used for smooth
 * animation) and its logical tile position (used for collision queries).
 */
typedef struct Box {
    int16_t  x, y;             /**< Pixel position (interpolated during movement). */
    int16_t  tileX, tileY;     /**< Current (origin) tile coordinates. */
    int16_t  targetTileX, targetTileY; /**< Destination tile coordinates. */

    uint32_t moveAccumulator;  /**< Elapsed time (µs) since the push began. */
    uint32_t cycleDuration;    /**< Total push duration in microseconds; should match the player. */

    bool     posChanged;       /**< Set to @c true when the pixel position changed this frame. */
    bool     active;           /**< @c false means this box slot is unused. */
} Box;

/**
 * @brief Updates a box's interpolated pixel position each game frame.
 *
 * Advances the movement accumulator by @p dt. When the accumulator reaches
 * @c cycleDuration the box snaps to its target tile; otherwise the pixel
 * position is linearly interpolated between the origin tile and the target.
 *
 * @param b  Pointer to the Box to update.
 * @param dt Elapsed time in microseconds since the last frame.
 */
void   Box_Update   (Box *b, uint32_t dt);

/**
 * @brief Attempts to start pushing a box in the given direction.
 *
 * Validates that the destination tile is within bounds, is not a wall, and
 * is not already occupied by another box.  On success the box's target tile
 * is updated and its move accumulator is reset.
 *
 * @param b        Pointer to the Box to push.
 * @param dx       Horizontal tile delta (-1, 0, or 1).
 * @param dy       Vertical tile delta (-1, 0, or 1).
 * @param tiles    The 15×15 level tile map used for wall collision.
 * @param boxes    Array of all boxes used for box-on-box collision.
 * @param boxCount Number of entries in @p boxes.
 * @return @c true if the push was accepted, @c false if it is blocked.
 */
bool   Box_StartPush(Box *b, int8_t dx, int8_t dy, const Level tiles[15][15], const Box boxes[], uint8_t boxCount);

/**
 * @brief Returns the index of the box occupying the given tile, if any.
 *
 * @param boxes Array of boxes to search.
 * @param count Number of entries in @p boxes.
 * @param tx    Tile column to query.
 * @param ty    Tile row to query.
 * @return The index (≥ 0) of the box at (@p tx, @p ty), or -1 if none.
 */
int8_t Box_At       (const Box boxes[], uint8_t count, int16_t tx, int16_t ty);

#endif /* __BOX_H */
