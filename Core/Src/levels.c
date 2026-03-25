/**
 * @file levels.c
 * @brief Level data definitions and level management functions for Sokoban.
 *
 * Contains the static level descriptors (tile maps, spawn points, box
 * positions) and implements the runtime functions that load a level into the
 * game, scan for box-spot positions, and check the win condition.
 */

#include "levels.h"
#include "box.h"
#include "player.h"
#include "renderer.h"
#include "st7789.h"

/**
 * @brief Static descriptor for level1.
 *
 * Auto-generated from: Level_1.ldtkl
 * Grid: 15x15 tiles  (16px each)
 * Player spawns at tile (3, 6).
 * Box count: 2
 */
const LevelDesc level1Desc = {
    .tiles = {
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     BOX_SPOT,  FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     BOX_SPOT,  FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, }
    },
    .spawnX   = 3,
    .spawnY   = 6,
    .boxCount = 2,
    .boxX = { 4, 5 },
    .boxY = { 6, 7 },
};

/**
 * @brief Static descriptor for level2.
 *
 * Auto-generated from: Level_2.ldtkl
 * Grid: 15x15 tiles  (16px each)
 * Player spawns at tile (7, 7).
 * Box count: 4
 */
const LevelDesc level2Desc = {
    .tiles = {
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_SIDE, VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, BOX_SPOT,  WALL_SIDE, VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     WALL_SIDE, VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_TOP,  FLOOR,     WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_SIDE, VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      WALL_SIDE, BOX_SPOT,  FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     BOX_SPOT,  WALL_SIDE, VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_SIDE, FLOOR,     WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_TOP,  VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     WALL_SIDE, VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, BOX_SPOT,  WALL_SIDE, VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      WALL_TOP,  WALL_TOP,  WALL_TOP,  VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, }
    },
    .spawnX   = 7,
    .spawnY   = 7,
    .boxCount = 4,
    .boxX = { 7, 6, 8, 7 },
    .boxY = { 6, 7, 7, 8 },
};

/**
 * @brief Static descriptor for level3.
 *
 * Auto-generated from: Level_3.ldtkl
 * Grid: 15x15 tiles  (16px each)
 * Player spawns at tile (6, 3).
 * Box count: 4
 */
const LevelDesc level3Desc = {
    .tiles = {
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_SIDE, VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     WALL_SIDE, FLOOR,     WALL_SIDE, VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     WALL_SIDE, FLOOR,     WALL_SIDE, VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     WALL_SIDE, FLOOR,     WALL_SIDE, VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     WALL_SIDE, WALL_TOP,  WALL_TOP,  VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, BOX_SPOT,  BOX_SPOT,  WALL_SIDE, VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, BOX_SPOT,  BOX_SPOT,  WALL_SIDE, VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, }
    },
    .spawnX   = 6,
    .spawnY   = 3,
    .boxCount = 4,
    .boxX = { 6, 7, 6, 7 },
    .boxY = { 5, 6, 7, 8 },
};

/**
 * @brief Static descriptor for level4.
 *
 * Auto-generated from: Level_4.ldtkl
 * Grid: 15x15 tiles  (16px each)
 * Player spawns at tile (7, 11).
 * Box count: 3
 */
const LevelDesc level4Desc = {
    .tiles = {
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_SIDE, VOID, },
        { VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     BOX_SPOT,  FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID, },
        { VOID,      WALL_SIDE, FLOOR,     WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_SIDE, FLOOR,     WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_SIDE, FLOOR,     WALL_SIDE, VOID, },
        { VOID,      WALL_SIDE, FLOOR,     WALL_SIDE, VOID,      VOID,      WALL_SIDE, FLOOR,     WALL_TOP,  VOID,      VOID,      WALL_SIDE, FLOOR,     WALL_SIDE, VOID, },
        { VOID,      WALL_SIDE, FLOOR,     WALL_SIDE, VOID,      WALL_SIDE, WALL_TOP,  FLOOR,     WALL_TOP,  WALL_SIDE, VOID,      WALL_SIDE, FLOOR,     WALL_SIDE, VOID, },
        { VOID,      WALL_SIDE, FLOOR,     WALL_TOP,  WALL_TOP,  WALL_TOP,  FLOOR,     FLOOR,     FLOOR,     WALL_TOP,  WALL_TOP,  WALL_TOP,  FLOOR,     WALL_SIDE, VOID, },
        { VOID,      WALL_SIDE, BOX_SPOT,  FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     BOX_SPOT,  WALL_SIDE, VOID, },
        { VOID,      WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      WALL_TOP,  WALL_SIDE, FLOOR,     WALL_SIDE, WALL_TOP,  VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     WALL_SIDE, VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     WALL_SIDE, VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      WALL_TOP,  WALL_TOP,  WALL_TOP,  VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, }
    },
    .spawnX   = 7,
    .spawnY   = 11,
    .boxCount = 3,
    .boxX = { 2, 12, 7 },
    .boxY = { 3, 3, 7 },
};

/**
 * @brief Static descriptor for level5.
 *
 * Auto-generated from: Level_5.ldtkl
 * Grid: 15x15 tiles  (16px each)
 * Player spawns at tile (10, 5).
 * Box count: 8
 */
const LevelDesc level5Desc = {
    .tiles = {
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_SIDE, VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_TOP,  BOX_SPOT,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_SIDE, VOID,      VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, WALL_TOP,  FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_TOP,  WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, BOX_SPOT,  FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     BOX_SPOT,  WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, BOX_SPOT,  FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     BOX_SPOT,  WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, BOX_SPOT,  FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     BOX_SPOT,  WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_TOP,  WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, WALL_TOP,  VOID,      VOID, },
        { VOID,      VOID,      VOID,      WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_SIDE, BOX_SPOT,  WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_TOP,  VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      WALL_TOP,  WALL_TOP,  WALL_TOP,  VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, }
    },
    .spawnX   = 10,
    .spawnY   = 5,
    .boxCount = 8,
    .boxX = { 5, 7, 9, 6, 8, 5, 7, 9 },
    .boxY = { 6, 6, 6, 7, 7, 8, 8, 8 },
};

/**
 * @brief Static descriptor for level6.
 *
 * Auto-generated from: Level_6.ldtkl
 * Grid: 15x15 tiles  (16px each)
 * Player spawns at tile (9, 3).
 * Box count: 4
 */
const LevelDesc level6Desc = {
    .tiles = {
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_SIDE, VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  FLOOR,     WALL_SIDE, WALL_SIDE, FLOOR,     WALL_SIDE, VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_TOP,  FLOOR,     WALL_SIDE, VOID,      VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  FLOOR,     BOX_SPOT,  BOX_SPOT,  FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     BOX_SPOT,  BOX_SPOT,  FLOOR,     WALL_SIDE, WALL_TOP,  WALL_TOP,  VOID,      VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     WALL_TOP,  WALL_TOP,  FLOOR,     WALL_SIDE, WALL_TOP,  WALL_TOP,  VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_TOP,  VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, }
    },
    .spawnX   = 9,
    .spawnY   = 3,
    .boxCount = 4,
    .boxX = { 7, 8, 5, 6 },
    .boxY = { 5, 6, 7, 8 },
};

/**
 * @brief Static descriptor for level7.
 *
 * Auto-generated from: Level_7.ldtkl
 * Grid: 15x15 tiles  (16px each)
 * Player spawns at tile (3, 9).
 * Box count: 4
 */
const LevelDesc level7Desc = {
    .tiles = {
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_SIDE, VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, BOX_SPOT,  WALL_SIDE, VOID,      VOID,      VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_SIDE, VOID, },
        { VOID,      VOID,      WALL_SIDE, BOX_SPOT,  WALL_SIDE, VOID,      VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID, },
        { VOID,      VOID,      WALL_SIDE, BOX_SPOT,  WALL_SIDE, VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_TOP,  FLOOR,     WALL_SIDE, FLOOR,     WALL_SIDE, VOID, },
        { VOID,      WALL_SIDE, WALL_TOP,  BOX_SPOT,  WALL_TOP,  WALL_TOP,  WALL_TOP,  FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID, },
        { VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID, },
        { VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID, },
        { VOID,      WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, }
    },
    .spawnX   = 3,
    .spawnY   = 9,
    .boxCount = 4,
    .boxX = { 8, 9, 10, 11 },
    .boxY = { 8, 8, 8, 8 },
};

/**
 * @brief Static descriptor for level8.
 *
 * Auto-generated from: Level_8.ldtkl
 * Grid: 15x15 tiles  (16px each)
 * Player spawns at tile (7, 7).
 * Box count: 4
 */
const LevelDesc level8Desc = {
    .tiles = {
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_SIDE, VOID,      VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     WALL_SIDE, WALL_TOP,  FLOOR,     WALL_TOP,  WALL_SIDE, FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     WALL_TOP,  BOX_SPOT,  FLOOR,     BOX_SPOT,  WALL_TOP,  FLOOR,     WALL_SIDE, WALL_TOP,  VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     BOX_SPOT,  FLOOR,     BOX_SPOT,  WALL_SIDE, FLOOR,     WALL_TOP,  WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     WALL_TOP,  WALL_TOP,  FLOOR,     WALL_TOP,  WALL_TOP,  FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_SIDE, FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  VOID,      VOID,      VOID,      WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, }
    },
    .spawnX   = 7,
    .spawnY   = 7,
    .boxCount = 4,
    .boxX = { 4, 10, 4, 10 },
    .boxY = { 4, 4, 10, 10 },
};

/**
 * @brief Static descriptor for level9.
 *
 * Auto-generated from: Level_9.ldtkl
 * Grid: 15x15 tiles  (16px each)
 * Player spawns at tile (6, 11).
 * Box count: 4
 */
const LevelDesc level9Desc = {
    .tiles = {
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_SIDE, VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  FLOOR,     WALL_SIDE, FLOOR,     WALL_SIDE, VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, FLOOR,     WALL_SIDE, VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      WALL_SIDE, WALL_TOP,  FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, FLOOR,     WALL_TOP,  WALL_TOP,  WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  FLOOR,     WALL_SIDE, WALL_TOP,  WALL_SIDE, FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      WALL_SIDE, BOX_SPOT,  WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, BOX_SPOT,  WALL_SIDE, FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      WALL_SIDE, BOX_SPOT,  WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_TOP,  BOX_SPOT,  WALL_SIDE, FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_TOP,  FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      WALL_SIDE, FLOOR,     FLOOR,     WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID, },
        { VOID,      WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  VOID,      VOID,      WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, }
    },
    .spawnX   = 6,
    .spawnY   = 11,
    .boxCount = 4,
    .boxX = { 7, 5, 7, 10 },
    .boxY = { 3, 6, 7, 7 },
};

/**
 * @brief Static descriptor for level10.
 *
 * Auto-generated from: Level_10.ldtkl
 * Grid: 15x15 tiles  (16px each)
 * Player spawns at tile (4, 5).
 * Box count: 4
 */
const LevelDesc level10Desc = {
    .tiles = {
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      WALL_SIDE, WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_SIDE, VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, BOX_SPOT,  FLOOR,     BOX_SPOT,  WALL_SIDE, VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      WALL_SIDE, FLOOR,     FLOOR,     FLOOR,     WALL_SIDE, FLOOR,     BOX_SPOT,  BOX_SPOT,  WALL_SIDE, VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  WALL_TOP,  VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, },
        { VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID,      VOID, }
    },
    .spawnX   = 4,
    .spawnY   = 5,
    .boxCount = 4,
    .boxX = { 9, 5, 6, 8 },
    .boxY = { 5, 6, 6, 7 },
};

/** @brief Number of box-spots cached from the last loaded level. */
uint8_t currentSpotCount = 0;

/** @brief Cached tile columns of each box-spot in the current level. */
uint8_t currentSpotX[BOX_MAX];

/** @brief Cached tile rows of each box-spot in the current level. */
uint8_t currentSpotY[BOX_MAX];

/** @brief Internal table mapping zero-based indices to level descriptors. */
static const LevelDesc* levels[] = {
    &level1Desc, &level2Desc, &level3Desc, &level4Desc, &level5Desc,
    &level6Desc, &level7Desc, &level8Desc, &level9Desc, &level10Desc
};

/**
 * @brief Returns the total number of levels in the internal level table.
 *
 * @return Number of available levels.
 */
uint8_t Level_GetCount(void) {
    return sizeof(levels) / sizeof(levels[0]);
}

/**
 * @brief Retrieves the descriptor for the level at @p idx.
 *
 * @param idx Zero-based level index.
 * @return Pointer to the LevelDesc, or @c NULL if @p idx is out of range.
 */
const LevelDesc* Level_GetDesc(uint8_t idx) {
    if (idx >= Level_GetCount()) return 0;
    return levels[idx];
}

/**
 * @brief Loads a level: resets player & boxes, fills the screen, and renders.
 *
 * Steps performed:
 *  1. Bounds-checks @p idx.
 *  2. Resets the player's position, tile coordinates, and animation state to
 *     the level's spawn data.  Movement/frame durations are set to 500 ms / 4
 *     frames and 500 ms respectively.
 *  3. Activates boxes according to @c desc->boxCount; inactive slots are
 *     left dormant.  Each active box inherits @c cycleDuration from the player.
 *  4. Clears the display, draws the tile map, draws all boxes, draws the
 *     player's initial strip, and scans for box-spots via Level_LoadSpots().
 *
 * @param idx      Zero-based level index to load.
 * @param player   Pointer to the Player to initialise.
 * @param boxes    Box array to initialise (must have length ≥ @p maxBoxes).
 * @param maxBoxes Capacity of @p boxes.
 */
void Level_Load(uint8_t idx, Player* player, struct Box* boxes, uint8_t maxBoxes) {
    if (idx >= Level_GetCount()) return;
    const LevelDesc* desc = levels[idx];

    player->x = player->prevX = desc->spawnX * 16;
    player->y = player->prevY = desc->spawnY * 16;
    player->tileX = player->targetTileX = desc->spawnX;
    player->tileY = player->targetTileY = desc->spawnY;
    player->state         = ANIM_IDLE_DOWN;
    player->frame         = 0;
    player->animAccumulator = 0;
    player->moveAccumulator = 0;
    player->frameDuration = 500000U / 4U;
    player->cycleDuration = 500000U;
    player->frameChanged  = true;
    player->posChanged    = false;
    player->pushedBoxIdx  = -1;

    for (uint8_t i = 0; i < maxBoxes; i++) {
        boxes[i].active = (i < desc->boxCount);
        if (!boxes[i].active) continue;
        boxes[i].tileX = boxes[i].targetTileX = desc->boxX[i];
        boxes[i].tileY = boxes[i].targetTileY = desc->boxY[i];
        boxes[i].x     = desc->boxX[i] * 16;
        boxes[i].y     = desc->boxY[i] * 16;
        boxes[i].moveAccumulator = 0;
        boxes[i].cycleDuration   = player->cycleDuration;
        boxes[i].posChanged      = false;
    }

    ST7789_Fill_Color(0x2200);
    Renderer_DrawLevel(desc->tiles);
    Renderer_DrawBoxes(desc->tiles, boxes, maxBoxes);
    Renderer_DrawPlayerStrip(desc->tiles, player, boxes, maxBoxes);

    Level_LoadSpots(desc->tiles);
}

/**
 * @brief Scans the tile map and caches all BOX_SPOT positions.
 *
 * Iterates the full 15×15 grid row-by-row, recording the column and row of
 * every BOX_SPOT tile into @c currentSpotX / @c currentSpotY up to @c BOX_MAX
 * entries.  @c currentSpotCount is reset at the start of each call.
 *
 * @param tiles The 15×15 tile map to scan (row-major: [row][col]).
 */
void Level_LoadSpots(const Level tiles[15][15]) {
    currentSpotCount = 0;
    for (uint8_t r = 0; r < 15; r++) {
        for (uint8_t c = 0; c < 15; c++) {
            if (tiles[r][c] == BOX_SPOT) {
                if (currentSpotCount < BOX_MAX) {
                    currentSpotX[currentSpotCount] = c;
                    currentSpotY[currentSpotCount] = r;
                    currentSpotCount++;
                }
            }
        }
    }
}

/**
 * @brief Checks whether every box-spot is covered by an active box.
 *
 * First counts active boxes and compares against @c currentSpotCount to
 * reject quickly when counts differ.  Then verifies each cached spot via
 * Box_At() to ensure a box is present at that position.
 *
 * @param boxes    Array of boxes in the current level.
 * @param boxCount Number of entries in @p boxes.
 * @return @c true if all spots are covered, @c false otherwise.
 */
bool Level_IsComplete(const struct Box *boxes, uint8_t boxCount) {
    uint8_t activeBoxes = 0;
    for(uint8_t i = 0; i < boxCount; i++) {
        if (boxes[i].active) activeBoxes++;
    }

    if (currentSpotCount != activeBoxes) return false;

    for (uint8_t i = 0; i < currentSpotCount; i++) {
        if (Box_At((const Box *)boxes, boxCount, currentSpotX[i], currentSpotY[i]) < 0) {
            return false;
        }
    }
    return true;
}
