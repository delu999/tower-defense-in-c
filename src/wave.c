#include "wave.h"
#include "enemy.h"
#include "config.h"
#include <stdlib.h>
#include <stdio.h>

// Level 1 wave definitions
static WaveEntry level1_waves[] = {
    // Wave 1
    {ENEMY_SIMPLE, 3, 1.0f}, {ENEMY_FAST, 3, 1.0f},
    // Wave 2
    {ENEMY_FAST, 10, 1.0f},
    // Wave 3
    {ENEMY_HEAVY, 5, 1.0f}, {ENEMY_SIMPLE, 10, 1.0f},
    // Wave 4
    {ENEMY_HEAVY, 5, 1.5f}, {ENEMY_SIMPLE, 10, 1.5f},
    // Wave 5
    {ENEMY_FLYING, 20, 5.0f},
    // Wave 6
    {ENEMY_SHIELDED, 5, 2.0f},
    // Wave 7
    {ENEMY_FAST, 20, 5.0f},
    // Wave 8
    {ENEMY_SIMPLE, 90, 6.0f}, {ENEMY_SHIELDED, 5, 2.0f},
    // Wave 9
    {ENEMY_FLYING, 40, 6.0f}, {ENEMY_HEAVY, 40, 10.0f},
    // Wave 10
    {ENEMY_BOSS, 1, 1.0f}
};

static i32 level1_wave_sizes[] = {2, 1, 2, 2, 1, 1, 1, 2, 2, 1};
#define LEVEL1_TOTAL_WAVES 10

// Level 2 wave definitions (slightly harder)
static WaveEntry level2_waves[] = {
    // Wave 1
    {ENEMY_SIMPLE, 5, 1.5f}, {ENEMY_FAST, 5, 1.5f},
    // Wave 2
    {ENEMY_HEAVY, 10, 1.5f},
    // Wave 3
    {ENEMY_FAST, 15, 2.0f}, {ENEMY_SHIELDED, 3, 2.0f},
    // Wave 4
    {ENEMY_FLYING, 25, 5.0f},
    // Wave 5
    {ENEMY_HEAVY, 10, 3.0f}, {ENEMY_SIMPLE, 20, 2.0f},
    // Wave 6
    {ENEMY_SHIELDED, 10, 3.0f},
    // Wave 7
    {ENEMY_FAST, 30, 6.0f}, {ENEMY_FLYING, 20, 6.0f},
    // Wave 8
    {ENEMY_SIMPLE, 100, 8.0f}, {ENEMY_HEAVY, 30, 8.0f},
    // Wave 9
    {ENEMY_FLYING, 50, 8.0f}, {ENEMY_SHIELDED, 20, 5.0f},
    // Wave 10
    {ENEMY_BOSS, 2, 1.5f}
};

static i32 level2_wave_sizes[] = {2, 1, 2, 1, 2, 1, 2, 2, 2, 1};
#define LEVEL2_TOTAL_WAVES 10

// Level 3 wave definitions (hardest)
static WaveEntry level3_waves[] = {
    // Wave 1
    {ENEMY_FAST, 10, 2.0f}, {ENEMY_HEAVY, 5, 2.0f},
    // Wave 2
    {ENEMY_SHIELDED, 8, 2.5f}, {ENEMY_FLYING, 15, 3.0f},
    // Wave 3
    {ENEMY_SIMPLE, 30, 3.0f}, {ENEMY_FAST, 30, 3.0f},
    // Wave 4
    {ENEMY_HEAVY, 20, 4.0f}, {ENEMY_SHIELDED, 10, 3.0f},
    // Wave 5
    {ENEMY_FLYING, 40, 8.0f},
    // Wave 6
    {ENEMY_FAST, 50, 8.0f}, {ENEMY_SIMPLE, 50, 8.0f},
    // Wave 7
    {ENEMY_SHIELDED, 20, 5.0f}, {ENEMY_HEAVY, 20, 6.0f},
    // Wave 8
    {ENEMY_FLYING, 60, 10.0f}, {ENEMY_BOSS, 1, 2.0f},
    // Wave 9
    {ENEMY_SIMPLE, 150, 10.0f}, {ENEMY_SHIELDED, 30, 6.0f},
    // Wave 10
    {ENEMY_BOSS, 3, 2.0f}, {ENEMY_FLYING, 50, 10.0f}
};

static i32 level3_wave_sizes[] = {2, 2, 2, 2, 1, 2, 2, 2, 2, 2};
#define LEVEL3_TOTAL_WAVES 10

void InitWaveManager(WaveManager *mgr, i32 level) {
    mgr->current_wave = 0;
    mgr->enemies_spawned = 0;
    mgr->spawn_timer = 0;
    mgr->countdown_timer = WAVE_COUNTDOWN_SECONDS;
    mgr->wave_active = false;

    switch (level) {
        case 0:
            mgr->waves = level1_waves;
            mgr->wave_sizes = level1_wave_sizes;
            mgr->total_waves = LEVEL1_TOTAL_WAVES;
            break;
        case 1:
            mgr->waves = level2_waves;
            mgr->wave_sizes = level2_wave_sizes;
            mgr->total_waves = LEVEL2_TOTAL_WAVES;
            break;
        case 2:
            mgr->waves = level3_waves;
            mgr->wave_sizes = level3_wave_sizes;
            mgr->total_waves = LEVEL3_TOTAL_WAVES;
            break;
        default:
            mgr->waves = level1_waves;
            mgr->wave_sizes = level1_wave_sizes;
            mgr->total_waves = LEVEL1_TOTAL_WAVES;
            break;
    }

    printf("Wave manager initialized: %d waves\n", mgr->total_waves);
}

void StartNextWave(GameState *state) {
    WaveManager *mgr = &state->wave_mgr;

    if (mgr->current_wave >= mgr->total_waves) {
        printf("All waves complete!\n");
        return;
    }

    mgr->wave_active = true;
    mgr->enemies_spawned = 0;
    mgr->spawn_timer = 0;

    printf("Starting wave %d/%d\n", mgr->current_wave + 1, mgr->total_waves);
}

void UpdateWaveManager(GameState *state, f32 dt) {
    WaveManager *mgr = &state->wave_mgr;

    // Check victory condition
    if (AllWavesComplete(mgr) && state->enemy_count == 0) {
        // All waves done and no enemies left - victory!
        return;
    }

    if (!mgr->wave_active) {
        // Countdown to next wave
        mgr->countdown_timer -= dt;

        // Allow player to start wave early by pressing SPACE
        if (IsKeyPressed(KEY_SPACE) || mgr->countdown_timer <= 0) {
            if (mgr->current_wave < mgr->total_waves) {
                StartNextWave(state);
            }
        }
    } else {
        // Wave is active - spawn enemies
        mgr->spawn_timer -= dt;

        if (mgr->spawn_timer <= 0) {
            // Calculate wave start index
            i32 wave_start = 0;
            for (i32 i = 0; i < mgr->current_wave; i++) {
                wave_start += mgr->wave_sizes[i];
            }

            i32 wave_size = mgr->wave_sizes[mgr->current_wave];
            WaveEntry *wave_entries = &mgr->waves[wave_start];

            // Spawn enemies from current wave
            i32 total_enemies_in_wave = 0;
            for (i32 i = 0; i < wave_size; i++) {
                total_enemies_in_wave += wave_entries[i].quantity;
            }

            if (mgr->enemies_spawned < total_enemies_in_wave) {
                // Find which entry to spawn from
                i32 spawn_count = 0;
                for (i32 i = 0; i < wave_size; i++) {
                    spawn_count += wave_entries[i].quantity;
                    if (mgr->enemies_spawned < spawn_count) {
                        // Spawn this enemy type
                        EnemyType type = wave_entries[i].enemy_type;
                        f32 difficulty = wave_entries[i].difficulty;

                        // Random spawn point
                        i32 spawn_idx = rand() % state->map.spawn_count;
                        Vector2 spawn_pos = state->map.spawn_points[spawn_idx];

                        SpawnEnemy(state, type, spawn_pos, difficulty);
                        mgr->enemies_spawned++;
                        mgr->spawn_timer = ENEMY_SPAWN_DELAY;
                        break;
                    }
                }
            } else {
                // Wave complete
                mgr->wave_active = false;
                mgr->current_wave++;
                mgr->countdown_timer = WAVE_COUNTDOWN_SECONDS;
                printf("Wave %d complete! Next wave in %.0f seconds\n",
                       mgr->current_wave, mgr->countdown_timer);
            }
        }
    }
}

bool AllWavesComplete(const WaveManager *mgr) {
    return mgr->current_wave >= mgr->total_waves && !mgr->wave_active;
}
