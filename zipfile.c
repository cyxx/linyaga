
#include "zipfile.h"

struct zipentry_t {
	char *name;
	uint32_t offset;
	uint32_t size;
	uint8_t *buffer;
	uint32_t buffer_offset;
};

struct zipfile_t {
	FILE *fp;
	struct zipentry_t *entries;
	int entries_count;
};

static int compare_zipentry(const void *a, const void *b) {
	return strcasecmp(((const struct zipentry_t *)a)->name, ((const struct zipentry_t *)b)->name);
}

struct zipfile_t *Zipfile_Open(const char *name) {
	FILE *fp = fopen(name, "rb");
	if (!fp) {
		fprintf(stderr, "Failed to open '%s'\n", name);
		return 0;
	}
	static const int EOD_SIZE = 22; /* 'end of central directory record' without comment */
	fseek(fp, -EOD_SIZE, SEEK_END);
	const uint32_t eod_signature = fread_le32(fp);
	assert(eod_signature == 0x06054B50);
	fread_le16(fp); /* current disk number */
	fread_le16(fp); /* central directory disk number */
	const uint16_t entries_count = fread_le16(fp);
	const uint16_t total_entries_count = fread_le16(fp);
	assert(total_entries_count == entries_count);
	fread_le32(fp); /* directory size */
	fread_le32(fp); /* directory offset */
	const uint16_t comment_size = fread_le16(fp);
	assert(comment_size == 0);
	struct zipentry_t *entries = (struct zipentry_t *)calloc(entries_count, sizeof(struct zipentry_t));
	if (!entries) {
		return 0;
	}
	fseek(fp, 0, SEEK_SET);
	for (int i = 0; i < entries_count; ++i) {
		struct zipentry_t *entry = &entries[i];
		const uint32_t signature = fread_le32(fp);
		assert(signature == 0x04034B50);
		fread_le16(fp); /* version needed for extraction */
		fread_le16(fp); /* flags */
		const uint16_t compression = fread_le16(fp);
		if (compression != 0) {
			fprintf(stdout, "Unhandled compression %d\n", compression);
			continue;
		}
		fread_le16(fp); /* last modification file time */
		fread_le16(fp); /* last modification file date */
		fread_le32(fp); /* crc */
		entry->size = fread_le32(fp);
		const uint32_t compressed_size = fread_le32(fp);
		const uint16_t name_length = fread_le16(fp);
		const uint16_t extra_length = fread_le16(fp);
		if (name_length != 0) {
			entry->name = (char *)malloc(name_length + 1);
			if (entry->name) {
				fread(entry->name, 1, name_length, fp);
				entry->name[name_length] = 0;
			} else {
				fseek(fp, name_length, SEEK_CUR);
			}
		}
		if (extra_length != 0) {
			fseek(fp, name_length, SEEK_CUR);
		}
		entry->offset = ftell(fp);
		fseek(fp, compressed_size, SEEK_CUR);
		// fprintf(stdout, "file %s compression %d size %d compressed_size %d\n", entry->name, compression, entry->size, compressed_size);
	}
	qsort(entries, entries_count, sizeof(struct zipentry_t), compare_zipentry);

	struct zipfile_t *zf = (struct zipfile_t *)malloc(sizeof(struct zipfile_t));
	if (zf) {
		zf->fp = fp;
		zf->entries = entries;
		zf->entries_count = entries_count;
	}
	return zf;
}

void Zipfile_Close(struct zipfile_t *zf) {
	fclose(zf->fp);
	for (int i = 0; i < zf->entries_count; ++i) {
		free(zf->entries[i].name);
	}
	free(zf->entries);
	zf->entries = 0;
	zf->entries_count = 0;
	free(zf);
}

struct zipentry_t *Zipfile_Find(struct zipfile_t *zf, const char *name) {
	struct zipentry_t ze;
	ze.name = name;
        return (struct zipentry_t *)bsearch(&ze, zf->entries, zf->entries_count, sizeof(struct zipentry_t), compare_zipentry);
}

static ssize_t zipentry_read(void *cookie, char *buf, size_t size) {
	struct zipentry_t *ze = (struct zipentry_t *)cookie;
	// fprintf(stdout, "zipentry_read ze %p size %ld\n", (void *)ze, size);
	const int count = (ze->buffer_offset + size > ze->size) ? (ze->size - ze->buffer_offset) : size;
	if (count > 0) {
		memcpy(buf, ze->buffer + ze->buffer_offset, count);
		ze->buffer_offset += count;
		return count;
	}
	return 0;
}

static int zipentry_seek(void *cookie, off_t *offset, int whence) {
	struct zipentry_t *ze = (struct zipentry_t *)cookie;
	// fprintf(stdout, "zipentry_seek ze %p whence %d offset %ld\n", (void *)ze, whence, *offset);
	int next_offset = ze->buffer_offset;
	switch (whence) {
	case SEEK_SET:
		next_offset = *offset;
		break;
	case SEEK_CUR:
		next_offset += *offset;
		break;
	case SEEK_END:
		next_offset = ze->size + *offset;
		break;
	}
	if (next_offset < 0 || next_offset > ze->size) {
		return -1;
	}
	ze->buffer_offset = *offset = next_offset;
	return 0;
}

static int zipentry_close(void *cookie) {
	struct zipentry_t *ze = (struct zipentry_t *)cookie;
	free(ze->buffer);
	ze->buffer = 0;
	return 0;
}

FILE *Zipfile_OpenEntry(struct zipfile_t *zf, struct zipentry_t *ze) {
	uint8_t *buffer = (uint8_t *)malloc(ze->size);
	if (!buffer) {
		fprintf(stderr, "Failed to allocate %d bytes\n", ze->size);
		return 0;
	}
	fseek(zf->fp, ze->offset, SEEK_SET);
	const int count = fread(buffer, 1, ze->size, zf->fp);
	// fprintf(stdout, "Read %d bytes from zf %p\n", count, (void *)zf);
	if (count != ze->size) {
		fprintf(stderr, "Error reading %d bytes, ret %d\n", ze->size, count);
		free(buffer);
		return 0;
	}
	ze->buffer = buffer;
	ze->buffer_offset = 0;
	cookie_io_functions_t funcs = {
		.read = zipentry_read,
		.write = 0,
		.seek = zipentry_seek,
		.close = zipentry_close,
	};
	return fopencookie(ze, "r", funcs);
}

int Zipfile_GetEntrySize(struct zipfile_t *zf, struct zipentry_t *ze) {
	return ze->size;
}
