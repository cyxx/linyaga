
#ifndef MIXER_H__
#define MIXER_H__

#include "intern.h"

typedef void (*MixerLockProc)(int);

int Mixer_Init(int hz, MixerLockProc lockProc);
int Mixer_Fini();

int Mixer_PlayMp3(FILE *fp);
int Mixer_PlayWav(FILE *fp);
int Mixer_Stop(int channel);
int Mixer_IsPlaying(int channel);
int Mixer_SetVolume(int channel, float volume);
int Mixer_SetPan(int channel, float pan);
int Mixer_MixStereoS16(int16_t *samples, int len);

#endif
