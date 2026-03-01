#ifndef CONFIG_H
#define CONFIG_H

#include "base_defs.h"
#include "raylib.h"

// Window settings
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define TARGET_FPS 120

// Game constants
#define TILE_SIZE 54
#define MAX_TOWERS 128
#define MAX_ENEMIES 512
#define MAX_BULLETS 1024
#define MAX_PATH_LEN 256
#define TOWER_TYPE_COUNT 6
#define ENEMY_TYPE_COUNT 6

// Map dimensions (max, actual per-level may be smaller)
#define MAP_WIDTH 20
#define MAP_HEIGHT 12
#define MAX_LEVEL_WAVES 32
#define MAX_LEVEL_WAVE_ENTRIES 256

// Map rendering offset (for HUD at top)
#define MAP_OFFSET_X 0
#define MAP_OFFSET_Y 40

// Shop panel layout (right side of screen)
// Uses 18 tiles as the map display width for Level 1-3
#define MAP_DISPLAY_COLS 18
#define SHOP_X (MAP_DISPLAY_COLS * TILE_SIZE)
#define SHOP_WIDTH (SCREEN_WIDTH - SHOP_X)

// HUD height
#define HUD_HEIGHT 40

// Background color (beige, matching Unity version)
#define BG_COLOR (Color){220, 209, 180, 255}

// Balance values
#define STARTING_CURRENCY 50
#define STARTING_BASE_LIFE 20
#define WAVE_COUNTDOWN_SECONDS 20.0f
#define ENEMY_SPAWN_DELAY 0.05f

// Data-driven stat types (loaded from turrets/*.conf and enemies/*.conf)
typedef struct {
    i32 damage;
    f32 fire_rate;      // shots per second
    f32 range;          // in tiles
    i32 cost;
} TowerStats;

typedef struct {
    i32 health;
    f32 speed;
    i32 reward;
    i32 damage_to_base;
} EnemyStats;

// Spritesheet layout (@2 retina: 128x128 tiles, 23 columns x 13 rows)
#define SPRITE_TILE_SIZE 128
#define SPRITE_SHEET_COLS 23

// Helper: Kenney tile number maps directly to spritesheet index
// (index 0 in the sheet is unused; tile001 is at index 1, tile002 at index 2, etc.)
#define TILE_NUM(n) (n)

// Map tile sprite indices (Kenney tile numbers)
#define SPRITE_TREE        TILE_NUM(130)   // Green tree/bush (border)
#define SPRITE_GRASS       TILE_NUM(24)    // Green flat grass
#define SPRITE_GROUND      TILE_NUM(103)   // Gray ground (buildable play area)
#define SPRITE_GROUND_ALT  TILE_NUM(172)   // Gray ground variant
#define SPRITE_ROCK        TILE_NUM(137)   // Pentagon rock (left/right border)
#define SPRITE_ROCK_ALT    TILE_NUM(135)   // Rock variant
#define SPRITE_SAND        TILE_NUM(188)   // Beige/tan background
#define SPRITE_PATH        TILE_NUM(172)   // Dirt path (walkable)

// Tower base sprites (non-rotating)
#define SPRITE_TOWER_BASE_VULCAN  TILE_NUM(180)
#define SPRITE_TOWER_BASE_DCA     TILE_NUM(181)
#define SPRITE_TOWER_BASE_FREEZE  TILE_NUM(181)
#define SPRITE_TOWER_BASE_MISSILE TILE_NUM(182)
#define SPRITE_TOWER_BASE_PLASMA  TILE_NUM(183)
#define SPRITE_TOWER_BASE_WALL    TILE_NUM(180)

// Tower gun sprites (rotating)
#define SPRITE_TOWER_GUN_VULCAN   TILE_NUM(203)
#define SPRITE_TOWER_GUN_DCA      TILE_NUM(205)
#define SPRITE_TOWER_GUN_FREEZE   TILE_NUM(22)
#define SPRITE_TOWER_GUN_MISSILE  TILE_NUM(226)
#define SPRITE_TOWER_GUN_PLASMA   TILE_NUM(206)

#define SHOP_TOWER_COUNT 6

// Bullet constants
#define BULLET_SPEED 10.0f
#define MISSILE_SPLASH_RADIUS 0.5f
#define FREEZE_SLOW_FACTOR 0.5f
#define FREEZE_DURATION 1.0f
#define DCA_BURST_COUNT 4
#define DCA_BURST_DELAY 0.1f

// Tower rotation speed
#define TOWER_ROTATION_SPEED 400.0f  // degrees per second

#endif // CONFIG_H
