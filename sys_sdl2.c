
#include <SDL.h>
#include "sys.h"

static int _screen_w;
static int _screen_h;
static SDL_Window *_window;
static SDL_Renderer *_renderer;
static SDL_Texture *_texture;

int System_Init() {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_ShowCursor(SDL_ENABLE);
	_screen_w = _screen_h = 0;
	_window = 0;
	_renderer = 0;
	_texture = 0;
	return 0;
}

void System_Fini() {
	if (_texture) {
		SDL_DestroyTexture(_texture);
		_texture = 0;
	}
	if (_renderer) {
		SDL_DestroyRenderer(_renderer);
		_renderer = 0;
	}
	if (_window) {
		SDL_DestroyWindow(_window);
		_window = 0;
	}
	SDL_Quit();
}

void System_set_screen_size(int w, int h, const char *caption, int scale, const char *filter, bool fullscreen) {
	assert(_screen_w == 0 && _screen_h == 0); // abort if called more than once
	_screen_w = w;
	_screen_h = h;
	if (!filter || strcmp(filter, "nearest") == 0) {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	} else if (strcmp(filter, "linear") == 0) {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	} else {
		fprintf(stderr, "Unhandled filter '%s'\n", filter);
	}
	const int window_w = w * scale;
	const int window_h = h * scale;
	const int flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_RESIZABLE;
	_window = SDL_CreateWindow(caption, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_w, window_h, flags);
	_renderer = SDL_CreateRenderer(_window, -1, 0);
	SDL_RenderSetLogicalSize(_renderer, w, h);
	static const uint32_t pfmt = SDL_PIXELFORMAT_RGB888;
	_texture = SDL_CreateTexture(_renderer, pfmt, SDL_TEXTUREACCESS_STREAMING, _screen_w, _screen_h);
}

void System_update_screen(const void *p, int present) {
	SDL_UpdateTexture(_texture, 0, p, _screen_w * sizeof(uint32_t));
	if (present) {
		SDL_RenderClear(_renderer);
		SDL_RenderCopy(_renderer, _texture, 0, 0);
		SDL_RenderPresent(_renderer);
	}
}

void System_set_screen_title(const char *name) {
	SDL_SetWindowTitle(_window, name);
}

bool System_poll_event(struct event_t *event) {
	SDL_Event ev;
	if (SDL_PollEvent(&ev)) {
		switch (ev.type) {
		case SDL_QUIT:
			event->type = EVENT_QUIT;
			return true;
		case SDL_WINDOWEVENT:
			switch (ev.window.event) {
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				event->type = EVENT_WINDOW_FOCUS;
				event->data.window_focus = 1;
				SDL_PauseAudio(0);
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				event->type = EVENT_WINDOW_FOCUS;
				event->data.window_focus = 0;
				SDL_PauseAudio(1);
				break;
			}
			break;
		case SDL_MOUSEMOTION:
                        event->type = EVENT_MOUSE_MOTION;
			event->data.mouse_motion.x = ev.motion.x;
			event->data.mouse_motion.y = ev.motion.y;
			return true;
                case SDL_MOUSEBUTTONDOWN:
			event->type = EVENT_MOUSE_BUTTON_DOWN;
			event->data.mouse_button = 1 << ev.button.button;
			return true;
		case SDL_MOUSEBUTTONUP:
			event->type = EVENT_MOUSE_BUTTON_UP;
			event->data.mouse_button = 1 << ev.button.button;
			return true;
		case SDL_KEYDOWN:
			event->type = EVENT_KEY_DOWN;
			event->data.key_code = ev.key.keysym.sym;
			return true;
		case SDL_KEYUP:
			event->type = EVENT_KEY_UP;
			event->data.key_code = ev.key.keysym.sym;
			return true;
		}
	}
	return false;
}

void System_StartAudio(sys_audio_cb callback, void *param) {
	SDL_AudioSpec desired;
	memset(&desired, 0, sizeof(desired));
	desired.freq = SYS_AUDIO_FREQ;
	desired.format = AUDIO_S16;
	desired.channels = 2;
	desired.samples = 2048;
	desired.callback = callback;
	desired.userdata = param;
	if (SDL_OpenAudio(&desired, 0) == 0) {
		SDL_PauseAudio(0);
	}
}

void System_StopAudio() {
	SDL_CloseAudio();
}

void System_LockAudio() {
	SDL_LockAudio();
}

void System_UnlockAudio() {
	SDL_UnlockAudio();
}
