
#include "animation.h"
#include "mixer.h"
#include "resource.h"
#include "sys.h"

static const char *USAGE =
	"Usage: %s path/to/.exe\n";

int Installer_Main(const char *filePath);

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

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stdout, USAGE, argv[0]);
		return 0;
	}
	System_Init();
	Mixer_Init(22050, AudioLock);
	System_StartAudio(AudioSamplesCb, 0);
	Animation_Init();
	Resource_Init();
	Installer_Main(argv[1]);
	Resource_Fini();
	Animation_Fini();
	Mixer_Fini();
	System_Fini();
	return 0;
}
