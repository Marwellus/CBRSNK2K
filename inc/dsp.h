#ifndef DSP_H
#define DSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "inc/types.h"

bool DSP_Test();
void DSP_Dispose();

void DSP_PlayLeft(char* filename);
void DSP_PlayRight(char* filename);
bool DSP_Play(char *filename, bool long_play);
bool DSP_Is_Playing();
void DSP_Stop();
void DSP_Set_Volume(int percent);
void DSP_Set_Distortion(int percent);

#ifdef __cplusplus
}
#endif

#endif
