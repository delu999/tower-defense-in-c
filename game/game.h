#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "config.h"
#include <stdbool.h>

#define CONFIG_NAME_LEN 32
#define LEVEL_NAME_LEN 64
#define CONFIG_PATH_LEN 256

// Enums
typedef enum {
    SCREEN_MENU,
    SCREEN_PLAYING,
    SCREEN_GAME_OVER,
    SCREEN_VICTORY
} GameScreen;

typedef enum {
    TILE_GROUND,
    TILE_BUILDABLE,
    TILE_BLOCKED,
    TILE_SPAWN,
    TILE_BASE
} TileType;

typedef enum {
    TOWER_VULCAN = 0,
    TOWER_DCA = 1,
    TOWER_FREEZE = 2,
    TOWER_MISSILE = 3,
    TOWER_PLASMA = 4,
    TOWER_WALL = 5
} TowerType;

typedef enum {
    ENEMY_SIMPLE = 0,
    ENEMY_FAST = 1,
    ENEMY_HEAVY = 2,
    ENEMY_SHIELDED = 3,
    ENEMY_FLYING = 4,
    ENEMY_BOSS = 5
} EnemyType;

typedef enum {
    BULLET_STANDARD,
    BULLET_MISSILE,
    BULLET_PLASMA
} BulletType;

typedef enum {
    DIR_NONE = 0,
    DIR_NORTH,
    DIR_SOUTH,
    DIR_EAST,
    DIR_WEST,
    DIR_NORTH_EAST,
    DIR_NORTH_WEST,
    DIR_SOUTH_EAST,
    DIR_SOUTH_WEST
} Direction;

// Structs
typedef struct {
    i32 tiles[MAP_HEIGHT][MAP_WIDTH];
    TileType cell_types[MAP_HEIGHT][MAP_WIDTH];
    Vector2 spawn_points[16];
    i32 spawn_count;
    Vector2 base_points[16];
    i32 base_count;
    i32 width;
    i32 height;
} Map;

typedef struct {
    TowerType type;
    Vector2 position;
    i32 grid_x, grid_y;
    f32 fire_countdown;
    i32 target_enemy_id;
    f32 rotation;
    bool active;
} Tower;

typedef struct {
    EnemyType type;
    Vector2 position;
    Vector2 flow_move_dir;
    f32 health, max_health;
    f32 base_speed;
    f32 speed_factor;
    f32 freeze_timer;
    i32 reward;
    i32 damage_to_base;
    f32 difficulty;
    Vector2 path[MAX_PATH_LEN];
    i32 path_len;
    i32 path_index;
    f32 shield_hp;
    bool active;
} Enemy;

typedef struct {
    BulletType type;
    Vector2 position;
    Vector2 direction;
    f32 speed;
    f32 damage;
    f32 max_range;
    f32 distance_traveled;
    i32 target_enemy_id;
    bool active;
} Bullet;

typedef struct {
    EnemyType enemy_type;
    i32 quantity;
    f32 difficulty;
} WaveEntry;

typedef struct {
    char name[CONFIG_NAME_LEN];
    EnemyStats stats;
    i32 sprite_id;
    i32 overlay_sprite_id;
} EnemyConfigEntry;

typedef struct {
    char name[CONFIG_NAME_LEN];
    TowerStats stats;
    i32 base_sprite_id;
    i32 gun_sprite_id;
} TowerConfigEntry;

typedef struct {
    WaveEntry *waves;
    i32 *wave_sizes;
    i32 total_waves;
    i32 current_wave;
    i32 enemies_spawned;
    f32 spawn_timer;
    f32 countdown_timer;
    bool wave_active;
} WaveManager;

// Forward declare UIState
typedef struct UIState UIState;

typedef struct {
    GameScreen screen;
    Map map;
    Tower towers[MAX_TOWERS];
    i32 tower_count;
    Enemy enemies[MAX_ENEMIES];
    i32 enemy_count;
    Bullet bullets[MAX_BULLETS];
    i32 bullet_count;
    WaveManager wave_mgr;
    i32 currency;
    i32 base_life;
    i32 current_level;
    UIState *ui;  // Pointer to avoid circular dependency
    Direction *flow_field;
    bool show_flow_field;
    bool paused;
    char level_name[LEVEL_NAME_LEN];
    char level_filename[CONFIG_PATH_LEN];
    char enemy_config_file[CONFIG_PATH_LEN];
    char turret_config_file[CONFIG_PATH_LEN];
    EnemyConfigEntry enemy_config[ENEMY_TYPE_COUNT];
    TowerConfigEntry tower_config[TOWER_TYPE_COUNT];
    i32 shop_tower_order[SHOP_TOWER_COUNT];
    i32 level_wave_sizes[MAX_LEVEL_WAVES];
    WaveEntry level_wave_entries[MAX_LEVEL_WAVE_ENTRIES];
    i32 level_total_waves;
    i32 level_wave_entry_count;
} GameState;

// Game functions
void InitGame(GameState *state, i32 level);
void UpdateGame(GameState *state, f32 dt);
void DrawGame(const GameState *state, Texture2D spritesheet, Font font);
void CleanupGame(GameState *state);

#endif // GAME_H
