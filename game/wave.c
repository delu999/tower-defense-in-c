#include "wave.h"
#include "enemy.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>

void InitWaveManager(WaveManager *mgr, WaveEntry *waves, i32 *wave_sizes, i32 total_waves) {
    mgr->waves = waves;
    mgr->wave_sizes = wave_sizes;
    mgr->total_waves = total_waves;
    mgr->current_wave = 0;
    mgr->enemies_spawned = 0;
    mgr->spawn_timer = 0.0f;
    mgr->countdown_timer = WAVE_COUNTDOWN_SECONDS;
    mgr->wave_active = false;

    printf("Wave manager initialized: %d waves\n", mgr->total_waves);
}

void StartNextWave(GameState *state) {
    WaveManager *mgr = &state->wave_mgr;

    if (mgr->current_wave >= mgr->total_waves) {
        printf("All waves complete!\n");
        return;
    }

    if (state->map.spawn_count <= 0) {
        printf("Cannot start wave: map has no spawn points\n");
        mgr->countdown_timer = WAVE_COUNTDOWN_SECONDS;
        return;
    }

    if (state->map.base_count <= 0) {
        printf("Cannot start wave: map has no base points\n");
        mgr->countdown_timer = WAVE_COUNTDOWN_SECONDS;
        return;
    }

    mgr->wave_active = true;
    mgr->enemies_spawned = 0;
    mgr->spawn_timer = 0.0f;

    printf("Starting wave %d/%d\n", mgr->current_wave + 1, mgr->total_waves);
}

void UpdateWaveManager(GameState *state, f32 dt) {
    WaveManager *mgr = &state->wave_mgr;

    if (AllWavesComplete(mgr) && state->enemy_count == 0) {
        return;
    }

    if (!mgr->wave_active) {
        mgr->countdown_timer -= dt;

        if (IsKeyPressed(KEY_SPACE) || mgr->countdown_timer <= 0.0f) {
            if (mgr->current_wave < mgr->total_waves) {
                StartNextWave(state);
            }
        }
        return;
    }

    mgr->spawn_timer -= dt;
    if (mgr->spawn_timer > 0.0f) {
        return;
    }

    i32 wave_start = 0;
    for (i32 i = 0; i < mgr->current_wave; i++) {
        wave_start += mgr->wave_sizes[i];
    }

    i32 wave_size = mgr->wave_sizes[mgr->current_wave];
    WaveEntry *wave_entries = &mgr->waves[wave_start];

    i32 total_enemies_in_wave = 0;
    for (i32 i = 0; i < wave_size; i++) {
        total_enemies_in_wave += wave_entries[i].quantity;
    }

    if (mgr->enemies_spawned >= total_enemies_in_wave) {
        mgr->wave_active = false;
        mgr->current_wave++;
        mgr->countdown_timer = WAVE_COUNTDOWN_SECONDS;
        printf("Wave %d complete! Next wave in %.0f seconds\n", mgr->current_wave, mgr->countdown_timer);
        return;
    }

    i32 spawn_count = 0;
    for (i32 i = 0; i < wave_size; i++) {
        spawn_count += wave_entries[i].quantity;
        if (mgr->enemies_spawned >= spawn_count) {
            continue;
        }

        if (state->map.spawn_count <= 0) {
            printf("Cannot spawn enemy: map has no spawn points\n");
            mgr->wave_active = false;
            mgr->countdown_timer = WAVE_COUNTDOWN_SECONDS;
            return;
        }

        i32 spawn_idx = rand() % state->map.spawn_count;
        Vector2 spawn_pos = state->map.spawn_points[spawn_idx];

        SpawnEnemy(state, wave_entries[i].enemy_type, spawn_pos, wave_entries[i].difficulty);
        mgr->enemies_spawned++;
        mgr->spawn_timer = ENEMY_SPAWN_DELAY;
        return;
    }
}

bool AllWavesComplete(const WaveManager *mgr) {
    return mgr->current_wave >= mgr->total_waves && !mgr->wave_active;
}
