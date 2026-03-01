#ifndef CONTENT_H
#define CONTENT_H

#include "game.h"
#include <stdbool.h>

bool LoadEnemyConfig(GameState *state, const char *filename);
bool LoadTurretConfig(GameState *state, const char *filename);
bool LoadLevelConfig(GameState *state, const char *filename);

const char *GetEnemyName(const GameState *state, EnemyType type);
const char *GetTowerName(const GameState *state, TowerType type);

#endif // CONTENT_H
