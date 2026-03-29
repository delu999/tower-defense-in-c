#ifndef MAP_H
#define MAP_H

#include "game.h"
#include "raylib.h"
#include <stdbool.h>

// Map functions
bool LoadMapFromConf(Map *map, const char *filename);
bool LoadMapAssets(void);
void UnloadMapAssets(void);
Texture2D GetExtraTileset(void);
void DrawMap(const Map *map, Texture2D spritesheet);
bool IsBuildable(const Map *map, i32 grid_x, i32 grid_y);
bool IsWalkable(const Map *map, i32 grid_x, i32 grid_y);
TileType GetTileType(const Map *map, i32 grid_x, i32 grid_y);
void SetTileType(Map *map, i32 grid_x, i32 grid_y, TileType type);
Vector2 GridToWorld(i32 grid_x, i32 grid_y);
void WorldToGrid(Vector2 world_pos, i32 *grid_x, i32 *grid_y);

#endif // MAP_H
