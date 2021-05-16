
#include <zlib.h>
#include "animation.h"

static const uint8_t MNG_SIG[] = { 0x8a, 0x4d, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };

enum {
	TAG_MHDR = 0x4d484452,
	TAG_tIME = 0x74494d45,
	TAG_BACK = 0x4241434b,
	TAG_sBIT = 0x73424954,
	TAG_PLTE = 0x504c5445,
	TAG_tRNS = 0x74524e53,
	TAG_FRAM = 0x4652414d,
	TAG_flAG = 0x666c4147,
	TAG_DEFI = 0x44454649,
	TAG_tEXt = 0x74455874,
	TAG_IHDR = 0x49484452,
	TAG_IDAT = 0x49444154,
	TAG_IEND = 0x49454e44,
	TAG_MEND = 0x4d454e44
};

struct image_t {
	uint32_t w, h;
	uint8_t depth;
	uint8_t color;
	uint8_t compression;
	uint8_t filter;
	uint8_t interlace;
	uint32_t palette[256];
	uint8_t *zdata;
	uint32_t zsize;
};

static uint32_t *decode_bitmap(struct image_t *image, int has_pal, const uint8_t *src, int size, int bpp) {
	assert(bpp != 1 || has_pal);
	uint32_t *rgba = (uint32_t *)malloc(image->h * image->w * sizeof(uint32_t));
	if (!rgba) {
		fprintf(stderr, "Failed to allocate RGBA buffer w:%d h:%d\n", image->w, image->h);
	} else {
		int offset = 0;
		for (int y = 0; y < image->h; ++y, offset += image->w) {
			++src; /* filter */
			switch (bpp) {
			case 1:
				for (int x = 0; x < image->w; ++x) {
					rgba[offset + x] = image->palette[*src++];
				}
				break;
			case 3:
				for (int x = 0; x < image->w; ++x) {
					const uint32_t color = src[2] | (src[1] << 8) | (src[0] << 16) | 0xFF000000;
					src += 3;
					rgba[offset + x] = color;
				}
				break;
			case 4:
				for (int x = 0; x < image->w; ++x) {
					const uint32_t color = src[2] | (src[1] << 8) | (src[0] << 16) | (src[3] << 24);
					src += 4;
					rgba[offset + x] = color;
				}
				break;
			}
		}
	}
	return rgba;
}

static uint32_t *decode_zdata(struct image_t *image, int has_pal) {
	int bpp = 0;
	switch (image->color) {
	case 2: /* RGB */
		bpp = 3;
		break;
	case 3: /* palette */
		bpp = 1;
		break;
	case 6: /* RGBA */
		bpp = 4;
		break;
	default:
		fprintf(stderr, "Unsupported PNG image color %d", image->color);
		break;
	}
	uint32_t *rgba = 0;
	const int buf_size = image->h * (image->w * bpp) + (image->h);
	if (image->zsize == buf_size) { /* uncompressed */
		rgba = decode_bitmap(image, has_pal, image->zdata, buf_size, bpp);
        } else {
		uint8_t *buf = (uint8_t *)malloc(buf_size);
		if (!buf) {
			fprintf(stderr, "Failed to allocate %d bytes\n", buf_size);
			return 0;
		}
		z_stream z_str;
		memset(&z_str, 0, sizeof(z_str));
		z_str.avail_in = image->zsize;
		z_str.next_in = image->zdata;
		z_str.avail_out = buf_size;
		z_str.next_out = buf;
		int ret = inflateInit(&z_str);
		if (ret == Z_OK) {
			ret = inflate(&z_str, Z_FINISH);
			if (ret != Z_STREAM_END) {
				fprintf(stderr, "inflate ret %d\n", ret);
			}
		}
		if (z_str.total_out != buf_size) {
			if (image->w == 2 && image->h == 2 && z_str.total_out == 18 && image->color == 3) {
				rgba = decode_bitmap(image, 0, buf, z_str.total_out, 4);
			} else {
				fprintf(stderr, "Invalid PNG data w:%d h:%d color:%d\n", image->w, image->h, image->color);
			}
                } else {
			rgba = decode_bitmap(image, has_pal, buf, z_str.total_out, bpp);
		}
		free(buf);
	}
	return rgba;
}

static void read_plte(FILE *fp, uint32_t *dst) {
	for (int i = 0; i < 256; ++i) {
		const uint8_t r = fgetc(fp);
		const uint8_t g = fgetc(fp);
		const uint8_t b = fgetc(fp);
		dst[i] = (dst[i] & 0xFF000000) | (r << 16) | (g << 8) | b;
	}
}

static void read_trns(FILE *fp, uint32_t *dst) {
	for (int i = 0; i < 256; ++i) {
		dst[i] = (dst[i] & 0xFFFFFF) | (fgetc(fp) << 24);
	}
}

int Animation_Load_MNG(FILE *fp, struct anim_t *anim, FreeFrameProc frameProc, FreeLayerProc layerProc) {
	uint8_t buf[8];
	fread(buf, 1, 8, fp);
	assert(memcmp(buf, MNG_SIG, 8) == 0);

	uint32_t palette[256];
	for (int i = 0; i < 256; ++i) {
		palette[i] = 0xFF000000;
	}

	int plte_flag = 0;
	int fram_flag = 0;
	int frames_count = 0;
	int layers_count = 0;
	struct frame_t *current_frame = 0;
	struct layer_t *current_layer = 0;

	char text[256];
	struct image_t current_image;

	while (!feof(fp)) {
		uint32_t size = fread_be32(fp);
		uint32_t tag = fread_be32(fp);
		switch (tag) {
		case TAG_tRNS:
			if (size == 256) {
				read_trns(fp, fram_flag ? current_image.palette : palette);
			} else {
				assert(size == 0);
			}
			break;
		case TAG_PLTE:
			if (size == 256 * 3) {
				read_plte(fp, fram_flag ? current_image.palette : palette);
				plte_flag = 1;
			} else {
				assert(size == 0);
			}
			break;
		case TAG_FRAM:
			if (size == 10) { /* first fram */
				fseek(fp, size, SEEK_CUR);
				assert(fram_flag == 0);
				fram_flag = 1;
			} else {
				assert(size == 0);
				assert(fram_flag == 1);
				if (layers_count == 0) { /* empty fram */
					break;
				}
			}
			/* add frame to animation */
			{
				struct frame_t *frame = frameProc();
				if (current_frame) {
					current_frame->next_frame = frame;
				}
				current_frame = frame;
				if (frames_count == 0) {
					anim->first_frame = current_frame;
				}
				++frames_count;
			}
			layers_count = 0;
			current_layer = 0;
			memcpy(current_image.palette, palette, sizeof(palette));
			break;
		case TAG_DEFI:
			assert(size == 12);
			/* add layer to frame */
			{
				struct layer_t *layer = layerProc();
				if (current_layer) {
					current_layer->next_layer = layer;
				}
				current_layer = layer;
				if (layers_count == 0) {
					current_frame->first_layer = layer;
				}
				++layers_count;
				current_frame->layers_count = layers_count;
			}
			fseek(fp, 4, SEEK_CUR);
			current_layer->x = fread_be32(fp);
			current_layer->y = fread_be32(fp);
			// fprintf(stdout, "frame %d layer %d pos %d,%d\n", frames_count, layers_count, current_layer->x, current_layer->y);
			current_layer->state = 1;
			break;
		case TAG_tEXt:
			assert(size <= sizeof(text));
			fread(text, 1, size, fp);
			assert(memcmp(text, "LAYER", 5) == 0);
			size -= 6;
			assert(size < sizeof(current_layer->name));
			memcpy(current_layer->name, text + 6, size);
			current_layer->name[size] = 0;
			// fprintf(stdout, "layer name %s\n", current_layer->name);
			break;
		case TAG_flAG:
			assert(size == 4);
			current_layer->mask = fread_le32(fp);
			break;
		case TAG_IHDR:
			assert(size == 13);
			current_image.w = fread_be32(fp);
			current_image.h = fread_be32(fp);
			current_image.depth = fgetc(fp);
			assert(current_image.depth == 8);
			current_image.color = fgetc(fp);
			current_image.compression = fgetc(fp);
			current_image.filter = fgetc(fp);
			current_image.interlace = fgetc(fp);
			assert(current_image.compression == 0);
			assert(current_image.filter == 0);
			assert(current_image.interlace == 0);
			current_image.zdata = 0;
			current_image.zsize = 0;
			break;
		case TAG_IDAT:
			current_image.zdata = realloc(current_image.zdata, current_image.zsize + size);
			fread(current_image.zdata + current_image.zsize, 1, size, fp);
			current_image.zsize += size;
			break;
		case TAG_IEND:
			assert(size == 0);
			current_layer->rgba = decode_zdata(&current_image, plte_flag);
			current_layer->w = current_image.w;
			current_layer->h = current_image.h;
			// fprintf(stdout, "decoded bitmap %d %d RGBA %p\n", current_image.w, current_image.h, current_layer->rgba);
			free(current_image.zdata);
			current_image.zdata = 0;
			current_image.zsize = 0;
			break;
		default:
			fseek(fp, size, SEEK_CUR);
			break;
		}
		fread_be32(fp); /* crc */
		if (tag == TAG_MEND) {
			break;
		}
	}
	anim->frames_count = frames_count;
	anim->current_frame = anim->first_frame;
	return 0;
}
