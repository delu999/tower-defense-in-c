#ifndef ENEMY_H
#define ENEMY_H

#include "game.h"
#include "raylib.h"

// Enemy functions
int SpawnEnemy(GameState *state, EnemyType type, Vector2 spawn_pos, float difficulty);
void UpdateEnemies(GameState *state, float dt);
void DrawEnemies(const GameState *state, Texture2D spritesheet);
void RemoveEnemy(GameState *state, int index);

// Enemy helper functions
void RecalculateEnemyPath(Enemy *enemy, const Map *map);
void DamageEnemy(Enemy *enemy, float damage);

#endif // ENEMY_H
