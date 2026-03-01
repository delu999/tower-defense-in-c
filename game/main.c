#include "raylib.h"
#include "game.h"
#include "wave.h"
#include "ui.h"
#include "config.h"
#include "map.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#define MAX_LEVELS 32
#define LEVEL_NAME_MAX 64

typedef struct {
    char filename[256];
    char name[LEVEL_NAME_MAX];
} LevelEntry;

// Global assets
static Texture2D spritesheet;
static Font font;

// Level list
static LevelEntry level_list[MAX_LEVELS];
static i32 level_count = 0;

static void ScanLevels(void) {
    level_count = 0;

    DIR *dir = opendir("levels");
    if (!dir) return;

    struct dirent *ent;
    while ((ent = readdir(dir)) && level_count < MAX_LEVELS) {
        i32 len = (i32)strlen(ent->d_name);
        if (len < 6 || strcmp(ent->d_name + len - 5, ".conf") != 0) continue;

        LevelEntry *le = &level_list[level_count];
        snprintf(le->filename, sizeof(le->filename), "levels/%s", ent->d_name);

        // Parse name from file
        strncpy(le->name, ent->d_name, LEVEL_NAME_MAX - 1);
        FILE *f = fopen(le->filename, "r");
        if (f) {
            char line[256];
            if (fgets(line, sizeof(line), f)) {
                i32 l = (i32)strlen(line);
                while (l > 0 && (line[l-1] == '\n' || line[l-1] == '\r')) line[--l] = '\0';
                if (strncmp(line, "name=", 5) == 0) {
                    strncpy(le->name, line + 5, LEVEL_NAME_MAX - 1);
                }
            }
            fclose(f);
        }

        level_count++;
    }

    closedir(dir);
}

i32 main(void) {
    // Initialize window with HiDPI support
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Tower Defense");
    SetTargetFPS(TARGET_FPS);

    // Load assets
    spritesheet = LoadTexture("assets/sprites/towerDefense_tilesheet@2.png");
    if (spritesheet.id == 0) {
        printf("Failed to load spritesheet!\n");
        CloseWindow();
        return 1;
    }
    // Use point sampling for atlas sprites to avoid texture bleeding between tiles.
    SetTextureFilter(spritesheet, TEXTURE_FILTER_POINT);

    // Load font at large size for crisp rendering at all display sizes
    font = LoadFontEx("assets/fonts/Poppins-Regular.ttf", 96, 0, 0);
    if (font.texture.id == 0) {
        printf("Failed to load font!\n");
        UnloadTexture(spritesheet);
        CloseWindow();
        return 1;
    }
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

    // Scan for .conf levels
    ScanLevels();

    // If no levels found, use built-in InitLevel1/2/3
    // This is fallback for when .conf files don't exist
    if (level_count == 0) {
        printf("No .conf levels found, using built-in levels\n");
    }

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
    printf("Found %d level(s)\n", level_count);

    // Menu scroll state
    i32 menu_scroll = 0;
    const i32 menu_item_h = 60;
    const i32 visible_items = (SCREEN_HEIGHT - 350) / menu_item_h;

    // Main game loop
    while (!WindowShouldClose()) {
        f32 dt = GetFrameTime();

        // Update
        switch (state.screen) {
            case SCREEN_MENU: {
                // Handle keyboard shortcuts for first 9 levels
                if (IsKeyPressed(KEY_ONE) && level_count > 0) {
                    state.current_level = 0;
                    if (LoadMapFromConf(&state.map, level_list[0].filename)) {
                        InitGame(&state, state.current_level);
                        state.screen = SCREEN_PLAYING;
                    }
                }
                if (IsKeyPressed(KEY_TWO) && level_count > 1) {
                    state.current_level = 1;
                    if (LoadMapFromConf(&state.map, level_list[1].filename)) {
                        InitGame(&state, state.current_level);
                        state.screen = SCREEN_PLAYING;
                    }
                }
                if (IsKeyPressed(KEY_THREE) && level_count > 2) {
                    state.current_level = 2;
                    if (LoadMapFromConf(&state.map, level_list[2].filename)) {
                        InitGame(&state, state.current_level);
                        state.screen = SCREEN_PLAYING;
                    }
                }
                if (IsKeyPressed(KEY_FOUR) && level_count > 3) {
                    state.current_level = 3;
                    if (LoadMapFromConf(&state.map, level_list[3].filename)) {
                        InitGame(&state, state.current_level);
                        state.screen = SCREEN_PLAYING;
                    }
                }
                if (IsKeyPressed(KEY_FIVE) && level_count > 4) {
                    state.current_level = 4;
                    if (LoadMapFromConf(&state.map, level_list[4].filename)) {
                        InitGame(&state, state.current_level);
                        state.screen = SCREEN_PLAYING;
                    }
                }
                if (IsKeyPressed(KEY_SIX) && level_count > 5) {
                    state.current_level = 5;
                    if (LoadMapFromConf(&state.map, level_list[5].filename)) {
                        InitGame(&state, state.current_level);
                        state.screen = SCREEN_PLAYING;
                    }
                }
                if (IsKeyPressed(KEY_SEVEN) && level_count > 6) {
                    state.current_level = 6;
                    if (LoadMapFromConf(&state.map, level_list[6].filename)) {
                        InitGame(&state, state.current_level);
                        state.screen = SCREEN_PLAYING;
                    }
                }
                if (IsKeyPressed(KEY_EIGHT) && level_count > 7) {
                    state.current_level = 7;
                    if (LoadMapFromConf(&state.map, level_list[7].filename)) {
                        InitGame(&state, state.current_level);
                        state.screen = SCREEN_PLAYING;
                    }
                }
                if (IsKeyPressed(KEY_NINE) && level_count > 8) {
                    state.current_level = 8;
                    if (LoadMapFromConf(&state.map, level_list[8].filename)) {
                        InitGame(&state, state.current_level);
                        state.screen = SCREEN_PLAYING;
                    }
                }

                // Mouse wheel scroll for menu
                i32 wheel = (i32)GetMouseWheelMove();
                if (wheel != 0) {
                    menu_scroll -= wheel;
                    if (menu_scroll < 0) menu_scroll = 0;
                    if (menu_scroll > level_count - visible_items) menu_scroll = level_count - visible_items;
                }

                // Mouse click on level buttons
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    Vector2 mouse = GetMousePosition();
                    i32 start_y = 220;
                    for (i32 i = menu_scroll; i < menu_scroll + visible_items && i < level_count; i++) {
                        i32 button_y = start_y + (i - menu_scroll) * menu_item_h;
                        i32 button_w = 400;
                        i32 button_x = SCREEN_WIDTH / 2 - button_w / 2;

                        Rectangle button = {button_x, button_y, button_w, menu_item_h - 4};
                        if (CheckCollisionPointRec(mouse, button)) {
                            state.current_level = i;
                            if (LoadMapFromConf(&state.map, level_list[i].filename)) {
                                InitGame(&state, state.current_level);
                                state.screen = SCREEN_PLAYING;
                            }
                            break;
                        }
                    }
                }
                break;
            }

            case SCREEN_PLAYING:
                UpdateGame(&state, dt);

                if (state.screen != SCREEN_PLAYING) {
                    break;
                }

                // Check win/lose conditions
                if (state.base_life <= 0) {
                    state.screen = SCREEN_GAME_OVER;
                } else if (AllWavesComplete(&state.wave_mgr) && state.enemy_count == 0) {
                    state.screen = SCREEN_VICTORY;
                }
                break;

            case SCREEN_GAME_OVER:
                if (IsKeyPressed(KEY_R)) {
                    // Reload map from config if levels exist
                    if (level_count > 0 && state.current_level < level_count) {
                        LoadMapFromConf(&state.map, level_list[state.current_level].filename);
                    }
                    InitGame(&state, state.current_level);
                    state.screen = SCREEN_PLAYING;
                }
                if (IsKeyPressed(KEY_M)) {
                    state.screen = SCREEN_MENU;
                }
                break;

            case SCREEN_VICTORY:
                if (IsKeyPressed(KEY_N) && state.current_level < level_count - 1) {
                    state.current_level++;
                    if (LoadMapFromConf(&state.map, level_list[state.current_level].filename)) {
                        InitGame(&state, state.current_level);
                        state.screen = SCREEN_PLAYING;
                    }
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

                // Level count
                char count_text[64];
                snprintf(count_text, sizeof(count_text), "%d level(s) available", level_count);
                DrawTextEx(font, count_text, (Vector2){SCREEN_WIDTH/2 - 100, 190}, 16, 1, LIGHTGRAY);

                // Level buttons (scrollable)
                i32 start_y = 220;
                i32 button_w = 400;
                i32 button_h = menu_item_h - 4;

                for (i32 i = menu_scroll; i < menu_scroll + visible_items && i < level_count; i++) {
                    i32 button_y = start_y + (i - menu_scroll) * menu_item_h;
                    i32 button_x = SCREEN_WIDTH / 2 - button_w / 2;

                    Rectangle button = {button_x, button_y, button_w, button_h};
                    Color button_color = DARKGRAY;

                    Vector2 mouse = GetMousePosition();
                    if (CheckCollisionPointRec(mouse, button)) {
                        button_color = GRAY;
                    }

                    DrawRectangleRec(button, button_color);
                    DrawRectangleLinesEx(button, 2, WHITE);

                    // Level name
                    DrawTextEx(font, level_list[i].name,
                               (Vector2){button_x + 15, button_y + 8},
                               20, 1, WHITE);

                    // Keyboard shortcut
                    if (i < 9) {
                        char shortcut[4] = {i + '1', '\0'};
                        DrawTextEx(font, shortcut,
                                   (Vector2){button_x + button_w - 35, button_y + 18},
                                   24, 1, YELLOW);
                    }
                }

                // Scroll indicator
                if (level_count > visible_items) {
                    i32 bar_x = SCREEN_WIDTH / 2 + button_w / 2 + 10;
                    i32 bar_y = start_y;
                    i32 bar_h = visible_items * menu_item_h;
                    f32 thumb_h = (f32)visible_items / level_count * bar_h;
                    f32 thumb_y = bar_y + (f32)menu_scroll / level_count * bar_h;

                    DrawRectangle(bar_x, bar_y, 6, bar_h, (Color){60, 60, 60, 255});
                    DrawRectangle(bar_x, (i32)thumb_y, 6, (i32)thumb_h, LIGHTGRAY);
                }

                DrawTextEx(font, "Click a level or press 1-9",
                          (Vector2){SCREEN_WIDTH/2 - 140, SCREEN_HEIGHT - 60}, 18, 1, LIGHTGRAY);
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
                if (state.current_level < level_count - 1) {
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
