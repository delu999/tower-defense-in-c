#ifndef UI_H
#define UI_H

#include "game.h"
#include "raylib.h"

// UI state
struct UIState {
    TowerType selected_tower;   // -1 if nothing selected
    int selected_tower_index;   // Tower being inspected (-1 if none)
    char alert_message[128];
    float alert_timer;
    bool placing_tower;
};

// UI functions
void InitUI(UIState *ui);
void UpdateUI(UIState *ui, GameState *game, float dt);
void DrawUI(const UIState *ui, const GameState *game, Texture2D spritesheet, Font font);

// Alert system
void ShowAlert(UIState *ui, const char *message);

#endif // UI_H
