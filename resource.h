
#ifndef RESOURCE_H__
#define RESOURCE_H__

#include "intern.h"

void Resource_Init();
void Resource_Fini();

int Resource_Exists(const char *name);
FILE *Resource_Open(const char *name);
int Resource_LoadAnimation(const char *name);
void Resource_FreeAnimation(int num);
int Resource_GetAnimationIndex(int num);

#endif // RESOURCE_H__
