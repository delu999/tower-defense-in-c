#include "tower.h"
#include "map.h"
#include "bullet.h"
#include "enemy.h"
#include "pathfinding.h"
#include "content.h"
#include "config.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Get base sprite index for tower type (non-rotating platform)
static i32 GetTowerBaseSpriteIndex(const GameState *state, TowerType type) {
    if (type < TOWER_VULCAN || type > TOWER_WALL) {
        return -1;
    }
    return state->tower_config[type].base_sprite_id;
}

// Get gun sprite index for tower type (rotating turret)
static i32 GetTowerGunSpriteIndex(const GameState *state, TowerType type) {
    if (type < TOWER_VULCAN || type > TOWER_WALL) {
        return -1;
    }
    return state->tower_config[type].gun_sprite_id;
}

static bool IsReachableCell(const Map *map, const bool *reachable, i32 grid_x, i32 grid_y) {
    if (grid_x < 0 || grid_x >= map->width || grid_y < 0 || grid_y >= map->height) {
        return false;
    }
    i32 idx = grid_y * map->width + grid_x;
    return reachable[idx];
}

static bool ValidatePlacementReachability(const GameState *state, const bool *reachable) {
    const Map *map = &state->map;

    for (i32 i = 0; i < map->spawn_count; i++) {
        i32 spawn_x = (i32)map->spawn_points[i].x;
        i32 spawn_y = (i32)map->spawn_points[i].y;
        if (!IsReachableCell(map, reachable, spawn_x, spawn_y)) {
            printf("Cannot place tower: spawn %d cannot reach any base\n", i);
            return false;
        }
    }

    for (i32 i = 0; i < state->enemy_count; i++) {
        const Enemy *enemy = &state->enemies[i];
        if (!enemy->active || enemy->type == ENEMY_FLYING) continue;

        i32 enemy_grid_x, enemy_grid_y;
        WorldToGrid(enemy->position, &enemy_grid_x, &enemy_grid_y);
        if (!IsReachableCell(map, reachable, enemy_grid_x, enemy_grid_y)) {
            printf("Cannot place tower: enemy %d would be trapped\n", i);
            return false;
        }
    }

    return true;
}

i32 PlaceTower(GameState *state, TowerType type, i32 grid_x, i32 grid_y) {
    // Check affordability
    if (state->currency < state->tower_config[type].stats.cost) {
        printf("Not enough currency to place tower\n");
        return -1;
    }

    // Check if tile is buildable
    if (!IsBuildable(&state->map, grid_x, grid_y)) {
        printf("Cannot place tower here\n");
        return -1;
    }

    // Check max towers
    if (state->tower_count >= MAX_TOWERS) {
        printf("Max tower limit reached\n");
        return -1;
    }

    // Prevent placing a tower on top of active enemies.
    for (i32 i = 0; i < state->enemy_count; i++) {
        if (!state->enemies[i].active) continue;
        i32 enemy_grid_x, enemy_grid_y;
        WorldToGrid(state->enemies[i].position, &enemy_grid_x, &enemy_grid_y);
        if (enemy_grid_x == grid_x && enemy_grid_y == grid_y) {
            printf("Cannot place tower: tile is occupied by an enemy\n");
            return -1;
        }
    }

    // Temporarily mark tile as blocked for path validation
    TileType old_type = GetTileType(&state->map, grid_x, grid_y);
    SetTileType(&state->map, grid_x, grid_y, TILE_BLOCKED);

    bool reachable[MAP_WIDTH * MAP_HEIGHT] = {0};
    Direction *trial_flow_field =
        CreateFlowFieldWithReachability(&state->map, reachable, MAP_WIDTH * MAP_HEIGHT);
    if (!trial_flow_field) {
        SetTileType(&state->map, grid_x, grid_y, old_type);
        printf("Cannot place tower: failed to rebuild flow field\n");
        return -1;
    }

    if (!ValidatePlacementReachability(state, reachable)) {
        free(trial_flow_field);
        SetTileType(&state->map, grid_x, grid_y, old_type);
        printf("Cannot place tower: enemies would be stuck!\n");
        return -1;
    }

    // Place the tower
    i32 index = state->tower_count++;
    Tower *tower = &state->towers[index];

    tower->type = type;
    tower->grid_x = grid_x;
    tower->grid_y = grid_y;
    tower->position = GridToWorld(grid_x, grid_y);
    tower->fire_countdown = 0;
    tower->target_enemy_id = -1;
    tower->rotation = 0;
    tower->active = true;

    // Deduct cost
    state->currency -= state->tower_config[type].stats.cost;
    printf("Placed %s tower at (%d, %d) for $%d\n",
           GetTowerName(state, type),
           grid_x, grid_y, state->tower_config[type].stats.cost);

    free(state->flow_field);
    state->flow_field = trial_flow_field;

    return index;
}

void RemoveTower(GameState *state, i32 index) {
    if (index < 0 || index >= state->tower_count) return;

    Tower *tower = &state->towers[index];

    // Restore tile
    SetTileType(&state->map, tower->grid_x, tower->grid_y, TILE_BUILDABLE);

    Direction *new_flow_field = CreateFlowField(&state->map);
    if (new_flow_field) {
        free(state->flow_field);
        state->flow_field = new_flow_field;
    } else {
        printf("Warning: failed to rebuild flow field after tower removal\n");
    }

    // Swap with last (O(1) removal)
    state->towers[index] = state->towers[state->tower_count - 1];
    state->tower_count--;
}

i32 FindNearestEnemy(const GameState *state, Vector2 tower_pos, f32 range) {
    i32 nearest = -1;
    f32 min_dist_sq = range * range * TILE_SIZE * TILE_SIZE;

    for (i32 i = 0; i < state->enemy_count; i++) {
        if (!state->enemies[i].active) continue;

        Vector2 diff = {
            state->enemies[i].position.x - tower_pos.x,
            state->enemies[i].position.y - tower_pos.y
        };
        f32 dist_sq = diff.x * diff.x + diff.y * diff.y;

        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            nearest = i;
        }
    }

    return nearest;
}

void FireBullet(GameState *state, i32 tower_index) {
    Tower *tower = &state->towers[tower_index];
    const TowerStats *stats = &state->tower_config[tower->type].stats;

    if (tower->target_enemy_id < 0) return;

    switch (tower->type) {
        case TOWER_DCA: {
            // DCA: spawn 4 bullets with staggered timing (handled by fire_countdown)
            f32 damage_per_bullet = stats->damage / 4.0f;
            SpawnBullet(state, BULLET_STANDARD, tower->position,
                       tower->target_enemy_id, damage_per_bullet);
            break;
        }

        case TOWER_FREEZE: {
            // Freeze: AoE slow effect (no bullet)
            for (i32 i = 0; i < state->enemy_count; i++) {
                if (!state->enemies[i].active) continue;

                Vector2 diff = {
                    state->enemies[i].position.x - tower->position.x,
                    state->enemies[i].position.y - tower->position.y
                };
                f32 dist = sqrtf(diff.x * diff.x + diff.y * diff.y);

                if (dist < stats->range * TILE_SIZE) {
                    state->enemies[i].freeze_timer = FREEZE_DURATION;
                }
            }
            break;
        }

        case TOWER_MISSILE: {
            // Missile: spawn missile bullet (splash on impact)
            SpawnBullet(state, BULLET_MISSILE, tower->position,
                       tower->target_enemy_id, stats->damage);
            break;
        }

        case TOWER_PLASMA: {
            // Plasma: standard bullet
            SpawnBullet(state, BULLET_PLASMA, tower->position,
                       tower->target_enemy_id, stats->damage);
            break;
        }

        case TOWER_VULCAN:
        default: {
            // Vulcan: standard bullet
            SpawnBullet(state, BULLET_STANDARD, tower->position,
                       tower->target_enemy_id, stats->damage);
            break;
        }

        case TOWER_WALL:
            // Wall doesn't shoot
            break;
    }
}

void UpdateTowers(GameState *state, f32 dt) {
    for (i32 i = 0; i < state->tower_count; i++) {
        Tower *tower = &state->towers[i];
        if (!tower->active) continue;

        const TowerStats *stats = &state->tower_config[tower->type].stats;

        // Wall doesn't do anything
        if (tower->type == TOWER_WALL) continue;

        // Find target
        i32 target_id = FindNearestEnemy(state, tower->position, stats->range);
        tower->target_enemy_id = target_id;

        // Rotate toward target
        if (target_id >= 0) {
            Enemy *target = &state->enemies[target_id];
            f32 dx = target->position.x - tower->position.x;
            f32 dy = target->position.y - tower->position.y;
            f32 target_angle = atan2f(dy, dx) * 180.0f / 3.14159f;

            // Lerp rotation
            f32 angle_diff = target_angle - tower->rotation;
            while (angle_diff > 180) angle_diff -= 360;
            while (angle_diff < -180) angle_diff += 360;

            f32 max_rotation = TOWER_ROTATION_SPEED * dt;
            if (fabsf(angle_diff) < max_rotation) {
                tower->rotation = target_angle;
            } else {
                tower->rotation += (angle_diff > 0 ? max_rotation : -max_rotation);
            }
        }

        // Fire countdown
        tower->fire_countdown -= dt;
        if (tower->fire_countdown <= 0 && target_id >= 0) {
            FireBullet(state, i);

            // Reset countdown
            if (stats->fire_rate > 0) {
                tower->fire_countdown = 1.0f / stats->fire_rate;

                // DCA special: fire 4 shots with small delays
                if (tower->type == TOWER_DCA) {
                    tower->fire_countdown = DCA_BURST_DELAY;  // Fast burst
                }
            }
        }
    }
}

void DrawTowers(const GameState *state, Texture2D spritesheet) {
    for (i32 i = 0; i < state->tower_count; i++) {
        const Tower *tower = &state->towers[i];
        if (!tower->active) continue;

        // Draw base (non-rotating)
        i32 base_id = GetTowerBaseSpriteIndex(state, tower->type);
        i32 base_sx = (base_id % SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
        i32 base_sy = (base_id / SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
        Rectangle base_src = {base_sx, base_sy, SPRITE_TILE_SIZE, SPRITE_TILE_SIZE};
        Rectangle dest = {
            tower->position.x - TILE_SIZE / 2,
            tower->position.y - TILE_SIZE / 2,
            TILE_SIZE,
            TILE_SIZE
        };
        DrawTexturePro(spritesheet, base_src, dest, (Vector2){0, 0}, 0, WHITE);

        // Draw gun (rotating) - walls have no gun
        i32 gun_id = GetTowerGunSpriteIndex(state, tower->type);
        if (gun_id >= 0) {
            i32 gun_sx = (gun_id % SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
            i32 gun_sy = (gun_id / SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
            Rectangle gun_src = {gun_sx, gun_sy, SPRITE_TILE_SIZE, SPRITE_TILE_SIZE};
            // For rotation, dest must be positioned at center and origin at center
            Rectangle gun_dest = {
                tower->position.x,
                tower->position.y,
                TILE_SIZE,
                TILE_SIZE
            };
            Vector2 origin = {TILE_SIZE / 2.0f, TILE_SIZE / 2.0f};
            DrawTexturePro(spritesheet, gun_src, gun_dest, origin, tower->rotation, WHITE);
        }
    }
}
