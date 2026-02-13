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
    int tiles[MAP_HEIGHT][MAP_WIDTH];
    TileType cell_types[MAP_HEIGHT][MAP_WIDTH];
    Vector2 spawn_points[16];
    int spawn_count;
    Vector2 base_points[16];
    int base_count;
    int width;
    int height;
} Map;

typedef struct {
    TowerType type;
    Vector2 position;
    int grid_x, grid_y;
    float fire_countdown;
    int target_enemy_id;
    float rotation;
    bool active;
} Tower;

typedef struct {
    EnemyType type;
    Vector2 position;
    float health, max_health;
    float base_speed;
    float speed_factor;
    float freeze_timer;
    int reward;
    int damage_to_base;
    float difficulty;
    Vector2 path[MAX_PATH_LEN];
    int path_len;
    int path_index;
    float shield_hp;
    bool active;
} Enemy;

typedef struct {
    BulletType type;
    Vector2 position;
    Vector2 direction;
    float speed;
    float damage;
    float max_range;
    float distance_traveled;
    int target_enemy_id;
    bool active;
} Bullet;

typedef struct {
    EnemyType enemy_type;
    int quantity;
    float difficulty;
} WaveEntry;

typedef struct {
    WaveEntry *waves;
    int *wave_sizes;
    int total_waves;
    int current_wave;
    int enemies_spawned;
    float spawn_timer;
    float countdown_timer;
    bool wave_active;
} WaveManager;

// Forward declare UIState
typedef struct UIState UIState;

typedef struct {
    GameScreen screen;
    Map map;
    Tower towers[MAX_TOWERS];
    int tower_count;
    Enemy enemies[MAX_ENEMIES];
    int enemy_count;
    Bullet bullets[MAX_BULLETS];
    int bullet_count;
    WaveManager wave_mgr;
    int currency;
    int base_life;
    int current_level;
    UIState *ui;  // Pointer to avoid circular dependency
} GameState;

// Game functions
void InitGame(GameState *state, int level);
void UpdateGame(GameState *state, float dt);
void DrawGame(const GameState *state, Texture2D spritesheet, Font font);
void CleanupGame(GameState *state);

#endif // GAME_H
