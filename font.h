
#ifndef FONT_H__
#define FONT_H__

#include "intern.h"

int Font_Init();
int Font_Fini();

int Font_Load(const uint32_t *rgba, int w, int h, uint8_t first_char, uint8_t last_char, uint8_t space_char);
int Font_GetCharRect(int font, uint8_t chr, int *x, int *y, int *w, int *h);
int Font_DrawChar(int font, uint8_t chr, struct surface_t *s, int x, int y);

#endif
