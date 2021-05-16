
#ifndef INTERN_H__
#define INTERN_H__

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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

#endif
