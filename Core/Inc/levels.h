/**
 * @file levels.h
 * @brief Level data and management for the Sokoban game.
 *
 * Defines tile types, level descriptors, and the functions used to load,
 * query, and check completion of levels at runtime.
 */

#ifndef __LEVELS_H
#define __LEVELS_H

#include <stdint.h>
#include <stdbool.h>

/** @brief Maximum number of boxes (and box-spots) supported per level. */
#define BOX_MAX 8

/* Forward declarations to avoid circular includes. */
struct Box;
struct Player;

/**
 * @brief Tile types that make up a Sokoban level grid.
 */
typedef enum {
    VOID     = 0, /**< Empty space outside the playable area. */
    WALL_SIDE,    /**< Side-facing wall tile (rendered with the side texture). */
    WALL_TOP,     /**< Top-facing wall tile (rendered with the top texture). */
    FLOOR,        /**< Walkable floor tile. */
    BOX_SPOT      /**< Target location where a box must be pushed. */
} Level;

/**
 * @brief Static descriptor for a single level.
 *
 * Contains the complete tile map, the player spawn position, and the initial
 * positions of all boxes.
 */
typedef struct {
    Level   tiles[15][15]; /**< 15×15 grid of tile types (row-major: [row][col]). */
    uint8_t spawnX;        /**< Player spawn column (tile units). */
    uint8_t spawnY;        /**< Player spawn row (tile units). */
    uint8_t boxCount;      /**< Number of active boxes in this level. */
    uint8_t boxX[BOX_MAX]; /**< Initial tile column for each box. */
    uint8_t boxY[BOX_MAX]; /**< Initial tile row for each box. */
} LevelDesc;

/** @brief Descriptor for level 1. */
extern const LevelDesc level1Desc;

/** @brief Descriptor for level 2. */
extern const LevelDesc level2Desc;

/**
 * @name Box-spot cache
 * These globals are populated by Level_LoadSpots() and used by
 * Level_IsComplete() to check whether all spots are covered.
 * @{
 */
extern uint8_t currentSpotCount;          /**< Number of box-spots in the current level. */
extern uint8_t currentSpotX[BOX_MAX];     /**< Column of each box-spot (tile units). */
extern uint8_t currentSpotY[BOX_MAX];     /**< Row of each box-spot (tile units). */
/** @} */

/**
 * @brief Scans the tile map and caches all BOX_SPOT positions.
 *
 * Populates @c currentSpotCount, @c currentSpotX, and @c currentSpotY.
 * Must be called once after loading a new level before using Level_IsComplete().
 *
 * @param tiles The 15×15 tile map to scan.
 */
void Level_LoadSpots(const Level tiles[15][15]);

/**
 * @brief Checks whether every box-spot is covered by an active box.
 *
 * Compares the number of active boxes against the cached spot count, then
 * verifies that each spot has a box on it using Box_At().
 *
 * @param boxes    Array of all boxes in the current level.
 * @param boxCount Number of entries in @p boxes.
 * @return @c true if the level is complete, @c false otherwise.
 */
bool Level_IsComplete(const struct Box *boxes, uint8_t boxCount);

/**
 * @brief Returns the total number of available levels.
 *
 * @return Number of levels in the internal level table.
 */
uint8_t Level_GetCount(void);

/**
 * @brief Retrieves the descriptor for the level at @p idx.
 *
 * @param idx Zero-based level index.
 * @return Pointer to the corresponding LevelDesc, or @c NULL if out of range.
 */
const LevelDesc* Level_GetDesc(uint8_t idx);

/**
 * @brief Loads a level: initialises the player and boxes, then renders the scene.
 *
 * Resets the player's position and animation state to the level's spawn data,
 * sets up box positions and cycle durations, clears the screen, and draws the
 * full initial frame (tiles, boxes, player strip).
 *
 * @param idx      Zero-based level index to load.
 * @param player   Pointer to the Player to initialise.
 * @param boxes    Array of Box instances to initialise (length ≥ @p maxBoxes).
 * @param maxBoxes Capacity of the @p boxes array.
 */
void Level_Load(uint8_t idx, struct Player* player, struct Box* boxes, uint8_t maxBoxes);

#endif /* __LEVELS_H */
