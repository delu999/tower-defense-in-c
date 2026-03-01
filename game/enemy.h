#ifndef ENEMY_H
#define ENEMY_H

#include "game.h"
#include "raylib.h"

// Enemy functions
i32 SpawnEnemy(GameState *state, EnemyType type, Vector2 spawn_pos, f32 difficulty);
void UpdateEnemies(GameState *state, f32 dt);
void DrawEnemies(const GameState *state, Texture2D spritesheet);
void RemoveEnemy(GameState *state, i32 index);

// Enemy helper functions
void RecalculateEnemyPath(Enemy *enemy, const Map *map);
void DamageEnemy(Enemy *enemy, f32 damage);

#endif // ENEMY_H
