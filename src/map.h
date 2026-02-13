#ifndef MAP_H
#define MAP_H

#include "game.h"
#include "raylib.h"
#include <stdbool.h>

// Map functions
void InitMap(Map *map, int level);
void DrawMap(const Map *map, Texture2D spritesheet);
bool IsBuildable(const Map *map, int grid_x, int grid_y);
bool IsWalkable(const Map *map, int grid_x, int grid_y);
TileType GetTileType(const Map *map, int grid_x, int grid_y);
void SetTileType(Map *map, int grid_x, int grid_y, TileType type);
Vector2 GridToWorld(int grid_x, int grid_y);
void WorldToGrid(Vector2 world_pos, int *grid_x, int *grid_y);

#endif // MAP_H
