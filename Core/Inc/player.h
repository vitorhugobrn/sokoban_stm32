/**
 * @file player.h
 * @brief Player entity and movement logic for the Sokoban game.
 *
 * Defines the AnimState enumeration, the Player data structure, and the
 * functions that drive player animation, movement, and box interaction.
 */

#ifndef __PLAYER_H
#define __PLAYER_H

#include <stdint.h>
#include <stdbool.h>
#include "levels.h"
#include "box.h"

/**
 * @brief Animation states for the player character.
 *
 * Idle states (0–3) are used when the player is standing still facing a
 * direction.  Walk states (4–7) are active during tile-to-tile movement.
 * The numeric relationship @c ANIM_WALK_x == @c ANIM_IDLE_x + 4 is
 * relied upon in Player_Update() to transition back to idle on arrival.
 */
typedef enum {
	ANIM_IDLE_DOWN  = 0, /**< Idle, facing down. */
	ANIM_IDLE_RIGHT,     /**< Idle, facing right. */
	ANIM_IDLE_UP,        /**< Idle, facing up. */
	ANIM_IDLE_LEFT,      /**< Idle, facing left. */

	ANIM_WALK_DOWN,      /**< Walking downward. */
	ANIM_WALK_RIGHT,     /**< Walking rightward. */
	ANIM_WALK_UP,        /**< Walking upward. */
	ANIM_WALK_LEFT       /**< Walking leftward. */
} AnimState;

/**
 * @brief Represents the player character on the game board.
 *
 * Tracks both the logical tile position (used for game-logic queries) and
 * the interpolated pixel position (used for smooth rendering).
 */
typedef struct Player {
    int16_t x, y;             /**< Current pixel position (interpolated). */
    int16_t prevX, prevY;     /**< Pixel position from the previous frame. */

    int16_t tileX, tileY;         /**< Current (origin) tile coordinates. */
    int16_t targetTileX, targetTileY; /**< Destination tile coordinates during movement. */

    AnimState state;           /**< Active animation state. */
    uint8_t   frame;           /**< Current frame index within the animation (0–3). */
    uint32_t  animAccumulator; /**< Elapsed time (µs) within the current animation frame. */
    uint32_t  moveAccumulator; /**< Elapsed time (µs) since movement started. */
    uint32_t  frameDuration;   /**< Duration of each animation frame in µs. */
    uint32_t  cycleDuration;   /**< Duration of a full tile-to-tile move in µs. */
    bool      frameChanged;    /**< @c true when the animation frame advanced this update. */
    bool      posChanged;      /**< @c true when the pixel position changed this update. */

    int8_t    pushedBoxIdx;    /**< Index into the boxes array of the box being pushed, or -1. */
} Player;

/**
 * @brief Advances the player's animation and movement each game frame.
 *
 * Updates the animation accumulator to cycle through frames at @c frameDuration
 * intervals.  When a walk state is active, also advances the movement
 * accumulator; on reaching @c cycleDuration the player snaps to the target
 * tile and reverts to the corresponding idle state.
 *
 * @param p  Pointer to the Player to update.
 * @param dt Elapsed time in microseconds since the last frame.
 */
void Player_Update(Player *p, uint32_t dt);

/**
 * @brief Attempts to begin walking in the specified direction.
 *
 * Validates that the player is idle, the target tile is in bounds and not a
 * wall, and (if occupied by a box) that the box can be pushed.  On success
 * the player's target tile and animation state are set and the push is
 * registered in @c pushedBoxIdx.
 *
 * @param p        Pointer to the Player.
 * @param dir      Desired walk direction (must be one of the @c ANIM_WALK_* values).
 * @param tiles    The 15×15 level tile map used for wall collision.
 * @param boxes    Array of boxes used for box interaction.
 * @param boxCount Number of entries in @p boxes.
 * @return @c true if movement started, @c false if it is blocked.
 */
bool Player_StartWalk(Player *p, AnimState dir, const Level tiles[15][15], Box boxes[], uint8_t boxCount);

/**
 * @brief Returns a pointer to the current animation frame's pixel data.
 *
 * Selects the appropriate frame array from sprites.h based on the player's
 * @c state and @c frame index.
 *
 * @param p Pointer to the (const) Player.
 * @return Pointer to a 16×16 RGB565 sprite (256 pixels).
 */
const uint16_t *Player_GetCurrentFrame(const Player *p);

#endif /* __PLAYER_H */
