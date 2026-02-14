#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "config.h"
#include <stdbool.h>

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
} GameState;

// Game functions
void InitGame(GameState *state, i32 level);
void UpdateGame(GameState *state, f32 dt);
void DrawGame(const GameState *state, Texture2D spritesheet, Font font);
void CleanupGame(GameState *state);

#endif // GAME_H
