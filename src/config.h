#ifndef CONFIG_H
#define CONFIG_H

// Window settings
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define TARGET_FPS 60

// Game constants
#define TILE_SIZE 54
#define MAX_TOWERS 128
#define MAX_ENEMIES 512
#define MAX_BULLETS 1024
#define MAX_PATH_LEN 256

// Map dimensions (max, actual per-level may be smaller)
#define MAP_WIDTH 20
#define MAP_HEIGHT 12

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

// Tower stats (matching Unity Level 1 values)
typedef struct {
    int damage;
    float fire_rate;      // shots per second
    float range;          // in tiles
    int cost;
} TowerStats;

static const TowerStats TOWER_STATS[] = {
    [0] = { .damage = 10,  .fire_rate = 1.0f, .range = 2.0f, .cost = 5  },  // Vulcan
    [1] = { .damage = 20,  .fire_rate = 1.0f, .range = 2.0f, .cost = 25 },  // DCA
    [2] = { .damage = 0,   .fire_rate = 0.5f, .range = 1.5f, .cost = 30 },  // Freeze
    [3] = { .damage = 7,   .fire_rate = 1.0f, .range = 4.0f, .cost = 20 },  // Missile
    [4] = { .damage = 5,   .fire_rate = 4.0f, .range = 2.5f, .cost = 15 },  // Plasma
    [5] = { .damage = 0,   .fire_rate = 0.0f, .range = 0.0f, .cost = 2  },  // Wall
};

// Enemy stats (matching Unity values)
typedef struct {
    int health;
    float speed;
    int reward;
    int damage_to_base;
} EnemyStats;

static const EnemyStats ENEMY_STATS[] = {
    [0] = { .health = 20,   .speed = 1.0f,  .reward = 1,  .damage_to_base = 1 },  // Simple
    [1] = { .health = 35,   .speed = 2.0f,  .reward = 1,  .damage_to_base = 1 },  // Fast
    [2] = { .health = 100,  .speed = 0.5f,  .reward = 1,  .damage_to_base = 1 },  // Heavy
    [3] = { .health = 40,   .speed = 1.0f,  .reward = 1,  .damage_to_base = 1 },  // Shielded
    [4] = { .health = 100,  .speed = 0.9f,  .reward = 1,  .damage_to_base = 1 },  // Flying
    [5] = { .health = 5000, .speed = 0.75f, .reward = 10, .damage_to_base = 5 },  // Boss
};

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

// Enemy sprites
#define SPRITE_ENEMY_SIMPLE       TILE_NUM(245)
#define SPRITE_ENEMY_FAST         TILE_NUM(247)
#define SPRITE_ENEMY_HEAVY        TILE_NUM(246)
#define SPRITE_ENEMY_SHIELDED     TILE_NUM(248)
#define SPRITE_ENEMY_FLYING       TILE_NUM(271)
#define SPRITE_ENEMY_FLYING_SHADOW TILE_NUM(294)
#define SPRITE_ENEMY_BOSS         TILE_NUM(269)
#define SPRITE_ENEMY_BOSS_SHELL   TILE_NUM(292)

// Shop tower display order (matching Unity shop layout)
// Wall, Vulcan, Plasma, Missile, DCA, Freeze
#define SHOP_TOWER_COUNT 6
static const int SHOP_TOWER_ORDER[] = {5, 0, 4, 3, 1, 2};
static const char *SHOP_TOWER_NAMES[] = {"Wall", "Vulcan", "Plasma", "Missile", "DCA", "Freeze"};

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
