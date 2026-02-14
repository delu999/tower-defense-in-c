#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "game.h"
#include "raylib.h"
#include <stdbool.h>

// A* pathfinding: finds path from start to any of the goals
// Returns path length (0 if no path found)
i32 FindPath(const Map *map, Vector2 start, const Vector2 *goals, i32 goal_count,
             Vector2 *out_path, i32 max_path_len);

// Flood fill validation: checks if all spawns can reach any base
// Returns true if all spawn points have a valid path to at least one base
bool ValidatePaths(const Map *map);

// Helper: visualize a path (for debugging)
void DrawPath(const Vector2 *path, i32 path_len, Color color);

#endif // PATHFINDING_H
