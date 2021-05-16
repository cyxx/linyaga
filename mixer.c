
#include "mixer.h"

#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#include "dr_mp3.h"

#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#include "dr_wav.h"

#define MAX_CHANNELS 16

enum {
	TAG_ID3  = 0x49443303,
	TAG_RIFF = 0x52494646
};

enum {
	TYPE_UNKNOWN = 0,
	TYPE_MP3,
	TYPE_WAV
};

struct mixer_channel_t {
	FILE *fp;
	int type;
	int stereo;
	union {
		drmp3 mp3;
		drwav wav;
	} state;
	struct mixer_channel_t *next;
};

static struct mixer_channel_t _channels[MAX_CHANNELS];
static struct mixer_channel_t *_next_channel;
static int _hz;
static MixerLockProc _lock;

static struct mixer_channel_t *find_free_channel() {
	struct mixer_channel_t *channel = _next_channel;
	if (channel) {
		_next_channel = _next_channel->next;
		channel->next = 0;
	} else {
		fprintf(stderr, "find_free_channel MAX_CHANNELS\n");
	}
	return channel;
}

static void free_channel(struct mixer_channel_t *channel) {
	channel->next = _next_channel;
	_next_channel = channel;
}

int Mixer_Init(int hz, MixerLockProc lock) {
	_hz = hz;
	_next_channel = &_channels[0];
	for (int i = 0; i < MAX_CHANNELS - 1; ++i) {
		_channels[i].next = &_channels[i + 1];
	}
	_lock = lock;
	return 0;
}

int Mixer_Fini() {
	return 0;
}

static size_t ChannelReadProc(void *pUserData, void *pBufferOut, size_t bytesToRead) {
	struct mixer_channel_t *channel = (struct mixer_channel_t *)pUserData;
	return fread(pBufferOut, 1, bytesToRead, channel->fp);
}

static drmp3_bool32 Mp3ChannelSeekProc(void *pUserData, int offset, drmp3_seek_origin origin) {
	struct mixer_channel_t *channel = (struct mixer_channel_t *)pUserData;
	if (origin == drmp3_seek_origin_start) {
		return fseek(channel->fp, offset, SEEK_SET) == 0;
	} else if (origin == drmp3_seek_origin_current) {
		return fseek(channel->fp, offset, SEEK_CUR) == 0;
	}
	return 0;
}

static drwav_bool32 WavChannelSeekProc(void *pUserData, int offset, drwav_seek_origin origin) {
	struct mixer_channel_t *channel = (struct mixer_channel_t *)pUserData;
	if (origin == drwav_seek_origin_start) {
		return fseek(channel->fp, offset, SEEK_SET) == 0;
	} else if (origin == drwav_seek_origin_current) {
		return fseek(channel->fp, offset, SEEK_CUR) == 0;
	}
	return 0;
}

int Mixer_PlayMp3(FILE *fp) {
	int num = -1;
	_lock(1);
	struct mixer_channel_t *channel = find_free_channel();
	if (channel) {
		channel->fp = fp;
		channel->type = TYPE_MP3;
		drmp3_init(&channel->state.mp3, ChannelReadProc, Mp3ChannelSeekProc, channel, 0);
		if (channel->state.mp3.sampleRate != _hz) {
			fprintf(stderr, "Unsupported sample rate %d for MP3\n", channel->state.mp3.sampleRate);
			free_channel(channel);
		} else {
			channel->stereo = (channel->state.mp3.channels == 2);
			num = channel - _channels;
		}
	}
	_lock(0);
	return num;
}

int Mixer_PlayWav(FILE *fp) {
	int num = -1;
	_lock(1);
	struct mixer_channel_t *channel = find_free_channel();
	if (channel) {
		channel->fp = fp;
		channel->type = TYPE_WAV;
		drwav_init(&channel->state.wav, ChannelReadProc, WavChannelSeekProc, channel, 0);
		if (channel->state.wav.sampleRate != _hz) {
			fprintf(stderr, "Unsupported sample rate %d for WAV\n", channel->state.wav.sampleRate);
			free_channel(channel);
		} else {
			channel->stereo = (channel->state.wav.channels == 2);
			num = channel - _channels;
		}
	}
	_lock(0);
	return num;
}

int Mixer_Stop(int channel) {
	_lock(1);
	free_channel(&_channels[channel]);
	_channels[channel].fp = 0;
	_lock(0);
	return 0;
}

int Mixer_IsPlaying(int channel) {
	_lock(1);
	const int playing = _channels[channel].fp != 0;
	_lock(0);
	return playing;
}

static int16_t clipS16(int sample) {
	return ((sample < SHRT_MIN) ? SHRT_MIN : ((sample > SHRT_MAX) ? SHRT_MAX : sample));
}

int Mixer_MixStereoS16(int16_t *samples, int len) {
	memset(samples, 0, sizeof(int16_t) * 2 * len);
	int16_t *buffer = alloca(sizeof(int16_t) * 2 * len);
	for (int i = 0; i < MAX_CHANNELS; ++i) {
		struct mixer_channel_t *channel = &_channels[i];
		if (channel->fp) {
			int count = 0;
			switch (channel->type) {
			case TYPE_MP3:
				count = drmp3_read_pcm_frames_s16(&channel->state.mp3, len, buffer);
				break;
			case TYPE_WAV:
				count = drwav_read_pcm_frames_s16(&channel->state.wav, len, buffer);
				break;
			}
			if (count == 0) {
				channel->fp = 0;
				continue;
			}
			if (channel->stereo) {
				for (int i = 0; i < count * 2; ++i) {
					samples[i] = clipS16(samples[i] + buffer[i]);
				}
			} else {
				for (int i = 0; i < count * 2; ++i) {
					samples[i] = clipS16(samples[i] + buffer[i / 2]);
				}
			}
		}
	}
	return 0;
}
