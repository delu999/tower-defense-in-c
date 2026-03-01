#ifndef WAVE_H
#define WAVE_H

#include "game.h"

// Wave functions
void InitWaveManager(WaveManager *mgr, WaveEntry *waves, i32 *wave_sizes, i32 total_waves);
void UpdateWaveManager(GameState *state, f32 dt);
void StartNextWave(GameState *state);
bool AllWavesComplete(const WaveManager *mgr);

#endif // WAVE_H
