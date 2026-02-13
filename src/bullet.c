#include "bullet.h"
#include "enemy.h"
#include "map.h"
#include "config.h"
#include <math.h>
#include <stdio.h>

int SpawnBullet(GameState *state, BulletType type, Vector2 position,
                int target_enemy_id, float damage) {
    if (state->bullet_count >= MAX_BULLETS) {
        return -1;
    }

    int index = state->bullet_count++;
    Bullet *bullet = &state->bullets[index];

    bullet->type = type;
    bullet->position = position;
    bullet->target_enemy_id = target_enemy_id;
    bullet->damage = damage;
    bullet->speed = BULLET_SPEED * TILE_SIZE;  // Convert to pixels/sec
    bullet->max_range = 20.0f * TILE_SIZE;     // Max travel distance
    bullet->distance_traveled = 0;
    bullet->active = true;

    // Initial direction (will be updated for homing)
    if (target_enemy_id >= 0 && target_enemy_id < state->enemy_count) {
        Enemy *target = &state->enemies[target_enemy_id];
        Vector2 dir = {
            target->position.x - position.x,
            target->position.y - position.y
        };
        float dist = sqrtf(dir.x * dir.x + dir.y * dir.y);
        if (dist > 0) {
            bullet->direction.x = dir.x / dist;
            bullet->direction.y = dir.y / dist;
        }
    }

    return index;
}

void UpdateBullets(GameState *state, float dt) {
    for (int i = 0; i < state->bullet_count; i++) {
        Bullet *bullet = &state->bullets[i];
        if (!bullet->active) continue;

        // Check if target is still alive
        bool target_alive = false;
        if (bullet->target_enemy_id >= 0 && bullet->target_enemy_id < state->enemy_count) {
            if (state->enemies[bullet->target_enemy_id].active) {
                target_alive = true;
            }
        }

        if (!target_alive) {
            // Target died, remove bullet
            RemoveBullet(state, i);
            i--;
            continue;
        }

        Enemy *target = &state->enemies[bullet->target_enemy_id];

        // Homing: update direction toward target
        Vector2 to_target = {
            target->position.x - bullet->position.x,
            target->position.y - bullet->position.y
        };
        float dist_to_target = sqrtf(to_target.x * to_target.x + to_target.y * to_target.y);

        if (dist_to_target > 0) {
            bullet->direction.x = to_target.x / dist_to_target;
            bullet->direction.y = to_target.y / dist_to_target;
        }

        // Move bullet
        float move_dist = bullet->speed * dt;
        bullet->position.x += bullet->direction.x * move_dist;
        bullet->position.y += bullet->direction.y * move_dist;
        bullet->distance_traveled += move_dist;

        // Check range limit
        if (bullet->distance_traveled > bullet->max_range) {
            RemoveBullet(state, i);
            i--;
            continue;
        }

        // Check collision with target
        if (dist_to_target < TILE_SIZE / 3) {  // Hit radius
            // Apply damage
            if (bullet->type == BULLET_MISSILE) {
                // Missile: splash damage
                for (int e = 0; e < state->enemy_count; e++) {
                    if (!state->enemies[e].active) continue;

                    Vector2 diff = {
                        state->enemies[e].position.x - bullet->position.x,
                        state->enemies[e].position.y - bullet->position.y
                    };
                    float dist = sqrtf(diff.x * diff.x + diff.y * diff.y);

                    if (dist < MISSILE_SPLASH_RADIUS * TILE_SIZE) {
                        DamageEnemy(&state->enemies[e], bullet->damage);

                        // Award currency if enemy dies
                        if (!state->enemies[e].active) {
                            EnemyType dead_type = state->enemies[e].type;
                            Vector2 dead_pos = state->enemies[e].position;
                            int reward = state->enemies[e].reward;

                            state->currency += reward;
                            printf("Enemy killed! +$%d (Total: $%d)\n", reward, state->currency);

                            // Boss special: spawn minions on death
                            if (dead_type == ENEMY_BOSS) {
                                printf("Boss defeated! Spawning minions...\n");
                                int grid_x, grid_y;
                                WorldToGrid(dead_pos, &grid_x, &grid_y);
                                Vector2 boss_grid = {grid_x, grid_y};

                                // Spawn 5 simple enemies at boss location
                                for (int m = 0; m < 5; m++) {
                                    SpawnEnemy(state, ENEMY_SIMPLE, boss_grid, 2.0f);
                                }
                            }

                            RemoveEnemy(state, e);
                            e--;
                        }
                    }
                }
            } else {
                // Standard bullet: single target damage
                DamageEnemy(target, bullet->damage);

                // Award currency if enemy dies
                if (!target->active) {
                    state->currency += target->reward;
                    printf("Enemy killed! +$%d (Total: $%d)\n",
                           target->reward, state->currency);

                    // Boss special: spawn minions on death
                    if (target->type == ENEMY_BOSS) {
                        printf("Boss defeated! Spawning minions...\n");
                        Vector2 boss_grid;
                        int grid_x, grid_y;
                        WorldToGrid(target->position, &grid_x, &grid_y);
                        boss_grid.x = grid_x;
                        boss_grid.y = grid_y;

                        // Spawn 5 simple enemies at boss location
                        for (int m = 0; m < 5; m++) {
                            SpawnEnemy(state, ENEMY_SIMPLE, boss_grid, 2.0f);
                        }
                    }

                    RemoveEnemy(state, bullet->target_enemy_id);
                }
            }

            // Remove bullet
            RemoveBullet(state, i);
            i--;
        }
    }
}

void DrawBullets(const GameState *state) {
    for (int i = 0; i < state->bullet_count; i++) {
        const Bullet *bullet = &state->bullets[i];
        if (!bullet->active) continue;

        Color color;
        float radius;

        switch (bullet->type) {
            case BULLET_MISSILE:
                color = ORANGE;
                radius = 6;
                break;
            case BULLET_PLASMA:
                color = PURPLE;
                radius = 5;
                break;
            default:
                color = YELLOW;
                radius = 4;
                break;
        }

        DrawCircle((int)bullet->position.x, (int)bullet->position.y, radius, color);
        DrawCircleLines((int)bullet->position.x, (int)bullet->position.y, radius, BLACK);
    }
}

void RemoveBullet(GameState *state, int index) {
    if (index < 0 || index >= state->bullet_count) return;

    // Swap with last (O(1) removal)
    state->bullets[index] = state->bullets[state->bullet_count - 1];
    state->bullet_count--;
}
