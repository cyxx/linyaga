
#include "animation.h"

static float le32_to_float(uint32_t x) {
	return *(float *)&x;
}

static const uint8_t RLE_SIG[] = { 0xF2, 0x65, 0x6C, 0x72, 0x00, 0x00, 0x20, 0x4D };

static uint32_t *decode(FILE *fp, int size, int w, int h, int fmt, const uint32_t *palette) {
	uint32_t *rgba = (uint32_t *)calloc(w * h, sizeof(uint32_t));
	if (!rgba) {
		fprintf(stderr, "Failed to allocate RGBA buffer w:%d h:%d\n", w, h);
	} else {
		int offset = 0;
		switch (fmt) {
		case 0x40012F9:
		case 0x40012FB: /* paletted */
			while (size > 0) {
				const uint8_t code = fgetc(fp);
				const int count = (code & 0x3F) + 1;
				if ((code & 0xC0) == 0xC0) {
					/* transparent */
				} else if ((code & 0x80) == 0x80) {
					const uint8_t color = fgetc(fp);
					for (int i = 0; i < count; ++i) {
						rgba[offset + i] = palette[color];
					}
					--size;
				} else {
					for (int i = 0; i < count; ++i) {
						const uint8_t color = fgetc(fp);
						rgba[offset + i] = palette[color];
					}
					size -= count;
				}
				--size;
				offset += count;
			}
			break;
		case 0xC0012F9: /* rgba */
			while (size > 0) {
				const uint8_t code = fgetc(fp);
				const int count = (code & 0x3F) + 1;
				if ((code & 0xC0) == 0xC0) {
					/* transparent */
				} else if ((code & 0x80) == 0x80) {
					const uint32_t color = fread_le32(fp);
					for (int i = 0; i < count; ++i) {
						rgba[offset + i] = color;
					}
					size -= 4;
				} else {
					for (int i = 0; i < count; ++i) {
						const uint32_t color = fread_le32(fp);
						rgba[offset + i] = color;
					}
					size -= count * 4;
				}
				--size;
				offset += count;
			}
			break;
		default:
			fprintf(stderr, "Unsupported RLE format 0x%x\n", fmt);
			break;
		}
	}
	// fprintf(stdout, "RLE remaining bytes %d\n", size);
	assert(size == 0);
	return rgba;
}

int Animation_Load_RLE(FILE *fp, struct anim_t *anim, FreeFrameProc frameProc, FreeLayerProc layerProc) {
	uint8_t buf[8];
	fread(buf, 1, sizeof(buf), fp);
	assert(memcmp(buf, RLE_SIG, 8) == 0);

	uint32_t palette[256];
	anim->frames_count = fread_le32(fp);
	uint32_t flags = fread_le32(fp);
	if (flags & 1) {
		fread(palette, sizeof(uint32_t), 256, fp);
	}
	struct frame_t *previous_frame = 0;
	for (int i = 0; i < anim->frames_count; ++i) {
		struct frame_t *frame = frameProc();
		if (i == 0) {
			anim->first_frame = frame;
		} else {
			previous_frame->next_frame = frame;
		}
		previous_frame = frame;

		uint8_t frame_hdr[16];
		fread(frame_hdr, 1, sizeof(frame_hdr), fp);
		frame->layers_count = READ_LE_UINT32(frame_hdr + 12);

		struct layer_t *previous_layer = 0;
		for (int j = 0; j < frame->layers_count; ++j) {
			struct layer_t *layer = layerProc();
			if (j == 0) {
				frame->first_layer = layer;
			} else {
				previous_layer->next_layer = layer;
			}
			previous_layer = layer;

			layer->x = (uint32_t)le32_to_float(fread_le32(fp));
			layer->y = (uint32_t)le32_to_float(fread_le32(fp));
			fread_le32(fp);
			layer->mask = fread_le32(fp);

			uint8_t text[0x39];
			fread(text, 1, sizeof(text), fp);
			memcpy(layer->name, text, sizeof(text));

			fread(buf, 1, 4, fp);
			assert(memcmp(buf, "rle\x00", 4) == 0);

			const uint32_t layer_fmt = fread_le32(fp);

			fseek(fp, 3, SEEK_CUR); // \x00\x00\x00

			layer->w = fread_le32(fp);
			layer->h = fread_le32(fp);

			const uint32_t layer_flags = fread_le32(fp);

			fread_le32(fp);
			const uint32_t a = fread_le32(fp);
			assert(a == 1);

			const int image_size = fread_le32(fp);

			uint32_t layer_palette[256];
			if (layer_flags & 1) {
				fread(layer_palette, sizeof(uint32_t), 256, fp);
			}

			layer->rgba = decode(fp, image_size, layer->w, layer->h, layer_fmt, (layer_flags & 1) ? layer_palette : palette);
			layer->state = 1;
		}
	}
	return 0;
}
