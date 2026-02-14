#include "game.h"
#include "map.h"
#include "pathfinding.h"
#include "enemy.h"
#include "tower.h"
#include "bullet.h"
#include "wave.h"
#include "ui.h"
#include <stdio.h>
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

    // Clear all entities
    memset(state->towers, 0, sizeof(state->towers));
    memset(state->enemies, 0, sizeof(state->enemies));
    memset(state->bullets, 0, sizeof(state->bullets));

    // Initialize map
    InitMap(&state->map, level);

    // Validate paths
    if (!ValidatePaths(&state->map)) {
        printf("WARNING: Map has unreachable areas!\n");
    }

    // Initialize wave manager
    InitWaveManager(&state->wave_mgr, level);

    printf("Game initialized!\n");
}

void UpdateGame(GameState *state, f32 dt) {
    // Update UI (handles mouse input)
    UpdateUI(state->ui, state, dt);

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
    DrawTextEx(font, buf, (Vector2){680, 8}, 24, 1, SKYBLUE);

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
}

void CleanupGame(GameState *state) {
    // No dynamic memory to free (all static arrays)
    printf("Game cleaned up.\n");
}
