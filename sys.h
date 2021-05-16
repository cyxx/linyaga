
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

typedef void (*sys_audio_cb)(void *, uint8_t *data, int len);

int	System_Init();
void	System_Fini();
void	System_set_screen_size(int w, int h, const char *caption, int scale, const char *filter, bool fullscreen);
void	System_set_screen_title(const char *caption);
void	System_update_screen(const void *p, int present);
int	System_LoadCursor(const uint32_t *rgba, int w, int h);
void	System_SetCursor(int num);
bool	System_PollEvent(struct event_t *ev);
void	System_StartAudio(sys_audio_cb callback, void *param);
void	System_StopAudio();
void	System_LockAudio();
void	System_UnlockAudio();

#endif /* SYS_H__ */
