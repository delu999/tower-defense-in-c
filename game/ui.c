#include "ui.h"
#include "tower.h"
#include "map.h"
#include "wave.h"
#include "content.h"
#include "config.h"
#include <string.h>
#include <stdio.h>

void InitUI(UIState *ui) {
    ui->selected_tower = -1;
    ui->selected_tower_index = -1;
    ui->placing_tower = false;
    ui->alert_timer = 0;
    ui->alert_message[0] = '\0';
}

void ShowAlert(UIState *ui, const char *message) {
    strncpy(ui->alert_message, message, sizeof(ui->alert_message) - 1);
    ui->alert_message[sizeof(ui->alert_message) - 1] = '\0';
    ui->alert_timer = 3.0f;
}

// Check if mouse is within the map area
static bool IsMouseInMap(Vector2 mouse_pos) {
    return mouse_pos.x >= MAP_OFFSET_X &&
           mouse_pos.x < MAP_OFFSET_X + MAP_DISPLAY_COLS * TILE_SIZE &&
           mouse_pos.y >= MAP_OFFSET_Y &&
           mouse_pos.y < MAP_OFFSET_Y + MAP_HEIGHT * TILE_SIZE;
}

// Shop button layout constants
#define SHOP_COLS 2
#define SHOP_ROWS 3
#define SHOP_BTN_SIZE 100
#define SHOP_BTN_PAD 10
#define SHOP_HEADER_H 50
#define SHOP_ICON_SIZE 64

#define CTRL_BTN_W 92
#define CTRL_BTN_H 28
#define CTRL_BTN_GAP 8

static Rectangle GetShopButton(i32 index) {
    i32 col = index % SHOP_COLS;
    i32 row = index / SHOP_COLS;
    f32 btn_area_w = SHOP_COLS * SHOP_BTN_SIZE + (SHOP_COLS - 1) * SHOP_BTN_PAD;
    f32 start_x = SHOP_X + (SHOP_WIDTH - btn_area_w) / 2;
    f32 start_y = MAP_OFFSET_Y + SHOP_HEADER_H;
    return (Rectangle){
        start_x + col * (SHOP_BTN_SIZE + SHOP_BTN_PAD),
        start_y + row * (SHOP_BTN_SIZE + 40 + SHOP_BTN_PAD),
        SHOP_BTN_SIZE,
        SHOP_BTN_SIZE + 40
    };
}

static Rectangle GetStartWaveButtonRect(void) {
    return (Rectangle){
        SHOP_X + 8,
        (HUD_HEIGHT - CTRL_BTN_H) / 2.0f,
        CTRL_BTN_W,
        CTRL_BTN_H
    };
}

static Rectangle GetPauseButtonRect(void) {
    return (Rectangle){
        SHOP_X + 8 + CTRL_BTN_W + CTRL_BTN_GAP,
        (HUD_HEIGHT - CTRL_BTN_H) / 2.0f,
        CTRL_BTN_W,
        CTRL_BTN_H
    };
}

static Rectangle GetMenuButtonRect(void) {
    return (Rectangle){
        SHOP_X + 8 + (CTRL_BTN_W + CTRL_BTN_GAP) * 2,
        (HUD_HEIGHT - CTRL_BTN_H) / 2.0f,
        CTRL_BTN_W,
        CTRL_BTN_H
    };
}

static bool CanStartWave(const GameState *game) {
    return !game->paused &&
           !game->wave_mgr.wave_active &&
           game->wave_mgr.current_wave < game->wave_mgr.total_waves;
}

static void SelectShopTower(UIState *ui, const GameState *game, i32 slot_index) {
    if (slot_index < 0 || slot_index >= SHOP_TOWER_COUNT) {
        return;
    }

    i32 tower_type = game->shop_tower_order[slot_index];
    if (ui->selected_tower == tower_type) {
        ui->selected_tower = -1;
        ui->placing_tower = false;
    } else {
        ui->selected_tower = tower_type;
        ui->placing_tower = true;
        ui->selected_tower_index = -1;
    }
}

static void DrawControlButton(Font font, Rectangle rect, const char *label, bool enabled, bool active) {
    Color bg = enabled ? (Color){75, 75, 75, 255} : (Color){50, 50, 50, 255};
    Color border = active ? YELLOW : (enabled ? LIGHTGRAY : GRAY);
    Color text = enabled ? WHITE : GRAY;

    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, 2, border);

    Vector2 text_size = MeasureTextEx(font, label, 16, 1);
    DrawTextEx(font, label,
               (Vector2){rect.x + (rect.width - text_size.x) / 2, rect.y + (rect.height - text_size.y) / 2},
               16, 1, text);
}

void UpdateUI(UIState *ui, GameState *game, f32 dt) {
    if (IsKeyPressed(KEY_P)) {
        game->paused = !game->paused;
    }

    if (IsKeyPressed(KEY_M)) {
        game->paused = false;
        game->screen = SCREEN_MENU;
        ui->placing_tower = false;
        ui->selected_tower = -1;
        ui->selected_tower_index = -1;
        return;
    }

    if (ui->alert_timer > 0) {
        ui->alert_timer -= dt;
    }

    Vector2 mouse_pos = GetMousePosition();
    i32 mouse_grid_x, mouse_grid_y;
    WorldToGrid(mouse_pos, &mouse_grid_x, &mouse_grid_y);

    // Handle shop button clicks
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Rectangle start_btn = GetStartWaveButtonRect();
        Rectangle pause_btn = GetPauseButtonRect();
        Rectangle menu_btn = GetMenuButtonRect();

        if (CheckCollisionPointRec(mouse_pos, start_btn)) {
            if (CanStartWave(game)) {
                StartNextWave(game);
            }
            return;
        }

        if (CheckCollisionPointRec(mouse_pos, pause_btn)) {
            game->paused = !game->paused;
            return;
        }

        if (CheckCollisionPointRec(mouse_pos, menu_btn)) {
            game->paused = false;
            game->screen = SCREEN_MENU;
            ui->placing_tower = false;
            ui->selected_tower = -1;
            ui->selected_tower_index = -1;
            return;
        }

        if (game->paused) {
            return;
        }

        for (i32 i = 0; i < SHOP_TOWER_COUNT; i++) {
            Rectangle btn = GetShopButton(i);
            if (CheckCollisionPointRec(mouse_pos, btn)) {
                SelectShopTower(ui, game, i);
                return;
            }
        }
    }

    if (game->paused) {
        return;
    }

    for (i32 i = 0; i < SHOP_TOWER_COUNT && i < 9; i++) {
        KeyboardKey key = (KeyboardKey)(KEY_ONE + i);
        if (IsKeyPressed(key)) {
            SelectShopTower(ui, game, i);
            return;
        }
    }

    // Handle tower placement (only in map area)
    if (ui->placing_tower && ui->selected_tower >= 0) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && IsMouseInMap(mouse_pos)) {
            i32 result = PlaceTower(game, (TowerType)ui->selected_tower, mouse_grid_x, mouse_grid_y);
            if (result < 0) {
                if (game->currency < game->tower_config[ui->selected_tower].stats.cost) {
                    ShowAlert(ui, "Not enough currency!");
                } else {
                    ShowAlert(ui, "Can't place tower here!");
                }
            } else {
                ui->placing_tower = false;
                ui->selected_tower = -1;
            }
        }

        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) || IsKeyPressed(KEY_ESCAPE)) {
            ui->placing_tower = false;
            ui->selected_tower = -1;
        }
    }

    // Handle tower inspection (click on existing tower in map)
    if (!ui->placing_tower && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && IsMouseInMap(mouse_pos)) {
        i32 found = -1;
        for (i32 i = 0; i < game->tower_count; i++) {
            if (game->towers[i].active &&
                game->towers[i].grid_x == mouse_grid_x &&
                game->towers[i].grid_y == mouse_grid_y) {
                found = i;
                break;
            }
        }
        ui->selected_tower_index = found;
    }

    // Handle tower deletion
    if (ui->selected_tower_index >= 0) {
        if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
            RemoveTower(game, ui->selected_tower_index);
            ui->selected_tower_index = -1;
            ShowAlert(ui, "Tower removed");
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            ui->selected_tower_index = -1;
        }
    }
}

// Helper to draw a sprite from the spritesheet
static void DrawSpriteRect(Texture2D spritesheet, i32 sprite_id, Rectangle dest) {
    i32 src_x = (sprite_id % SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
    i32 src_y = (sprite_id / SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
    Rectangle src = {src_x, src_y, SPRITE_TILE_SIZE, SPRITE_TILE_SIZE};
    DrawTexturePro(spritesheet, src, dest, (Vector2){0, 0}, 0, WHITE);
}

void DrawUI(const UIState *ui, const GameState *game, Texture2D spritesheet, Font font) {
    // --- Placement preview ---
    if (ui->placing_tower && ui->selected_tower >= 0) {
        Vector2 mouse_pos = GetMousePosition();
        i32 mouse_grid_x, mouse_grid_y;
        WorldToGrid(mouse_pos, &mouse_grid_x, &mouse_grid_y);

        if (IsMouseInMap(mouse_pos)) {
            Vector2 world_pos = GridToWorld(mouse_grid_x, mouse_grid_y);
            bool can_place = IsBuildable(&game->map, mouse_grid_x, mouse_grid_y);

            // Range circle
            f32 range = game->tower_config[ui->selected_tower].stats.range * TILE_SIZE;
            Color range_color = can_place ? ColorAlpha(GREEN, 0.2f) : ColorAlpha(RED, 0.2f);
            DrawCircle((i32)world_pos.x, (i32)world_pos.y, range, range_color);
            DrawCircleLines((i32)world_pos.x, (i32)world_pos.y, range,
                           can_place ? GREEN : RED);

            // Tower base preview
            i32 base_id = game->tower_config[ui->selected_tower].base_sprite_id;
            Rectangle dest = {
                world_pos.x - TILE_SIZE / 2,
                world_pos.y - TILE_SIZE / 2,
                TILE_SIZE, TILE_SIZE
            };

            // Draw base sprite
            DrawSpriteRect(spritesheet, base_id, dest);

            // Draw gun sprite on top (if applicable)
            i32 gun_id = game->tower_config[ui->selected_tower].gun_sprite_id;
            if (gun_id >= 0) {
                DrawSpriteRect(spritesheet, gun_id, dest);
            }
        }
    }

    // --- Range circle for inspected tower ---
    if (ui->selected_tower_index >= 0 && ui->selected_tower_index < game->tower_count) {
        const Tower *tower = &game->towers[ui->selected_tower_index];
        if (tower->active) {
            f32 range = game->tower_config[tower->type].stats.range * TILE_SIZE;
            DrawCircle((i32)tower->position.x, (i32)tower->position.y, range,
                      ColorAlpha(BLUE, 0.2f));
            DrawCircleLines((i32)tower->position.x, (i32)tower->position.y, range, BLUE);
        }
    }

    // --- Shop panel on the right ---
    // Background
    DrawRectangle(SHOP_X, 0, SHOP_WIDTH, SCREEN_HEIGHT, (Color){80, 80, 80, 255});

    // Header
    DrawRectangle(SHOP_X, 0, SHOP_WIDTH, MAP_OFFSET_Y + SHOP_HEADER_H, (Color){50, 50, 50, 255});
    const char *shop_title = "Shop";
    Vector2 title_size = MeasureTextEx(font, shop_title, 32, 1);
    DrawTextEx(font, shop_title,
              (Vector2){SHOP_X + (SHOP_WIDTH - title_size.x) / 2, MAP_OFFSET_Y + 10},
              32, 1, WHITE);

    // Top-right gameplay controls
    DrawControlButton(font, GetStartWaveButtonRect(), "Start Wave", CanStartWave(game), false);
    DrawControlButton(font, GetPauseButtonRect(), game->paused ? "Resume" : "Pause", true, game->paused);
    DrawControlButton(font, GetMenuButtonRect(), "Menu", true, false);

    // Tower buttons (2x3 grid)
    for (i32 i = 0; i < SHOP_TOWER_COUNT; i++) {
        i32 tower_type = game->shop_tower_order[i];
        Rectangle btn = GetShopButton(i);
        bool selected = (ui->selected_tower == tower_type);
        bool affordable = game->currency >= game->tower_config[tower_type].stats.cost;

        // Button background
        Color bg = selected ? (Color){60, 120, 180, 255} : (Color){100, 100, 100, 255};
        DrawRectangleRec(btn, bg);
        if (selected) {
            DrawRectangleLinesEx(btn, 3, GOLD);
        }

        // Tower icon (centered in top portion of button)
        f32 icon_x = btn.x + (btn.width - SHOP_ICON_SIZE) / 2;
        f32 icon_y = btn.y + 8;
        Rectangle icon_dest = {icon_x, icon_y, SHOP_ICON_SIZE, SHOP_ICON_SIZE};

        // Draw base sprite
        DrawSpriteRect(spritesheet, game->tower_config[tower_type].base_sprite_id, icon_dest);
        // Draw gun sprite on top (if applicable)
        if (game->tower_config[tower_type].gun_sprite_id >= 0) {
            DrawSpriteRect(spritesheet, game->tower_config[tower_type].gun_sprite_id, icon_dest);
        }

        // Tower name
        const char *tower_name = GetTowerName(game, (TowerType)tower_type);
        Vector2 name_size = MeasureTextEx(font, tower_name, 18, 1);
        DrawTextEx(font, tower_name,
                  (Vector2){btn.x + (btn.width - name_size.x) / 2, btn.y + SHOP_BTN_SIZE + 2},
                  18, 1, WHITE);

        // Cost
        char cost_str[16];
        snprintf(cost_str, sizeof(cost_str), "$ %d", game->tower_config[tower_type].stats.cost);
        Vector2 cost_size = MeasureTextEx(font, cost_str, 18, 1);
        Color cost_color = affordable ? GREEN : RED;
        DrawTextEx(font, cost_str,
                  (Vector2){btn.x + (btn.width - cost_size.x) / 2, btn.y + SHOP_BTN_SIZE + 20},
                  18, 1, cost_color);
    }

    // "Select an item" text at the bottom of shop
    if (ui->selected_tower < 0 && ui->selected_tower_index < 0) {
        const char *hint = "Select an item";
        Vector2 hint_size = MeasureTextEx(font, hint, 22, 1);
        DrawTextEx(font, hint,
                  (Vector2){SHOP_X + (SHOP_WIDTH - hint_size.x) / 2, SCREEN_HEIGHT - 60},
                  22, 1, LIGHTGRAY);
    }

    // Tower info when inspecting
    if (ui->selected_tower_index >= 0 && ui->selected_tower_index < game->tower_count) {
        const Tower *tower = &game->towers[ui->selected_tower_index];
        if (tower->active) {
            char buf[128];
            snprintf(buf, sizeof(buf), "%s Tower", GetTowerName(game, tower->type));
            Vector2 info_size = MeasureTextEx(font, buf, 20, 1);
            DrawTextEx(font, buf,
                      (Vector2){SHOP_X + (SHOP_WIDTH - info_size.x) / 2, SCREEN_HEIGHT - 80},
                      20, 1, YELLOW);
            DrawTextEx(font, "DEL to remove",
                      (Vector2){SHOP_X + 30, SCREEN_HEIGHT - 50},
                      16, 1, LIGHTGRAY);
        }
    }

    // Alert message (centered over map)
    if (ui->alert_timer > 0) {
        f32 alpha = (ui->alert_timer < 1.0f) ? ui->alert_timer : 1.0f;
        Color alert_color = ColorAlpha(RED, alpha);
        Vector2 alert_size = MeasureTextEx(font, ui->alert_message, 24, 1);
        f32 map_center_x = MAP_OFFSET_X + (MAP_DISPLAY_COLS * TILE_SIZE) / 2.0f;
        DrawTextEx(font, ui->alert_message,
                  (Vector2){map_center_x - alert_size.x / 2, MAP_OFFSET_Y + 20},
                  24, 1, alert_color);
    }
}
