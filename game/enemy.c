#include "enemy.h"
#include "map.h"
#include "content.h"
#include "config.h"
#include <stdio.h>
#include <math.h>

// Get sprite index for enemy type
static i32 GetEnemySpriteIndex(const GameState *state, EnemyType type) {
    if (type < ENEMY_SIMPLE || type > ENEMY_BOSS) {
        return -1;
    }
    return state->enemy_config[type].sprite_id;
}

static Vector2 DirToVec(Direction dir) {
    switch (dir) {
        case DIR_NORTH:      return (Vector2){ 0.0f, -1.0f };
        case DIR_SOUTH:      return (Vector2){ 0.0f,  1.0f };
        case DIR_EAST:       return (Vector2){ 1.0f,  0.0f };
        case DIR_WEST:       return (Vector2){-1.0f,  0.0f };
        case DIR_NORTH_EAST: return (Vector2){ 0.70710678f, -0.70710678f };
        case DIR_NORTH_WEST: return (Vector2){-0.70710678f, -0.70710678f };
        case DIR_SOUTH_EAST: return (Vector2){ 0.70710678f,  0.70710678f };
        case DIR_SOUTH_WEST: return (Vector2){-0.70710678f,  0.70710678f };
        case DIR_NONE:
        default:             return (Vector2){ 0.0f,  0.0f };
    }
}

static bool IsValidDirection(Direction dir) {
    return dir >= DIR_NONE && dir <= DIR_SOUTH_WEST;
}

static bool IsNearTileCenter(Vector2 pos, i32 grid_x, i32 grid_y) {
    Vector2 center = GridToWorld(grid_x, grid_y);
    f32 half_box = TILE_SIZE * 0.25f;
    return fabsf(pos.x - center.x) <= half_box && fabsf(pos.y - center.y) <= half_box;
}

static bool CanMoveDiagonalStep(const Map *map, i32 from_x, i32 from_y, i32 to_x, i32 to_y) {
    i32 dx = to_x - from_x;
    i32 dy = to_y - from_y;

    if (dx == 0 || dy == 0) return true;

    return IsWalkable(map, from_x + dx, from_y) &&
           IsWalkable(map, from_x, from_y + dy);
}

static bool MoveFallbackToWalkableNeighbor(Enemy *enemy, const Map *map, f32 speed, f32 dt) {
    i32 grid_x, grid_y;
    WorldToGrid(enemy->position, &grid_x, &grid_y);

    const i32 dx[] = {0, 0, 1, -1, 1, -1, 1, -1};
    const i32 dy[] = {-1, 1, 0, 0, -1, -1, 1, 1};

    for (i32 k = 0; k < 8; k++) {
        i32 nx = grid_x + dx[k];
        i32 ny = grid_y + dy[k];
        if (!IsWalkable(map, nx, ny)) continue;
        if (!CanMoveDiagonalStep(map, grid_x, grid_y, nx, ny)) continue;

        Vector2 target = GridToWorld(nx, ny);
        Vector2 move = {
            target.x - enemy->position.x,
            target.y - enemy->position.y
        };
        f32 len = sqrtf(move.x * move.x + move.y * move.y);
        if (len <= 0.001f) continue;

        move.x /= len;
        move.y /= len;
        enemy->position.x += move.x * speed * dt;
        enemy->position.y += move.y * speed * dt;
        return true;
    }

    return false;
}

i32 SpawnEnemy(GameState *state, EnemyType type, Vector2 spawn_pos, f32 difficulty) {
    if (state->enemy_count >= MAX_ENEMIES) {
        printf("Cannot spawn enemy: max limit reached\n");
        return -1;
    }

    if (type < ENEMY_SIMPLE || type > ENEMY_BOSS) {
        printf("Cannot spawn enemy: invalid enemy type %d\n", type);
        return -1;
    }

    i32 index = state->enemy_count++;
    Enemy *enemy = &state->enemies[index];

    // Initialize enemy
    enemy->type = type;
    enemy->base_speed = state->enemy_config[type].stats.speed * TILE_SIZE;  // Convert to pixels/sec
    enemy->speed_factor = 1.0f;
    enemy->max_health = state->enemy_config[type].stats.health * difficulty;
    enemy->health = enemy->max_health;
    enemy->reward = state->enemy_config[type].stats.reward;
    enemy->damage_to_base = state->enemy_config[type].stats.damage_to_base;
    enemy->difficulty = difficulty;
    enemy->freeze_timer = 0;
    enemy->shield_hp = 0;
    enemy->active = true;
    enemy->flow_move_dir = (Vector2){0.0f, 0.0f};

    // Special case: shielded enemy starts with shield
    if (type == ENEMY_SHIELDED) {
        enemy->shield_hp = enemy->max_health * 0.5f;  // Shield absorbs 50% of max HP
    }

    // Position at spawn
    i32 spawn_x = (i32)spawn_pos.x;
    i32 spawn_y = (i32)spawn_pos.y;
    enemy->position = GridToWorld(spawn_x, spawn_y);

    enemy->path_len = 0;
    enemy->path_index = 0;

    if (type == ENEMY_FLYING) {
        RecalculateEnemyPath(enemy, &state->map);
    }

    return index;
}

void RecalculateEnemyPath(Enemy *enemy, const Map *map) {
    i32 grid_x, grid_y;
    WorldToGrid(enemy->position, &grid_x, &grid_y);

    if (enemy->type == ENEMY_FLYING) {
        if (map->base_count <= 0) {
            enemy->path_len = 0;
            enemy->path_index = 0;
            return;
        }
        enemy->path_len = 2;
        enemy->path[0] = GridToWorld(grid_x, grid_y);
        enemy->path[1] = GridToWorld((i32)map->base_points[0].x, (i32)map->base_points[0].y);
        enemy->path_index = 0;
        return;
    }

    enemy->path_len = 0;
    enemy->path_index = 0;
}

void UpdateEnemies(GameState *state, f32 dt) {
    for (i32 i = 0; i < state->enemy_count; i++) {
        Enemy *enemy = &state->enemies[i];
        if (!enemy->active) continue;

        if (enemy->freeze_timer > 0.0f) {
            enemy->freeze_timer -= dt;
            enemy->speed_factor = FREEZE_SLOW_FACTOR;
        } else {
            enemy->speed_factor = 1.0f;
        }

        if (enemy->type == ENEMY_FLYING) {
            if (state->map.base_count <= 0) continue;

            Vector2 base_world = GridToWorld(
                (i32)state->map.base_points[0].x,
                (i32)state->map.base_points[0].y
            );

            Vector2 dir = {
                base_world.x - enemy->position.x,
                base_world.y - enemy->position.y
            };
            f32 dist = sqrtf(dir.x * dir.x + dir.y * dir.y);

            if (dist < 4.0f) {
                state->base_life -= enemy->damage_to_base;
                printf("Enemy reached base! Base life: %d\n", state->base_life);
                RemoveEnemy(state, i);
                i--;
                continue;
            }

            dir.x /= dist;
            dir.y /= dist;

            f32 speed = enemy->base_speed * enemy->speed_factor;
            enemy->position.x += dir.x * speed * dt;
            enemy->position.y += dir.y * speed * dt;
            continue;
        }

        i32 grid_x, grid_y;
        WorldToGrid(enemy->position, &grid_x, &grid_y);

        if (GetTileType(&state->map, grid_x, grid_y) == TILE_BASE) {
            state->base_life -= enemy->damage_to_base;
            printf("Enemy reached base! Base life: %d\n", state->base_life);
            RemoveEnemy(state, i);
            i--;
            continue;
        }

        if (!state->flow_field ||
            grid_x < 0 || grid_x >= state->map.width ||
            grid_y < 0 || grid_y >= state->map.height) {
            continue;
        }

        i32 idx = grid_y * state->map.width + grid_x;
        Direction dir = state->flow_field[idx];
        if (!IsValidDirection(dir)) {
            enemy->flow_move_dir = (Vector2){0.0f, 0.0f};
            continue;
        }
        if (dir == DIR_NONE || GetTileType(&state->map, grid_x, grid_y) == TILE_BLOCKED) {
            f32 speed = enemy->base_speed * enemy->speed_factor;
            enemy->flow_move_dir = (Vector2){0.0f, 0.0f};
            (void)MoveFallbackToWalkableNeighbor(enemy, &state->map, speed, dt);
            continue;
        }

        if (IsNearTileCenter(enemy->position, grid_x, grid_y) ||
            (enemy->flow_move_dir.x == 0.0f && enemy->flow_move_dir.y == 0.0f)) {
            enemy->flow_move_dir = DirToVec(dir);
        }

        Vector2 v = enemy->flow_move_dir;
        f32 speed = enemy->base_speed * enemy->speed_factor;
        enemy->position.x += v.x * speed * dt;
        enemy->position.y += v.y * speed * dt;

        WorldToGrid(enemy->position, &grid_x, &grid_y);
        if (GetTileType(&state->map, grid_x, grid_y) == TILE_BASE) {
            state->base_life -= enemy->damage_to_base;
            printf("Enemy reached base! Base life: %d\n", state->base_life);
            RemoveEnemy(state, i);
            i--;
            continue;
        }
    }
}

void DrawEnemies(const GameState *state, Texture2D spritesheet) {
    for (i32 i = 0; i < state->enemy_count; i++) {
        const Enemy *enemy = &state->enemies[i];
        if (!enemy->active) continue;

        i32 sprite_id = GetEnemySpriteIndex(state, enemy->type);

        // Calculate source rectangle (128x128 tiles, 23 columns)
        i32 src_x = (sprite_id % SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
        i32 src_y = (sprite_id / SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
        Rectangle src = {src_x, src_y, SPRITE_TILE_SIZE, SPRITE_TILE_SIZE};

        // Destination (center the sprite on enemy position)
        Rectangle dest = {
            enemy->position.x - TILE_SIZE / 2,
            enemy->position.y - TILE_SIZE / 2,
            TILE_SIZE,
            TILE_SIZE
        };

        // Draw shadow/wings for flying enemies
        if (enemy->type == ENEMY_FLYING && state->enemy_config[enemy->type].overlay_sprite_id >= 0) {
            i32 shadow_sprite = state->enemy_config[enemy->type].overlay_sprite_id;
            i32 shadow_sx = (shadow_sprite % SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
            i32 shadow_sy = (shadow_sprite / SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
            Rectangle shadow_src = {shadow_sx, shadow_sy, SPRITE_TILE_SIZE, SPRITE_TILE_SIZE};
            DrawTexturePro(spritesheet, shadow_src, dest, (Vector2){0, 0}, 0, WHITE);
        }

        // Draw shell layer for boss enemies
        if (enemy->type == ENEMY_BOSS && state->enemy_config[enemy->type].overlay_sprite_id >= 0) {
            i32 shell_sprite = state->enemy_config[enemy->type].overlay_sprite_id;
            i32 shell_sx = (shell_sprite % SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
            i32 shell_sy = (shell_sprite / SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
            Rectangle shell_src = {shell_sx, shell_sy, SPRITE_TILE_SIZE, SPRITE_TILE_SIZE};
            DrawTexturePro(spritesheet, shell_src, dest, (Vector2){0, 0}, 0, WHITE);
        }

        // Draw main enemy sprite
        Color tint = WHITE;
        if (enemy->freeze_timer > 0) {
            tint = SKYBLUE;  // Tint frozen enemies
        }
        DrawTexturePro(spritesheet, src, dest, (Vector2){0, 0}, 0, tint);

        // Health bar
        f32 health_ratio = enemy->health / enemy->max_health;
        i32 bar_width = 48;
        i32 bar_height = 6;
        i32 bar_x = (i32)(enemy->position.x - bar_width / 2);
        i32 bar_y = (i32)(enemy->position.y - TILE_SIZE / 2 - 10);

        // Background
        DrawRectangle(bar_x, bar_y, bar_width, bar_height, RED);
        // Health
        DrawRectangle(bar_x, bar_y, (i32)(bar_width * health_ratio), bar_height, GREEN);
        // Border
        DrawRectangleLines(bar_x, bar_y, bar_width, bar_height, BLACK);

        // Shield bar (for shielded enemies)
        if (enemy->type == ENEMY_SHIELDED && enemy->shield_hp > 0) {
            f32 shield_max = enemy->max_health * 0.5f;
            f32 shield_ratio = enemy->shield_hp / shield_max;
            i32 shield_y = bar_y - bar_height - 2;
            DrawRectangle(bar_x, shield_y, bar_width, bar_height, DARKGRAY);
            DrawRectangle(bar_x, shield_y, (i32)(bar_width * shield_ratio), bar_height, SKYBLUE);
            DrawRectangleLines(bar_x, shield_y, bar_width, bar_height, BLACK);
        }
    }
}

void RemoveEnemy(GameState *state, i32 index) {
    if (index < 0 || index >= state->enemy_count) return;

    // Swap with last enemy (O(1) removal)
    state->enemies[index] = state->enemies[state->enemy_count - 1];
    state->enemy_count--;
}

void DamageEnemy(Enemy *enemy, f32 damage) {
    if (!enemy->active) return;

    // Shielded enemy: damage goes to shield first
    if (enemy->type == ENEMY_SHIELDED && enemy->shield_hp > 0) {
        f32 absorbed = damage * 0.5f;  // Shield absorbs 50% of damage
        enemy->shield_hp -= absorbed;

        if (enemy->shield_hp <= 0) {
            // Shield broken, remaining damage goes through (with 90% reduction)
            f32 overflow = -enemy->shield_hp;
            enemy->shield_hp = 0;
            enemy->health -= overflow * 0.1f;  // 90% reduction
        }
    } else {
        // Normal damage
        enemy->health -= damage;
    }

    if (enemy->health <= 0) {
        enemy->active = false;
    }
}
