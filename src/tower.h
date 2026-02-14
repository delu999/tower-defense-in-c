#ifndef TOWER_H
#define TOWER_H

#include "game.h"
#include "raylib.h"

// Tower functions
i32 PlaceTower(GameState *state, TowerType type, i32 grid_x, i32 grid_y);
void RemoveTower(GameState *state, i32 index);
void UpdateTowers(GameState *state, f32 dt);
void DrawTowers(const GameState *state, Texture2D spritesheet);

// Tower helper functions
i32 FindNearestEnemy(const GameState *state, Vector2 tower_pos, f32 range);
void FireBullet(GameState *state, i32 tower_index);

#endif // TOWER_H
