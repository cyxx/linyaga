
#ifndef INTERN_H__
#define INTERN_H__

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

struct surface_t {
	uint32_t *buffer;
	int w, h;
};

#undef MIN
static inline int MIN(int a, int b) {
	return (a < b) ? a : b;
}

#undef MAX
static inline int MAX(int a, int b) {
	return (a > b) ? a : b;
}

static inline uint32_t READ_BE_UINT32(const uint8_t *p) {
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static inline uint16_t READ_BE_UINT16(const uint8_t *p) {
	return (p[0] << 8) | p[1];
}

static inline uint32_t READ_LE_UINT32(const uint8_t *p) {
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static inline uint16_t READ_LE_UINT16(const uint8_t *p) {
	return p[0] | (p[1] << 8);
}

static inline uint32_t fread_be32(FILE *fp) {
	uint8_t buf[4];
	fread(buf, 1, sizeof(buf), fp);
	return READ_BE_UINT32(buf);
}

static inline uint16_t fread_be16(FILE *fp) {
	uint8_t buf[2];
	fread(buf, 1, sizeof(buf), fp);
	return READ_BE_UINT16(buf);
}

static inline uint32_t fread_le32(FILE *fp) {
	uint8_t buf[4];
	fread(buf, 1, sizeof(buf), fp);
	return READ_LE_UINT32(buf);
}

static inline uint16_t fread_le16(FILE *fp) {
	uint8_t buf[2];
	fread(buf, 1, sizeof(buf), fp);
	return READ_LE_UINT16(buf);
}

static inline uint32_t blend(uint32_t a, uint32_t b, int balpha) {
	const uint8_t alpha = ((b >> 24) * balpha) >> 8;
	switch (alpha) {
	case 0:
		return a;
	case 255:
		return b;
	}
	const uint32_t rb1 = ((a & 0xFF00FF) * (255 - alpha)) >> 8;
	const uint32_t rb2 = ((b & 0xFF00FF) * alpha) >>  8;
	const uint32_t g1  = ((a & 0x00FF00) * (255 - alpha)) >> 8;
	const uint32_t g2  = ((b & 0x00FF00) * alpha) >> 8;
	return ((rb1 | rb2) & 0xFF00FF) | ((g1 | g2) & 0x00FF00);
}

#endif
