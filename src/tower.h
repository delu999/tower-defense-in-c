#ifndef TOWER_H
#define TOWER_H

#include "game.h"
#include "raylib.h"

// Tower functions
int PlaceTower(GameState *state, TowerType type, int grid_x, int grid_y);
void RemoveTower(GameState *state, int index);
void UpdateTowers(GameState *state, float dt);
void DrawTowers(const GameState *state, Texture2D spritesheet);

// Tower helper functions
int FindNearestEnemy(const GameState *state, Vector2 tower_pos, float range);
void FireBullet(GameState *state, int tower_index);

#endif // TOWER_H
