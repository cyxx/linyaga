
#include "animation.h"

#define MAX_LAYERS 2048

static struct layer_t _layers[MAX_LAYERS];
static struct layer_t *_next_free_layer;

#define MAX_FRAMES 512

static struct frame_t _frames[MAX_FRAMES];
static struct frame_t *_next_free_frame;

#define MAX_ANIMATIONS 64

static struct anim_t _animations[MAX_ANIMATIONS];
static struct anim_t *_next_free_animation;


static struct layer_t *FindFreeLayer() {
	struct layer_t *layer = _next_free_layer;
	if (layer) {
		_next_free_layer = layer->next_free;
		layer->next_free = 0;
	} else {
		fprintf(stderr, "FindFreeLayer MAX_LAYERS\n");
	}
	return layer;
}

static void FreeLayer(struct layer_t *layer) {
	layer->next_free = _next_free_layer;
	_next_free_layer = layer;
}

static struct frame_t *FindFreeFrame() {
	struct frame_t *frame = _next_free_frame;
	if (frame) {
		_next_free_frame = frame->next_free;
		frame->next_free = 0;
	} else {
		fprintf(stderr, "FindFreeFrame MAX_FRAMES\n");
	}
	return frame;
}

static void FreeFrame(struct frame_t *frame) {
	frame->next_frame = 0;
	frame->next_free = _next_free_frame;
	_next_free_frame = frame;
}

static struct anim_t *FindFreeAnimation() {
	struct anim_t *animation = _next_free_animation;
	if (animation) {
		_next_free_animation = animation->next_free;
		animation->next_free = 0;
	} else {
		fprintf(stderr, "FindFreeAnimation MAX_ANIMATIONS\n");
	}
	return animation;
}

static void FreeAnimation(struct anim_t *animation) {
	animation->next_free = _next_free_animation;
	_next_free_animation = animation;
}

int Animation_Init() {
	_next_free_layer = &_layers[0];
	for (int i = 0; i < MAX_LAYERS - 1; ++i) {
		_layers[i].next_free = &_layers[i + 1];
	}
	_next_free_frame = &_frames[0];
	for (int i = 0; i < MAX_FRAMES - 1; ++i) {
		_frames[i].next_free = &_frames[i + 1];
	}
	_next_free_animation = &_animations[0];
	for (int i = 0; i < MAX_ANIMATIONS - 1; ++i) {
		_animations[i].next_free = &_animations[i + 1];
	}
	return 0;
}

int Animation_Fini() {
	return 0;
}

static struct {
	const char *ext;
	int (*load)(FILE *, struct anim_t *, FreeFrameProc, FreeLayerProc);
} _animationFormats[] = {
	{ "mng", Animation_Load_MNG },
	{ "rle", Animation_Load_RLE },
	{ 0, 0 }
};

int Animation_Load(FILE *fp, const char *name) {
	struct anim_t *animation = FindFreeAnimation();
	if (animation) {
		for (int i = 0; _animationFormats[i].ext; ++i) {
			if (strcasecmp(_animationFormats[i].ext, name) == 0) {
				if (_animationFormats[i].load(fp, animation, FindFreeFrame, FindFreeLayer) < 0) {
					break;
				}
				return animation - _animations;
			}
		}
		fprintf(stderr, "Invalid animation '%s'\n", name);
		FreeAnimation(animation);
	}
	return -1;
}

int Animation_Free(int anim) {
	for (struct frame_t *frame = _animations[anim].first_frame; frame; ) {
		struct frame_t *next_frame = frame->next_frame;
		for (struct layer_t *layer = frame->first_layer; layer; ) {
			struct layer_t *next_layer = layer->next_layer;
			free(layer->rgba);
			memset(layer, 0, sizeof(struct layer_t));
			FreeLayer(layer);
			layer = next_layer;
		}
		memset(frame, 0, sizeof(struct frame_t));
		FreeFrame(frame);
		frame = next_frame;
	}
	FreeAnimation(&_animations[anim]);
	return 0;
}

int Animation_GetFramesCount(int anim) {
	return _animations[anim].frames_count;
}

int Animation_Seek(int anim, int frame_num) {
	struct frame_t *frame = _animations[anim].first_frame;
	for (; frame_num-- != 0 && frame; frame = frame->next_frame);
	assert(frame);
	_animations[anim].current_frame = frame;
	return 0;
}

int Animation_SetLayer(int anim, int frame_num, const char *name, int state) {
	struct frame_t *frame;
	if (frame_num < 0) {
		frame = _animations[anim].current_frame;
	} else {
		frame = _animations[anim].first_frame;
		for (; frame_num-- != 0 && frame; frame = frame->next_frame);
	}
	assert(frame);
	struct layer_t *layer = frame->first_layer;
	for (; layer; layer = layer->next_layer) {
		if (strcasecmp(layer->name, name) == 0) {
			layer->state = state;
			break;
		}
	}
	return 0;
}

static uint32_t blend(uint32_t a, uint32_t b) {
	const uint8_t alpha = b >> 24;
	switch (alpha) {
	case 0:
		return a;
	case 255:
		return b;
	}
	uint32_t rb1 = ((a & 0xFF00FF) * (255 - alpha)) >>  8;
	uint32_t rb2 = ((b & 0xFF00FF) * alpha) >>  8;
	uint32_t g1  = ((a & 0x00FF00) * (255 - alpha)) >> 8;
	uint32_t g2  = ((b & 0x00FF00) * alpha) >> 8;
	return ((rb1 | rb2) & 0xFF00FF) | ((g1 | g2) & 0x00FF00);
}

static void draw_layer(struct layer_t *layer, struct surface_t *s, int x, int y, int *x1, int *y1, int *x2, int *y2) {
	const uint32_t *src = layer->rgba;
	int w = layer->w;
	if (x < 0) {
		src -= x;
		w += x;
		x = 0;
	}
	if (x + w > s->w) {
		w = s->w - x;
	}
	if (w <= 0) {
		return;
	}
	int h = layer->h;
	if (y < 0) {
		src -= y * layer->w;
		h += y;
		y = 0;
	}
	if (y + h > s->h) {
		h = s->h - y;
	}
	if (h <= 0) {
		return;
	}
	uint32_t *dst = s->buffer + y * s->w + x;
	for (int j = 0; j < h; ++j) {
		for (int i = 0; i < w; ++i) {
			dst[i] = blend(dst[i], src[i]);
		}
		dst += s->w;
		src += layer->w;
	}
	if (x < *x1) {
		*x1 = x;
	}
	if (y < *y1) {
		*y1 = y;
	}
	if (x + w > *x2) {
		*x2 = x + w;
	}
	if (y + h > *y2) {
		*y2 = y + h;
	}
}

static int is_phoneme(struct layer_t *layer, int mask) {
	if (layer->mask == 0) {
		return 0;
	}
	return (layer->mask & mask) == 0;
}

int Animation_Draw(int anim, struct surface_t *s, int dx, int dy, int mask, int *x, int *y, int *w, int *h) {
	struct frame_t *frame = _animations[anim].current_frame;
	assert(frame);
	int x1 = 640 - 1;
	int y1 = 480 - 1;
	int x2 = 0;
	int y2 = 0;
	struct layer_t *layer = frame->first_layer;
	for (; layer; layer = layer->next_layer) {
		if (layer->state == 0 || is_phoneme(layer, mask)) {
			continue;
		}
		const int lx = layer->x + dx;
		const int ly = layer->y + dy;
		draw_layer(layer, s, lx, ly, &x1, &y1, &x2, &y2);
	}
	*x = x1;
	*y = y1;
	*w = x2 - x1 + 1;
	*h = y2 - y1 + 1;
	return 0;
}
