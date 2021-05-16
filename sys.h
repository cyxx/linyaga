
#ifndef SYS_H__
#define SYS_H__

#include "intern.h"

#define INPUT_DIRECTION_LEFT  (1 << 0)
#define INPUT_DIRECTION_RIGHT (1 << 1)
#define INPUT_DIRECTION_UP    (1 << 2)
#define INPUT_DIRECTION_DOWN  (1 << 3)

#define SYS_AUDIO_FREQ 22050

struct event_t {
	enum {
		EVENT_QUIT,
		EVENT_MOUSE_MOTION,
		EVENT_MOUSE_BUTTON_DOWN,
		EVENT_MOUSE_BUTTON_UP,
		EVENT_KEY_DOWN,
		EVENT_KEY_UP,
		EVENT_WINDOW_FOCUS,
	} type;
	union {
		struct {
			int x, y;
		} mouse_motion;
		int mouse_button;
		int key_code;
		int window_focus;
	} data;
};

enum {
	KEYCODE_FIRST = 0x100,
	KEYCODE_ESCAPE = KEYCODE_FIRST,
	KEYCODE_F1,
	KEYCODE_F2,
	KEYCODE_F3,
	KEYCODE_F4,
	KEYCODE_F5,
	KEYCODE_F6,
	KEYCODE_F7,
	KEYCODE_F8,
	KEYCODE_F9,
	KEYCODE_F10,
	KEYCODE_F11,
	KEYCODE_F12,
	KEYCODE_BACKSPACE,
	KEYCODE_TAB,
	KEYCODE_ENTER,
	KEYCODE_SHIFT,
	KEYCODE_CONTROL,
	KEYCODE_ALT,
	KEYCODE_UP,
	KEYCODE_DOWN,
	KEYCODE_LEFT,
	KEYCODE_RIGHT
};

typedef void (*SysAudioCb)(void *, uint8_t *data, int len);

int	System_Init();
void	System_Fini();

void	System_SetIcon(const uint8_t *data, int size);
void	System_SetScreenWindowed(int flag);
void	System_SetScreenSize(int w, int h);
void	System_SetScreenTitle(const char *caption);
void	System_UpdateScreen(const void *p, int present);
int	System_LoadCursor(const uint32_t *rgba, int w, int h);
void	System_SetCursor(int num);
bool	System_PollEvent(struct event_t *ev);
void	System_StartAudio(SysAudioCb callback, void *param);
void	System_StopAudio();
void	System_LockAudio();
void	System_UnlockAudio();

#endif /* SYS_H__ */
