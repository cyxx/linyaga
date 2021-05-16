
#include <libavcodec/avcodec.h>
#include "sys.h"
#include "video.h"

enum {
	TAG_BIKi = 0x694b4942
};

struct video_t {
	FILE *fp;
	int w, h;
	int frame_rate;
	int frames_count;
	uint32_t flags;
	int tracks_count;
	uint32_t *frames_offset;
	int current_frame;
	AVCodecContext *context;
	AVFrame *frame;
};

static struct video_t _current_video;

static const AVCodec *_codec;

int Video_Init() {
//	avcodec_register_all();
	_codec = avcodec_find_decoder(AV_CODEC_ID_BINKVIDEO);
	assert(_codec);
	return 0;
}

int Video_Fini() {
	return 0;
}

int Video_IsVideoPlaying(int video) {
	return _current_video.fp && _current_video.current_frame < _current_video.frames_count;
}

static int read_bink_header(FILE *fp, struct video_t *v) {
	const uint32_t tag = fread_le32(fp);
	assert(tag == TAG_BIKi);
	const uint32_t size = fread_le32(fp) + 8;
	const int frames = fread_le32(fp);
	fread_le32(fp); /* maximum frame size */
	fread_le32(fp);
	v->w = fread_le32(fp);
	v->h = fread_le32(fp);
	const int frame_rate_num = fread_le32(fp);
	const int frame_rate_den = fread_le32(fp);
	v->frame_rate = frame_rate_num * 1000 / frame_rate_den;
	v->flags = fread_le32(fp);

	const int audio_tracks = fread_le32(fp);
	for (int i = 0; i < audio_tracks; ++i) {
		fread_le32(fp);
		fread_le16(fp); /* sample rate */
		fread_le16(fp); /* flags */
		fread_le32(fp);
	}
	v->tracks_count = audio_tracks;

	v->frames_offset = (uint32_t *)malloc((frames + 1) * sizeof(uint32_t));
	if (v->frames_offset) {
		for (int i = 0; i < frames; ++i) {
			v->frames_offset[i] = fread_le32(fp);
		}
		v->frames_offset[frames] = size;
		v->frames_count = frames;
	}
	v->fp = fp;
	return 0;
}

int Video_PlayVideo(FILE *fp) {
	if (_current_video.fp) {
		return -1;
	}
	read_bink_header(fp, &_current_video);
	_current_video.context = avcodec_alloc_context3(_codec);
	if (!_current_video.context) {
		return -1;
	}
	_current_video.context->codec_tag = TAG_BIKi;
	_current_video.context->width  = _current_video.w;
	_current_video.context->height = _current_video.h;

	_current_video.context->extradata_size = 4;
	_current_video.context->extradata = av_mallocz(4 + AV_INPUT_BUFFER_PADDING_SIZE);
	TO_LE32(_current_video.context->extradata, _current_video.flags);

	avcodec_open2(_current_video.context, _codec, 0);
	_current_video.frame = av_frame_alloc();
	return 0;
}

int Video_StopVideo(int video) {
	if (_current_video.context) {
		avcodec_free_context(&_current_video.context);
		_current_video.context = 0;
	}
	if (_current_video.frame) {
		av_frame_free(&_current_video.frame);
		_current_video.frame = 0;
	}
	free(_current_video.frames_offset);
	memset(&_current_video, 0, sizeof(_current_video));
	return 0;
}

int Video_RenderVideo(int video) {
	assert(_current_video.current_frame < _current_video.frames_count);
	const uint32_t offset = _current_video.frames_offset[_current_video.current_frame];
	uint32_t total_size = (_current_video.frames_offset[_current_video.current_frame + 1] & ~1) - (offset & ~1);
	fseek(_current_video.fp, offset & ~1, SEEK_SET);
	for (int i = 0; i < _current_video.tracks_count; ++i) {
		const uint32_t audio_size = fread_le32(_current_video.fp);
		fseek(_current_video.fp, audio_size, SEEK_CUR);
		total_size -= 4 + audio_size;
	}
	AVPacket pkt;
	av_new_packet(&pkt, total_size);
	fread(pkt.data, 1, total_size, _current_video.fp);
	if (offset & 1) {
		pkt.flags |= AV_PKT_FLAG_KEY;
	}
	int ret = avcodec_send_packet(_current_video.context, &pkt);
	if (!(ret < 0)) {
		AVFrame *f = _current_video.frame;
		ret = avcodec_receive_frame(_current_video.context, f);
		if (ret == 0 && f->format == AV_PIX_FMT_YUV420P) {
			System_UpdateScreenYUV(f->width, f->height, f->data[0], f->linesize[0], f->data[1], f->linesize[1], f->data[2], f->linesize[2]);
		}
	}
	av_packet_unref(&pkt);
	++_current_video.current_frame;
	return 0;
}
