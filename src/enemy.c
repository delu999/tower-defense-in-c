#include "enemy.h"
#include "map.h"
#include "pathfinding.h"
#include "config.h"
#include <stdio.h>
#include <math.h>

// Get sprite index for enemy type
static i32 GetEnemySpriteIndex(EnemyType type) {
    switch (type) {
        case ENEMY_SIMPLE:   return SPRITE_ENEMY_SIMPLE;
        case ENEMY_FAST:     return SPRITE_ENEMY_FAST;
        case ENEMY_HEAVY:    return SPRITE_ENEMY_HEAVY;
        case ENEMY_SHIELDED: return SPRITE_ENEMY_SHIELDED;
        case ENEMY_FLYING:   return SPRITE_ENEMY_FLYING;
        case ENEMY_BOSS:     return SPRITE_ENEMY_BOSS;
        default:             return SPRITE_ENEMY_SIMPLE;
    }
}

i32 SpawnEnemy(GameState *state, EnemyType type, Vector2 spawn_pos, f32 difficulty) {
    if (state->enemy_count >= MAX_ENEMIES) {
        printf("Cannot spawn enemy: max limit reached\n");
        return -1;
    }

    i32 index = state->enemy_count++;
    Enemy *enemy = &state->enemies[index];

    // Initialize enemy
    enemy->type = type;
    enemy->base_speed = ENEMY_STATS[type].speed * TILE_SIZE;  // Convert to pixels/sec
    enemy->speed_factor = 1.0f;
    enemy->max_health = ENEMY_STATS[type].health * difficulty;
    enemy->health = enemy->max_health;
    enemy->reward = ENEMY_STATS[type].reward;
    enemy->damage_to_base = ENEMY_STATS[type].damage_to_base;
    enemy->difficulty = difficulty;
    enemy->freeze_timer = 0;
    enemy->shield_hp = 0;
    enemy->active = true;

    // Special case: shielded enemy starts with shield
    if (type == ENEMY_SHIELDED) {
        enemy->shield_hp = enemy->max_health * 0.5f;  // Shield absorbs 50% of max HP
    }

    // Position at spawn
    i32 spawn_x = (i32)spawn_pos.x;
    i32 spawn_y = (i32)spawn_pos.y;
    enemy->position = GridToWorld(spawn_x, spawn_y);

    // Calculate path to base
    RecalculateEnemyPath(enemy, &state->map);

    if (enemy->path_len == 0) {
        printf("Warning: spawned enemy has no path to base!\n");
    }

    return index;
}

void RecalculateEnemyPath(Enemy *enemy, const Map *map) {
    i32 grid_x, grid_y;
    WorldToGrid(enemy->position, &grid_x, &grid_y);
    Vector2 grid_pos = {grid_x, grid_y};

    // Flying enemies take direct path (ignoring obstacles)
    if (enemy->type == ENEMY_FLYING) {
        enemy->path_len = 2;
        enemy->path[0] = GridToWorld(grid_x, grid_y);
        enemy->path[1] = GridToWorld((i32) map->base_points[0].x, (i32) grid_y);  // Fly directly to first base
        enemy->path_index = 0;
        printf("flying enemy: path length: %d, path: (%d, %d) -> (%d, %d)\n", enemy->path_len, (int)enemy->path[0].x, (int)enemy->path[0].y, (int)enemy->path[1].x, (int)enemy->path[1].y);
        return;
    }

    // Regular enemies use A* pathfinding
    Vector2 path_grid[MAX_PATH_LEN];
    i32 len = FindPath(map, grid_pos, map->base_points, map->base_count,
                       path_grid, MAX_PATH_LEN);

    if (len > 0) {
        enemy->path_len = len;
        for (i32 i = 0; i < len; i++) {
            enemy->path[i] = GridToWorld((i32)path_grid[i].x, (i32)path_grid[i].y);
        }
        enemy->path_index = 0;
    } else {
        enemy->path_len = 0;
        enemy->path_index = 0;
    }
}

void UpdateEnemies(GameState *state, f32 dt) {
    for (i32 i = 0; i < state->enemy_count; i++) {
        Enemy *enemy = &state->enemies[i];
        if (!enemy->active) continue;

        // Update freeze timer
        if (enemy->freeze_timer > 0) {
            enemy->freeze_timer -= dt;
            enemy->speed_factor = FREEZE_SLOW_FACTOR;
        } else {
            enemy->speed_factor = 1.0f;
        }

        // Movement
        if (enemy->path_index < enemy->path_len) {
            Vector2 target = enemy->path[enemy->path_index];
            Vector2 direction = {
                target.x - enemy->position.x,
                target.y - enemy->position.y
            };
            f32 dist = sqrtf(direction.x * direction.x + direction.y * direction.y);

            if (dist < 2.0f) {  // Reached waypoint
                enemy->path_index++;
            } else {
                // Move toward waypoint
                direction.x /= dist;
                direction.y /= dist;
                f32 speed = enemy->base_speed * enemy->speed_factor;
                enemy->position.x += direction.x * speed * dt;
                enemy->position.y += direction.y * speed * dt;
            }
        } else {
            // Reached base
            state->base_life -= enemy->damage_to_base;
            printf("Enemy reached base! Base life: %d\n", state->base_life);

            // Deactivate enemy
            RemoveEnemy(state, i);
            i--;  // Adjust index after removal
        }
    }
}

void DrawEnemies(const GameState *state, Texture2D spritesheet) {
    for (i32 i = 0; i < state->enemy_count; i++) {
        const Enemy *enemy = &state->enemies[i];
        if (!enemy->active) continue;

        i32 sprite_id = GetEnemySpriteIndex(enemy->type);

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
        if (enemy->type == ENEMY_FLYING) {
            i32 shadow_sx = (SPRITE_ENEMY_FLYING_SHADOW % SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
            i32 shadow_sy = (SPRITE_ENEMY_FLYING_SHADOW / SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
            Rectangle shadow_src = {shadow_sx, shadow_sy, SPRITE_TILE_SIZE, SPRITE_TILE_SIZE};
            DrawTexturePro(spritesheet, shadow_src, dest, (Vector2){0, 0}, 0, WHITE);
        }

        // Draw shell layer for boss enemies
        if (enemy->type == ENEMY_BOSS) {
            i32 shell_sx = (SPRITE_ENEMY_BOSS_SHELL % SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
            i32 shell_sy = (SPRITE_ENEMY_BOSS_SHELL / SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
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
