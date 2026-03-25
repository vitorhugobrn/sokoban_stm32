/**
 * @file box.c
 * @brief Movable box logic for the Sokoban game.
 *
 * Implements smooth tile-to-tile movement via linear interpolation and
 * collision checks against walls and other boxes.
 */

#include "box.h"

/**
 * @brief Updates a box's interpolated pixel position.
 *
 * Clears @c posChanged, then, if the box is active and not yet at its target
 * tile, advances @c moveAccumulator by @p dt.  When the accumulator reaches
 * @c cycleDuration the box snaps to the target; otherwise the pixel position
 * is linearly interpolated using a 0–255 fixed-point progress scalar.
 *
 * @param b  Pointer to the Box to update.
 * @param dt Elapsed time in microseconds since the last frame.
 */
void Box_Update(Box *b, uint32_t dt) {
    b->posChanged = false;
    if (!b->active) return;
    if (b->tileX == b->targetTileX && b->tileY == b->targetTileY) return;

    b->moveAccumulator += dt;

    if (b->moveAccumulator >= b->cycleDuration) {
        b->moveAccumulator = 0;
        b->tileX = b->targetTileX;
        b->tileY = b->targetTileY;
        b->x     = b->tileX * 16;
        b->y     = b->tileY * 16;
        b->posChanged = true;
    } else {
        uint32_t progress = (b->moveAccumulator * 256U) / b->cycleDuration;

        int16_t startX = b->tileX       * 16;
        int16_t startY = b->tileY       * 16;
        int16_t endX   = b->targetTileX * 16;
        int16_t endY   = b->targetTileY * 16;

        int16_t newX = startX + ((int16_t)(endX - startX) * (int16_t)progress) / 256;
        int16_t newY = startY + ((int16_t)(endY - startY) * (int16_t)progress) / 256;

        if (newX != b->x || newY != b->y) {
            b->x = newX;
            b->y = newY;
            b->posChanged = true;
        }
    }
}

/**
 * @brief Attempts to begin pushing the box one tile in direction (@p dx, @p dy).
 *
 * Validates:
 *  - The destination tile is within the 15×15 grid.
 *  - The destination is not a wall tile.
 *  - No other active box occupies the destination.
 *
 * On success, sets the target tile, resets @c moveAccumulator, and flags
 * @c posChanged.
 *
 * @param b        Pointer to the Box to push.
 * @param dx       Horizontal tile delta (-1, 0, or 1).
 * @param dy       Vertical tile delta (-1, 0, or 1).
 * @param tiles    Level tile map for wall collision.
 * @param boxes    All box instances for box-on-box collision.
 * @param boxCount Number of entries in @p boxes.
 * @return @c true if the push was accepted; @c false if blocked.
 */
bool Box_StartPush(Box *b, int8_t dx, int8_t dy, const Level tiles[15][15], const Box boxes[], uint8_t boxCount) {
    int16_t newTileX = b->tileX + dx;
    int16_t newTileY = b->tileY + dy;

    if (newTileX < 0 || newTileX >= 15 || newTileY < 0 || newTileY >= 15)
        return false;

    Level tile = tiles[newTileY][newTileX];
    if (tile == WALL_SIDE || tile == WALL_TOP)
        return false;

    if (Box_At(boxes, boxCount, newTileX, newTileY) >= 0)
        return false;

    b->targetTileX     = newTileX;
    b->targetTileY     = newTileY;
    b->moveAccumulator = 0;
    b->posChanged      = true;
    return true;
}

/**
 * @brief Searches for an active box occupying tile (@p tx, @p ty).
 *
 * @param boxes Array of boxes to search.
 * @param count Number of entries in @p boxes.
 * @param tx    Tile column to query.
 * @param ty    Tile row to query.
 * @return Index (≥ 0) of the matching box, or -1 if no active box is found.
 */
int8_t Box_At(const Box boxes[], uint8_t count, int16_t tx, int16_t ty) {
    for (uint8_t i = 0; i < count; i++) {
        if (!boxes[i].active) continue;
        if (boxes[i].tileX == tx && boxes[i].tileY == ty)
            return (int8_t)i;
    }
    
    return -1;
}
