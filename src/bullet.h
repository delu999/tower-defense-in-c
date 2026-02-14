#ifndef BULLET_H
#define BULLET_H

#include "game.h"
#include "raylib.h"

// Bullet functions
i32 SpawnBullet(GameState *state, BulletType type, Vector2 position,
                i32 target_enemy_id, f32 damage);
void UpdateBullets(GameState *state, f32 dt);
void DrawBullets(const GameState *state);
void RemoveBullet(GameState *state, i32 index);

#endif // BULLET_H
