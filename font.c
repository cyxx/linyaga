
#include "font.h"

#define MAX_FONTS       16
#define MAX_CHAR_RECTS 128

struct char_rect_t {
	int x, y;
	int w, h;
};

struct font_t {
	uint8_t first_char, last_char, space_char;
	const uint32_t *rgba;
	int w, h;
	struct char_rect_t char_rects[MAX_CHAR_RECTS];
};

static struct font_t _fonts[MAX_FONTS];
static int _fonts_count;

int Font_Init() {
	return 0;
}

int Font_Fini() {
	fprintf(stdout, "Total fonts %d\n", _fonts_count);
	return 0;
}

static void scan_horizontal_chars(const uint32_t *rgba, int w, int h, int count, struct font_t *font) {
	int start_x = 0;
	int current = 0;
	bool prev_transparent = false;
	for (int x = 0; x < w; ++x) {
		bool transparent = true;
		for (int y = 0; y < h; ++y) {
			const uint32_t color = rgba[y * w + x];
			if ((color >> 24) != 0) {
				transparent = false; /* column not transparent */
				break;
			}
		}
		// xxxTTTxxx
		if (transparent) {
			if (!prev_transparent) { /* character end */
				font->char_rects[current].x = start_x;
				font->char_rects[current].y = 0;
				font->char_rects[current].w = x - start_x;
				font->char_rects[current].h = h;
				++current;
				//fprintf(stdout, "character end x %d\n", x);
			}
		} else {
			if (prev_transparent) { /* character start */
				//fprintf(stdout, "character start x %d\n", x);
				start_x = x;
			}
		}
		prev_transparent = transparent;
	}
	//fprintf(stdout, "Scanned %d characters\n", current);
	assert(current + 1 == count);
	font->char_rects[current].x = start_x;
	font->char_rects[current].y = 0;
	font->char_rects[current].w = w - start_x;
	font->char_rects[current].h = h;
}

int Font_Load(const uint32_t *rgba, int w, int h, uint8_t first_char, uint8_t last_char, uint8_t space_char) {
	assert(_fonts_count < MAX_FONTS);
	struct font_t *font = &_fonts[_fonts_count++];
	font->first_char = first_char;
	font->last_char = last_char;
	font->space_char = space_char;
	const int count = last_char - first_char + 1;
	//fprintf(stdout, "Total ascii characters in font %d, w:%d h:%d\n", count, w, h);
	assert(count <= MAX_CHAR_RECTS);
	scan_horizontal_chars(rgba, w, h, count, font);
	font->rgba = rgba;
	font->w = w;
	font->h = h;
	return _fonts_count - 1;
}

int Font_GetCharRect(int font_num, uint8_t chr, int *x, int *y, int *w, int *h) {
	assert(!(font_num < 0));
	struct font_t *font = &_fonts[font_num];
	if (!(chr >= font->first_char && chr <= font->last_char)) {
		chr = font->space_char;
	}
	const int num = chr - font->first_char;
	struct char_rect_t *r = &font->char_rects[num];
	*x = r->x;
	*y = r->y;
	*w = r->w;
	*h = r->h;
	return 0;
}

int Font_DrawChar(int font_num, uint8_t chr, struct surface_t *s, int x, int y) {
	assert(!(font_num < 0));
	struct font_t *font = &_fonts[font_num];
	if (!(chr >= font->first_char && chr <= font->last_char)) {
		chr = font->space_char;
	}
	const int num = chr - font->first_char;
	struct char_rect_t *r = &font->char_rects[num];
	const uint32_t *src = font->rgba + r->y * font->w + r->x;
	int w = r->w;
	if (x < 0) {
		src -= x;
		w += x;
		x = 0;
	}
	if (x + w > s->w) {
		w = s->w - x;
	}
	if (w <= 0) {
		return 0;
	}
	int h = r->h;
	if (y < 0) {
		src -= y * font->w;
		h += y;
		y = 0;
	}
	if (y + h > s->h) {
		h = s->h - y;
	}
	if (h <= 0) {
		return 0;
	}
	uint32_t *dst = s->buffer + y * s->w + x;
	for (int j = 0; j < h; ++j) {
		for (int i = 0; i < w; ++i) {
			dst[i] = blend(dst[i], src[i]);
		}
		dst += s->w;
		src += font->w;
	}
	return 0;
}
