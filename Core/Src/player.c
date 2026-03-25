/**
 * @file player.c
 * @brief Player animation and movement logic for the Sokoban game.
 *
 * Implements frame-based animation cycling, tile-to-tile movement
 * interpolation, wall and box collision, and sprite frame selection.
 */

#include "player.h"
#include "box.h"
#include "sprites.h"

/**
 * @brief Advances the player's animation and movement state each frame.
 *
 * Always increments @c animAccumulator by @p dt and cycles the animation
 * @c frame every @c frameDuration microseconds (wrapping at frame 4).
 *
 * When a walk state is active (@c state >= @c ANIM_WALK_DOWN), also increments
 * @c moveAccumulator.  On reaching @c cycleDuration:
 *  - The player snaps to @c targetTileX / @c targetTileY.
 *  - @c state reverts to the matching idle state (@c ANIM_WALK_x → @c ANIM_IDLE_x).
 *  - Accumulators and @c frame are reset and @c pushedBoxIdx is cleared.
 *
 * Otherwise, pixel position is linearly interpolated from the origin tile to
 * the target tile using a 0–255 fixed-point progress scalar.
 *
 * @param p  Pointer to the Player to update.
 * @param dt Elapsed time in microseconds since the last frame.
 */
void Player_Update(Player *p, uint32_t dt) {
    p->frameChanged     = false;
    p->posChanged       = false;
    p->animAccumulator += dt;

    while (p->animAccumulator >= p->frameDuration) {
        p->animAccumulator -= p->frameDuration;
        p->frame++;
        p->frameChanged = true;

        if (p->frame >= 4)
            p->frame = 0;
    }

    if (p->state >= ANIM_WALK_DOWN) {
        p->moveAccumulator += dt;

        if (p->moveAccumulator >= p->cycleDuration) {
            /* Arrived at target tile */
            p->moveAccumulator = 0;
            p->animAccumulator = 0;
            p->tileX = p->targetTileX;
            p->tileY = p->targetTileY;
            p->x     = p->tileX * 16;
            p->y     = p->tileY * 16;
            p->state = p->state - 4;  /* ANIM_WALK_## -> ANIM_IDLE_## */
            p->frame = 0;
            p->frameChanged = true;
            p->posChanged   = true;
            p->pushedBoxIdx = -1;     /* push finished */
        } else {
            /* Interpolate pixel position */
            uint32_t progress = (p->moveAccumulator * 256U) / p->cycleDuration;

            int16_t startX = p->tileX       * 16;
            int16_t startY = p->tileY       * 16;
            int16_t endX   = p->targetTileX * 16;
            int16_t endY   = p->targetTileY * 16;

            int16_t newX = startX + ((int16_t)(endX - startX) * (int16_t)(progress)) / 256;
            int16_t newY = startY + ((int16_t)(endY - startY) * (int16_t)(progress)) / 256;

            if (newX != p->x || newY != p->y) {
                p->x = newX;
                p->y = newY;
                p->posChanged = true;
            }
        }
    }
}

/**
 * @brief Attempts to begin walking the player in direction @p dir.
 *
 * Returns @c false immediately if the player is already moving.
 * Resolves the tile delta from @p dir, then validates:
 *  - The target tile is within the 15×15 grid.
 *  - The target tile is not a wall.
 *  - If a box occupies the target tile, Box_StartPush() succeeds.
 *
 * On success, @c targetTileX/Y, @c state, @c frame, and both accumulators
 * are updated, and @c pushedBoxIdx is set to the pushed box index (or -1).
 *
 * @param p        Pointer to the Player.
 * @param dir      Walk direction (one of @c ANIM_WALK_DOWN/UP/LEFT/RIGHT).
 * @param tiles    Level tile map for wall collision.
 * @param boxes    Box array for push interaction.
 * @param boxCount Number of entries in @p boxes.
 * @return @c true if movement started; @c false if blocked or already moving.
 */
bool Player_StartWalk(Player *p, AnimState dir,
                      const Level tiles[15][15],
                      Box boxes[], uint8_t boxCount) {
    if (p->state > ANIM_IDLE_LEFT)
        return false;

    int8_t dx = 0, dy = 0;
    switch (dir) {
        case ANIM_WALK_RIGHT: dx =  1; break;
        case ANIM_WALK_LEFT:  dx = -1; break;
        case ANIM_WALK_DOWN:  dy =  1; break;
        case ANIM_WALK_UP:    dy = -1; break;
        default: return false;
    }

    int16_t newTileX = p->tileX + dx;
    int16_t newTileY = p->tileY + dy;

    if (newTileX < 0 || newTileX >= 15 || newTileY < 0 || newTileY >= 15)
        return false;

    Level tile = tiles[newTileY][newTileX];
    if (tile == WALL_SIDE || tile == WALL_TOP)
        return false;

    /* Check for box in target tile */
    int8_t boxIdx = Box_At(boxes, boxCount, newTileX, newTileY);
    if (boxIdx >= 0) {
        /* Try to push it */
        if (!Box_StartPush(&boxes[boxIdx], dx, dy, tiles, boxes, boxCount))
            return false;
        p->pushedBoxIdx = boxIdx;
    } else {
        p->pushedBoxIdx = -1;
    }

    p->targetTileX     = newTileX;
    p->targetTileY     = newTileY;
    p->state           = dir;
    p->frame           = 0;
    p->animAccumulator = 0;
    p->moveAccumulator = 0;
    return true;
}

/**
 * @brief Returns a pointer to the current animation frame's pixel data.
 *
 * Selects the appropriate frame pointer array based on @c p->state and
 * indexes it with @c p->frame.  Falls back to @c idleDownFrames[0] for
 * any unrecognised state.
 *
 * @param p Pointer to the (const) Player.
 * @return Pointer to a 16×16 RGB565 sprite (256 pixels).
 */
const uint16_t *Player_GetCurrentFrame(const Player *p) {
    switch (p->state) {
        case ANIM_WALK_DOWN:  return walkDownFrames[p->frame];
        case ANIM_WALK_UP:    return walkUpFrames[p->frame];
        case ANIM_WALK_LEFT:  return walkLeftFrames[p->frame];
        case ANIM_WALK_RIGHT: return walkRightFrames[p->frame];
        case ANIM_IDLE_DOWN:  return idleDownFrames[p->frame];
        case ANIM_IDLE_UP:    return idleUpFrames[p->frame];
        case ANIM_IDLE_LEFT:  return idleLeftFrames[p->frame];
        case ANIM_IDLE_RIGHT: return idleRightFrames[p->frame];
        default:              return idleDownFrames[0];
    }
}
