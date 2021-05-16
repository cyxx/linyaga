
#include <sys/param.h>
#include "animation.h"
#include "font.h"
#include "mixer.h"
#include "resource.h"
#include "sys.h"

static const char *USAGE =
	"Usage: DATAPATH=path/to/he/ %s path/to/.exe\n";

int Installer_Main(int argc, char *argv[]);

static void AudioSamplesCb(void *userdata, uint8_t *data, int len) {
	assert((len & 3) == 0);
	Mixer_MixStereoS16((int16_t *)data, len / 4);
}

static void AudioLock(int flag) {
	if (flag) {
		System_LockAudio();
	} else {
		System_UnlockAudio();
	}
}

#define BITMAPFILEHEADER_SIZE 14
#define BITMAPINFOHEADER_SIZE 40

static void TO_LE16(uint8_t *dst, uint16_t value) {
	dst[0] = value & 255;
	dst[1] = value >> 8;
}

static void TO_LE32(uint8_t *dst, uint32_t value) {
	TO_LE16(&dst[0], value & 0xFFFF);
	TO_LE16(&dst[2], value >> 16);
}

static uint8_t *LoadIco(const char *path, int *bitmapsize) {
	uint8_t *bitmap = 0;
	const char *ext = strrchr(path, '.');
	if (ext) {
		char icon_path[MAXPATHLEN];
		strcpy(icon_path, path);
		strcpy(icon_path + (ext - path) + 1, "ico");
		FILE *fp = fopen(icon_path, "rb");
		if (fp) {
			fprintf(stdout, "Using '%s' as icon\n", icon_path);
			uint16_t reserved = fread_le16(fp);
			uint16_t type = fread_le16(fp);
			uint16_t count = fread_le16(fp);
			if (reserved == 0 && type == 1 && count == 1) {
				const uint8_t w = fgetc(fp);
				const uint8_t h = fgetc(fp);
				fgetc(fp); /* colors */
				if (w == 32 && h == 32) {
					fgetc(fp); /* reserved */
					fread_le16(fp); /* planes */
					fread_le16(fp); /* bpp */
					uint32_t size = fread_le32(fp);
					uint32_t offset = fread_le32(fp);
					assert(offset == ftell(fp));
					bitmap = (uint8_t *)malloc(size + BITMAPFILEHEADER_SIZE);
					if (bitmap) {
						// icons are stored as DIB bitmap
						bitmap[0] = 'B'; bitmap[1] = 'M';
						TO_LE32(bitmap + 2, BITMAPFILEHEADER_SIZE + size);
						TO_LE16(bitmap + 6, 0);
						TO_LE16(bitmap + 8, 0);
						TO_LE32(bitmap + 10, BITMAPFILEHEADER_SIZE + BITMAPINFOHEADER_SIZE + 256 * 4);
						uint8_t *buf = bitmap + BITMAPFILEHEADER_SIZE;
						fread(buf, 1, size, fp);
						// fixup dimensions to 32x32
						if (buf[0] == BITMAPINFOHEADER_SIZE && buf[4] == 32 && buf[8] != 32) {
							buf[8] = 32;
						}
						*bitmapsize = size + BITMAPFILEHEADER_SIZE;
					}
				}
			}
			fclose(fp);
		}
	}
	return bitmap;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stdout, USAGE, argv[0]);
		return 0;
	}
	System_Init();
	Mixer_Init(22050, AudioLock);
	System_StartAudio(AudioSamplesCb, 0);
	Animation_Init();
	Font_Init();
	Resource_Init();
	int size = 0;
	uint8_t *ico = LoadIco(argv[1], &size);
	if (ico) {
		System_SetIcon(ico, size);
		free(ico);
	}
	Installer_Main(argc - 1, argv + 1);
	Resource_Fini();
	Font_Fini();
	Animation_Fini();
	Mixer_Fini();
	System_Fini();
	return 0;
}
