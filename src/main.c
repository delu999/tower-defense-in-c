#include "raylib.h"
#include "game.h"
#include "wave.h"
#include "ui.h"
#include "config.h"
#include <stdio.h>

// Global assets
static Texture2D spritesheet;
static Font font;

int main(void) {
    // Initialize window with HiDPI support
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Tower Defense - C + Raylib");
    SetTargetFPS(TARGET_FPS);

    // Load assets
    spritesheet = LoadTexture("assets/sprites/towerDefense_tilesheet@2.png");
    if (spritesheet.id == 0) {
        printf("Failed to load spritesheet!\n");
        CloseWindow();
        return 1;
    }
    SetTextureFilter(spritesheet, TEXTURE_FILTER_BILINEAR);

    // Load font at large size for crisp rendering at all display sizes
    font = LoadFontEx("assets/fonts/Poppins-Regular.ttf", 96, 0, 0);
    if (font.texture.id == 0) {
        printf("Failed to load font!\n");
        UnloadTexture(spritesheet);
        CloseWindow();
        return 1;
    }
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

    // Initialize game state
    GameState state = {0};
    UIState ui = {0};
    state.ui = &ui;
    state.screen = SCREEN_MENU;
    state.currency = STARTING_CURRENCY;
    state.base_life = STARTING_BASE_LIFE;
    state.current_level = 0;
    InitUI(&ui);

    printf("Assets loaded successfully!\n");
    printf("Spritesheet: %dx%d\n", spritesheet.width, spritesheet.height);

    // Main game loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Update
        switch (state.screen) {
            case SCREEN_MENU:
                // Simple menu: press 1, 2, or 3 to start a level
                if (IsKeyPressed(KEY_ONE)) {
                    state.current_level = 0;
                    InitGame(&state, 0);
                    state.screen = SCREEN_PLAYING;
                }
                if (IsKeyPressed(KEY_TWO)) {
                    state.current_level = 1;
                    InitGame(&state, 1);
                    state.screen = SCREEN_PLAYING;
                }
                if (IsKeyPressed(KEY_THREE)) {
                    state.current_level = 2;
                    InitGame(&state, 2);
                    state.screen = SCREEN_PLAYING;
                }
                break;

            case SCREEN_PLAYING:
                UpdateGame(&state, dt);

                // Check win/lose conditions
                if (state.base_life <= 0) {
                    state.screen = SCREEN_GAME_OVER;
                } else if (AllWavesComplete(&state.wave_mgr) && state.enemy_count == 0) {
                    state.screen = SCREEN_VICTORY;
                }
                break;

            case SCREEN_GAME_OVER:
                if (IsKeyPressed(KEY_R)) {
                    InitGame(&state, state.current_level);
                    state.screen = SCREEN_PLAYING;
                }
                if (IsKeyPressed(KEY_M)) {
                    state.screen = SCREEN_MENU;
                }
                break;

            case SCREEN_VICTORY:
                if (IsKeyPressed(KEY_N) && state.current_level < 2) {
                    state.current_level++;
                    InitGame(&state, state.current_level);
                    state.screen = SCREEN_PLAYING;
                }
                if (IsKeyPressed(KEY_M)) {
                    state.screen = SCREEN_MENU;
                }
                break;
        }

        // Draw
        BeginDrawing();
        ClearBackground(BG_COLOR);

        switch (state.screen) {
            case SCREEN_MENU: {
                // Title
                DrawTextEx(font, "TOWER DEFENSE", (Vector2){SCREEN_WIDTH/2 - 220, 80}, 56, 2, DARKBLUE);
                DrawTextEx(font, "C + Raylib Edition", (Vector2){SCREEN_WIDTH/2 - 140, 150}, 24, 1, GRAY);

                // Level buttons
                const int button_w = 200;
                const int button_h = 60;
                const int button_spacing = 80;
                const int start_y = 250;

                for (int i = 0; i < 3; i++) {
                    int button_x = SCREEN_WIDTH / 2 - button_w / 2;
                    int button_y = start_y + i * button_spacing;

                    Rectangle button = {button_x, button_y, button_w, button_h};
                    Color button_color = DARKGRAY;

                    if (CheckCollisionPointRec(GetMousePosition(), button)) {
                        button_color = GRAY;
                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            state.current_level = i;
                            InitGame(&state, i);
                            state.screen = SCREEN_PLAYING;
                        }
                    }

                    DrawRectangleRec(button, button_color);
                    DrawRectangleLinesEx(button, 2, WHITE);

                    char level_text[32];
                    snprintf(level_text, sizeof(level_text), "Level %d", i + 1);
                    int text_w = MeasureText(level_text, 32);
                    DrawTextEx(font, level_text,
                              (Vector2){button_x + button_w / 2 - text_w / 2, button_y + 15},
                              32, 1, WHITE);
                }

                DrawTextEx(font, "Click a level to begin, or press 1, 2, 3",
                          (Vector2){SCREEN_WIDTH/2 - 240, 520}, 20, 1, LIGHTGRAY);
                break;
            }

            case SCREEN_PLAYING:
                DrawGame(&state, spritesheet, font);
                break;

            case SCREEN_GAME_OVER:
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.7f));
                DrawTextEx(font, "GAME OVER", (Vector2){SCREEN_WIDTH/2 - 180, 200}, 60, 2, RED);
                DrawTextEx(font, "The base was destroyed!", (Vector2){SCREEN_WIDTH/2 - 140, 290}, 24, 1, LIGHTGRAY);
                DrawTextEx(font, "Press R to Retry", (Vector2){SCREEN_WIDTH/2 - 110, 370}, 28, 1, WHITE);
                DrawTextEx(font, "Press M for Menu", (Vector2){SCREEN_WIDTH/2 - 115, 420}, 28, 1, WHITE);
                break;

            case SCREEN_VICTORY:
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.6f));
                DrawTextEx(font, "VICTORY!", (Vector2){SCREEN_WIDTH/2 - 150, 200}, 64, 2, GOLD);
                DrawTextEx(font, "All waves defeated!", (Vector2){SCREEN_WIDTH/2 - 120, 290}, 24, 1, LIGHTGRAY);
                if (state.current_level < 2) {
                    DrawTextEx(font, "Press N for Next Level", (Vector2){SCREEN_WIDTH/2 - 150, 370}, 28, 1, YELLOW);
                } else {
                    DrawTextEx(font, "You completed all levels!", (Vector2){SCREEN_WIDTH/2 - 165, 370}, 28, 1, GREEN);
                }
                DrawTextEx(font, "Press M for Menu", (Vector2){SCREEN_WIDTH/2 - 115, 420}, 28, 1, WHITE);
                break;
        }

        EndDrawing();
    }

    // Cleanup
    CleanupGame(&state);
    UnloadTexture(spritesheet);
    UnloadFont(font);
    CloseWindow();

    return 0;
}
