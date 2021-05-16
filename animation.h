
#ifndef ANIMATION_H__
#define ANIMATION_H__

#include "intern.h"

struct surface_t {
	uint32_t *buffer;
	int w, h;
};

struct layer_t {
	int x, y, w, h;
	int mask, state;
	char name[64];
	uint32_t *rgba;
	struct layer_t *next_layer;
	struct layer_t *next_free;
};

struct frame_t {
	int layers_count;
	struct layer_t *first_layer;
	struct frame_t *next_frame;
	struct frame_t *next_free;
};

struct anim_t {
	int frames_count;
	struct frame_t *first_frame;
	struct frame_t *current_frame;
	struct anim_t *next_free;
};

typedef struct layer_t *(*FreeLayerProc)();
typedef struct frame_t *(*FreeFrameProc)();

int Animation_Load_MNG(FILE *, struct anim_t *, FreeFrameProc, FreeLayerProc);
int Animation_Load_RLE(FILE *, struct anim_t *, FreeFrameProc, FreeLayerProc);

int Animation_Init();
int Animation_Fini();

int Animation_Load(FILE *fp, const char *name);
int Animation_Free(int anim);

int Animation_GetFramesCount(int anim);
struct layer_t *Animation_GetLayer(int anim, int frame, int layer);

int Animation_Seek(int anim, int frame);
int Animation_SetLayer(int anim, int frame, const char *name, int state);
int Animation_Draw(int anim, struct surface_t *s, int dx, int dy, int mask, int *x, int *y, int *w, int *h);

#endif
