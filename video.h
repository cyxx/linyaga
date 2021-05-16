
#ifndef VIDEO_H__
#define VIDEO_H__

#include "intern.h"

int Video_Init();
int Video_Fini();

int Video_IsVideoPlaying(int video);
int Video_PlayVideo(FILE *fp);
int Video_StopVideo(int video);
int Video_RenderVideo(int video);

#endif
