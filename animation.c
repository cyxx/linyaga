
#include "animation.h"

#define MAX_LAYERS 2048

static struct layer_t _layers[MAX_LAYERS];
static struct layer_t *_next_free_layer;
static int _total_layers_count;

#define MAX_FRAMES 1024

static struct frame_t _frames[MAX_FRAMES];
static struct frame_t *_next_free_frame;
static int _total_frames_count;

#define MAX_ANIMATIONS 64

static struct anim_t _animations[MAX_ANIMATIONS];
static struct anim_t *_next_free_animation;
static int _total_animations_count;


static struct layer_t *find_free_layer() {
	struct layer_t *layer = _next_free_layer;
	if (layer) {
		_next_free_layer = layer->next_free;
		layer->next_free = 0;
		++_total_layers_count;
	} else {
		fprintf(stderr, "find_free_layer MAX_LAYERS\n");
	}
	return layer;
}

static void free_layer(struct layer_t *layer) {
	layer->next_layer = 0;
	layer->next_free = _next_free_layer;
	_next_free_layer = layer;
	--_total_layers_count;
}

static struct frame_t *find_free_frame() {
	struct frame_t *frame = _next_free_frame;
	if (frame) {
		_next_free_frame = frame->next_free;
		frame->next_free = 0;
		++_total_frames_count;
	} else {
		fprintf(stderr, "find_free_frame MAX_FRAMES\n");
	}
	return frame;
}

static void FreeFrame(struct frame_t *frame) {
	frame->next_frame = 0;
	frame->next_free = _next_free_frame;
	_next_free_frame = frame;
	--_total_frames_count;
}

static struct anim_t *find_free_animation() {
	struct anim_t *animation = _next_free_animation;
	if (animation) {
		_next_free_animation = animation->next_free;
		animation->next_free = 0;
		++_total_animations_count;
	} else {
		fprintf(stderr, "find_free_animation MAX_ANIMATIONS\n");
	}
	return animation;
}

static void free_animation(struct anim_t *animation) {
	animation->next_free = _next_free_animation;
	_next_free_animation = animation;
	--_total_animations_count;
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
	fprintf(stdout, "Total animations %d frames %d layers %d\n", _total_animations_count, _total_frames_count, _total_layers_count);
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
	struct anim_t *animation = find_free_animation();
	if (animation) {
		for (int i = 0; _animationFormats[i].ext; ++i) {
			if (strcasecmp(_animationFormats[i].ext, name) == 0) {
				if (_animationFormats[i].load(fp, animation, find_free_frame, find_free_layer) < 0) {
					break;
				}
				return animation - _animations;
			}
		}
		fprintf(stderr, "Unsupported animation '%s'\n", name);
		free_animation(animation);
	}
	return -1;
}

int Animation_Free(int anim) {
	assert(!(anim < 0));
	for (struct frame_t *frame = _animations[anim].first_frame; frame; ) {
		struct frame_t *next_frame = frame->next_frame;
		for (struct layer_t *layer = frame->first_layer; layer; ) {
			struct layer_t *next_layer = layer->next_layer;
			free(layer->rgba);
			memset(layer, 0, sizeof(struct layer_t));
			free_layer(layer);
			layer = next_layer;
		}
		memset(frame, 0, sizeof(struct frame_t));
		FreeFrame(frame);
		frame = next_frame;
	}
	free_animation(&_animations[anim]);
	return 0;
}

int Animation_GetFramesCount(int anim) {
	assert(!(anim < 0));
	return _animations[anim].frames_count;
}

int Animation_GetFrameLayersCount(int anim, int frame_num) {
	assert(!(anim < 0));
	struct frame_t *frame = _animations[anim].first_frame;
	for (; frame_num-- != 0 && frame; frame = frame->next_frame);
	assert(frame);
	return frame->layers_count;
}

int Animation_GetFrameRect(int anim, int frame_num, int *x, int *y, int *w, int *h) {
	assert(!(anim < 0));
	struct frame_t *frame = _animations[anim].first_frame;
	for (; frame_num-- != 0 && frame; frame = frame->next_frame);
	assert(frame);
	int x1 = 640 - 1;
	int y1 = 480 - 1;
	int x2 = 0;
	int y2 = 0;
	struct layer_t *layer = frame->first_layer;
	for (; layer; layer = layer->next_layer) {
		if (layer->x < x1) {
			x1 = layer->x;
		}
		if (layer->y < y1) {
			y1 = layer->y;
		}
		if (layer->x + layer->w > x2) {
			x2 = layer->x + layer->w;
		}
		if (layer->y + layer->h > y2) {
			y2 = layer->y + layer->h;
		}
	}
	*x = x1;
	*y = y1;
	*w = x2 - x1;
	if (*w < 0) {
		*w = 0;
	}
	*h = y2 - y1;
	if (*h < 0) {
		*h = 0;
	}
	return 0;
}

struct layer_t *Animation_GetLayer(int anim, int frame_num, int layer_num) {
	assert(!(anim < 0));
	struct frame_t *frame = _animations[anim].first_frame;
	for (; frame_num-- != 0 && frame; frame = frame->next_frame);
	assert(frame);
	struct layer_t *layer = frame->first_layer;
	for (; layer_num-- != 0 && layer; layer = layer->next_layer);
	assert(layer);
	return layer;
}

int Animation_Seek(int anim, int frame_num) {
	assert(!(anim < 0));
	struct frame_t *frame = _animations[anim].first_frame;
	for (; frame_num-- != 0 && frame; frame = frame->next_frame);
	assert(frame);
	_animations[anim].current_frame = frame;
	return 0;
}

int Animation_SetLayer(int anim, int frame_num, const char *name, int state) {
	assert(!(anim < 0));
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

static void draw_layer(struct layer_t *layer, struct surface_t *s, int x, int y, int alpha, int *x1, int *y1, int *x2, int *y2) {
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
			dst[i] = blend(dst[i], src[i], alpha);
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

int Animation_Draw(int anim, struct surface_t *s, int dx, int dy, int mask, int alpha, int *x, int *y, int *w, int *h) {
	assert(!(anim < 0));
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
		draw_layer(layer, s, lx, ly, alpha, &x1, &y1, &x2, &y2);
	}
	*x = x1;
	*y = y1;
	*w = x2 - x1 + 1;
	if (*w < 0) {
		*w = 0;
	}
	*h = y2 - y1 + 1;
	if (*h < 0) {
		*h = 0;
	}
	return 0;
}
