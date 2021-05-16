
#include <SDL.h>
#include "sys.h"

#define MAX_CURSORS 16

struct cursor_t {
	SDL_Cursor *cursor;
	SDL_Surface *surface;
};

static int _screen_w;
static int _screen_h;
static SDL_Window *_window;
static SDL_Renderer *_renderer;
static SDL_Texture *_gameTexture;
static SDL_Texture *_videoTexture;
static SDL_Surface *_icon;
static struct cursor_t _cursors[MAX_CURSORS];
static int _cursors_count;
static bool _fullscreen;
static bool _screen_yuv;

static void init_screen(int w, int h, bool fullscreen) {
	const int flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_RESIZABLE;
	_window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
	if (_icon) {
		SDL_SetWindowIcon(_window, _icon);
	}
	_renderer = SDL_CreateRenderer(_window, -1, 0);
	SDL_RenderSetLogicalSize(_renderer, w, h);
	_gameTexture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, w, h);
	_videoTexture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, w, h);
}

static void fini_screen() {
	if (_gameTexture) {
		SDL_DestroyTexture(_gameTexture);
		_gameTexture = 0;
	}
	if (_videoTexture) {
		SDL_DestroyTexture(_videoTexture);
		_videoTexture = 0;
	}
	if (_renderer) {
		SDL_DestroyRenderer(_renderer);
		_renderer = 0;
	}
	if (_window) {
		SDL_DestroyWindow(_window);
		_window = 0;
	}
}

int System_Init() {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_ShowCursor(SDL_ENABLE);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	_screen_w = _screen_h = 0;
	return 0;
}

void System_Fini() {
	fini_screen();
	if (_icon) {
		SDL_FreeSurface(_icon);
		_icon = 0;
	}
	for (int i = 0; i < _cursors_count; ++i) {
		struct cursor_t *cursor = &_cursors[i];
		SDL_FreeSurface(cursor->surface);
		SDL_FreeCursor(cursor->cursor);
	}
	_cursors_count = 0;
	SDL_Quit();
}

void System_SetIcon(const uint8_t *data, int size) {
	SDL_RWops *rw = SDL_RWFromConstMem(data, size);
	_icon = SDL_LoadBMP_RW(rw, 1);
}

void System_SetScreenWindowed(int flag) {
	_fullscreen = !flag;
	if (_window) {
		SDL_SetWindowFullscreen(_window, _fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	}
}

void System_SetScreenSize(int w, int h) {
	assert(_screen_w == 0 && _screen_h == 0); // abort if called more than once
	_screen_w = w;
	_screen_h = h;
	init_screen(w, h, _fullscreen);
}

void System_UpdateScreen(const void *p) {
	SDL_RenderClear(_renderer);
	if (_screen_yuv) {
		SDL_RenderCopy(_renderer, _videoTexture, 0, 0);
		_screen_yuv = false;
	} else {
		SDL_UpdateTexture(_gameTexture, 0, p, _screen_w * sizeof(uint32_t));
		SDL_RenderCopy(_renderer, _gameTexture, 0, 0);
	}
	SDL_RenderPresent(_renderer);
}

void System_UpdateScreenYUV(int w, int h, const uint8_t *ydata, int ypitch, const uint8_t *udata, int upitch, const uint8_t *vdata, int vpitch) {
	const int ret = SDL_UpdateYUVTexture(_videoTexture, 0, ydata, ypitch, udata, upitch, vdata, vpitch);
	_screen_yuv = (ret == 0);
}

void System_SetScreenTitle(const char *name) {
	SDL_SetWindowTitle(_window, name);
}

int System_LoadCursor(const uint32_t *rgba, int w, int h) {
	if (_cursors_count >= MAX_CURSORS) {
		fprintf(stderr, "System_LoadCursor MAX_CURSORS\n");
		return -1;
	}
	struct cursor_t *cursor = &_cursors[_cursors_count++];
	cursor->surface = SDL_CreateRGBSurfaceFrom(rgba, w, h, 32, w * sizeof(uint32_t), 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
	cursor->cursor = SDL_CreateColorCursor(cursor->surface, 1, 1);
	return cursor - _cursors;
}

void System_SetCursor(int num) {
	assert(num < _cursors_count);
	SDL_SetCursor(_cursors[num].cursor);
}

static int key_code(const SDL_Keysym *key) {
	char code = 0;
	if (key->sym >= SDLK_0 && key->sym <= SDLK_9) {
		code = '0' + key->sym - SDLK_0;
	} else if (key->sym >= SDLK_a && key->sym <= SDLK_z) {
		code = 'a' + key->sym - SDLK_a;
	} else if (key->scancode == SDL_SCANCODE_0) {
		code = '0';
	} else if (key->scancode >= SDL_SCANCODE_1 && key->scancode <= SDL_SCANCODE_9) {
		code = '1' + key->scancode - SDL_SCANCODE_1;
	} else if (key->sym == SDLK_SPACE || key->sym == SDLK_KP_SPACE) {
		code = ' ';
	} else {
		switch (key->sym) {
		case SDLK_ESCAPE:
			return KEYCODE_ESCAPE;
		case SDLK_F1:
			return KEYCODE_F1;
		case SDLK_F2:
			return KEYCODE_F2;
		case SDLK_F3:
			return KEYCODE_F3;
		case SDLK_F4:
			return KEYCODE_F4;
		case SDLK_F5:
			return KEYCODE_F5;
		case SDLK_F6:
			return KEYCODE_F6;
		case SDLK_F7:
			return KEYCODE_F7;
		case SDLK_F8:
			return KEYCODE_F8;
		case SDLK_F9:
			return KEYCODE_F9;
		case SDLK_F10:
			return KEYCODE_F10;
		case SDLK_F11:
			return KEYCODE_F11;
		case SDLK_F12:
			return KEYCODE_F12;
		case SDLK_BACKSPACE:
			return KEYCODE_BACKSPACE;
		case SDLK_TAB:
			return KEYCODE_TAB;
		case SDLK_RETURN:
			return KEYCODE_ENTER;
                case SDLK_RSHIFT:
                case SDLK_LSHIFT:
			return KEYCODE_SHIFT;
		case SDLK_RCTRL:
		case SDLK_LCTRL:
			return KEYCODE_CONTROL;
		case SDLK_RALT:
		case SDLK_LALT:
			return KEYCODE_ALT;
		case SDLK_UP:
			return KEYCODE_UP;
		case SDLK_DOWN:
			return KEYCODE_DOWN;
		case SDLK_LEFT:
			return KEYCODE_LEFT;
		case SDLK_RIGHT:
			return KEYCODE_RIGHT;
		}
	}
	return code;
}

bool System_PollEvent(struct event_t *event) {
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
			event->data.key_code = key_code(&ev.key.keysym);
			return true;
		case SDL_KEYUP:
			event->type = EVENT_KEY_UP;
			event->data.key_code = key_code(&ev.key.keysym);
			return true;
		}
	}
	return false;
}

void System_StartAudio(SysAudioCb callback, void *param) {
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
