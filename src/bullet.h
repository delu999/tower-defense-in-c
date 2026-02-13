#ifndef BULLET_H
#define BULLET_H

#include "game.h"
#include "raylib.h"

// Bullet functions
int SpawnBullet(GameState *state, BulletType type, Vector2 position,
                int target_enemy_id, float damage);
void UpdateBullets(GameState *state, float dt);
void DrawBullets(const GameState *state);
void RemoveBullet(GameState *state, int index);

#endif // BULLET_H
