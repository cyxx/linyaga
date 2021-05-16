
#ifndef ZIPFILE_H__
#define ZIPFILE_H__

#include "intern.h"

struct zipfile_t;
struct zipentry_t;

struct zipfile_t *Zipfile_Open(const char *name);
void Zipfile_Close(struct zipfile_t *zf);

struct zipentry_t *Zipfile_Find(struct zipfile_t *zf, const char *name);
FILE *Zipfile_OpenEntry(struct zipfile_t *zf, struct zipentry_t *ze);
int Zipfile_GetEntrySize(struct zipfile_t *zf, struct zipentry_t *ze);

#endif
