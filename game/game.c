#include "game.h"
#include "map.h"
#include "pathfinding.h"
#include "enemy.h"
#include "tower.h"
#include "bullet.h"
#include "wave.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void InitGame(GameState *state, i32 level) {
    printf("Initializing level %d...\n", level);

    // Reset counters
    state->tower_count = 0;
    state->enemy_count = 0;
    state->bullet_count = 0;
    state->currency = STARTING_CURRENCY;
    state->base_life = STARTING_BASE_LIFE;
    state->current_level = level;
    state->paused = false;

    if (state->flow_field) {
        free(state->flow_field);
        state->flow_field = NULL;
    }
    if (state->flying_flow_field) {
        free(state->flying_flow_field);
        state->flying_flow_field = NULL;
    }
    state->show_flow_field = false;

    // Clear all entities
    memset(state->towers, 0, sizeof(state->towers));
    memset(state->enemies, 0, sizeof(state->enemies));
    memset(state->bullets, 0, sizeof(state->bullets));

    // Validate paths
    if (!ValidatePaths(&state->map)) {
        printf("WARNING: Map has unreachable areas!\n");
    }

    state->flow_field = CreateFlowField(&state->map);
    if (!state->flow_field) {
        printf("WARNING: Failed to create flow field\n");
    }

    // Flying enemies use the initial flow field (no turrets blocking)
    state->flying_flow_field = CreateFlowField(&state->map);
    if (!state->flying_flow_field) {
        printf("WARNING: Failed to create flying flow field\n");
    }

    // Initialize wave manager from loaded level config.
    InitWaveManager(&state->wave_mgr,
                    state->level_wave_entries,
                    state->level_wave_sizes,
                    state->level_total_waves);
    InitUI(state->ui);

    printf("Game initialized!\n");
}

void UpdateGame(GameState *state, f32 dt) {
    // Update UI (handles mouse input)
    UpdateUI(state->ui, state, dt);

    if (state->screen != SCREEN_PLAYING) {
        return;
    }

    if (IsKeyPressed(KEY_F)) {
        state->show_flow_field = !state->show_flow_field;
    }

    if (state->paused) {
        return;
    }

    // Update wave manager
    UpdateWaveManager(state, dt);

    // Update towers
    UpdateTowers(state, dt);

    // Update bullets
    UpdateBullets(state, dt);

    // Update enemies
    UpdateEnemies(state, dt);
}

void DrawGame(const GameState *state, Texture2D spritesheet, Font font) {
    // Draw top HUD bar
    DrawRectangle(0, 0, SCREEN_WIDTH, HUD_HEIGHT, ColorAlpha(BLACK, 0.8f));

    char buf[128];

    // Life
    snprintf(buf, sizeof(buf), "Life: %d", state->base_life);
    DrawTextEx(font, buf, (Vector2){15, 8}, 24, 1, RED);

    // Currency
    snprintf(buf, sizeof(buf), "$ %d", state->currency);
    DrawTextEx(font, buf, (Vector2){170, 8}, 24, 1, GOLD);

    // Enemy count
    snprintf(buf, sizeof(buf), "Enemies: %d", state->enemy_count);
    DrawTextEx(font, buf, (Vector2){320, 8}, 24, 1, ORANGE);

    // Wave info
    const WaveManager *mgr = &state->wave_mgr;
    if (mgr->wave_active) {
        snprintf(buf, sizeof(buf), "Wave %d/%d", mgr->current_wave + 1, mgr->total_waves);
        DrawTextEx(font, buf, (Vector2){520, 8}, 24, 1, WHITE);
    } else if (mgr->current_wave < mgr->total_waves) {
        snprintf(buf, sizeof(buf), "%.0fs", mgr->countdown_timer);
        DrawTextEx(font, buf, (Vector2){520, 8}, 24, 1, YELLOW);
    }

    // Level
    snprintf(buf, sizeof(buf), "Level %d", state->current_level + 1);
    DrawTextEx(font, buf, (Vector2){700, 8}, 24, 1, SKYBLUE);

    // Draw the map
    DrawMap(&state->map, spritesheet);

    // Draw towers
    DrawTowers(state, spritesheet);

    // Draw enemies
    DrawEnemies(state, spritesheet);

    // Draw bullets
    DrawBullets(state);

    // Draw UI (shop panel, placement preview, etc.)
    DrawUI(state->ui, state, spritesheet, font);

    if (state->paused) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.35f));
        DrawTextEx(font, "PAUSED", (Vector2){SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 - 30}, 48, 2, WHITE);
        DrawTextEx(font, "Press P or click Resume", (Vector2){SCREEN_WIDTH / 2 - 145, SCREEN_HEIGHT / 2 + 20}, 22, 1, LIGHTGRAY);
    }

    if (state->show_flow_field) {
        DrawFlowField(state->flow_field, &state->map);
    }
}

void CleanupGame(GameState *state) {
    if (state->flow_field) {
        free(state->flow_field);
        state->flow_field = NULL;
    }
    if (state->flying_flow_field) {
        free(state->flying_flow_field);
        state->flying_flow_field = NULL;
    }
    printf("Game cleaned up.\n");
}
