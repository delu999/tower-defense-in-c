#ifndef WAVE_H
#define WAVE_H

#include "game.h"

// Wave functions
void InitWaveManager(WaveManager *mgr, int level);
void UpdateWaveManager(GameState *state, float dt);
void StartNextWave(GameState *state);
bool AllWavesComplete(const WaveManager *mgr);

#endif // WAVE_H
