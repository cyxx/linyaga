
#include <Python.h>
#include "animation.h"
#include "font.h"
#include "mixer.h"
#include "resource.h"
#include "sys.h"
#include "video.h"

#if PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION == 2
#define Py_RETURN_NONE  return Py_INCREF(Py_None), Py_None
#define Py_RETURN_TRUE  return Py_INCREF(Py_True), Py_True
#define Py_RETURN_FALSE return Py_INCREF(Py_False), Py_False
#define PyFile_IncUseCount( x )
#define PyFile_DecUseCount( x )
#endif

static uint32_t *_screenBuffer;
static int _screenW, _screenH;

static PyObject *yagahost_hasasset(PyObject *self, PyObject *args) {
	const char *path;

	if (!PyArg_ParseTuple(args, "s", &path)) {
		return 0;
	}
	if (Resource_Exists(path)) {
		Py_RETURN_TRUE;
	} else {
		Py_RETURN_FALSE;
	}
}

static PyObject *yagahost_loadasset(PyObject *self, PyObject *args) {
	const char *path;

	if (!PyArg_ParseTuple(args, "s", &path)) {
		return 0;
	}
	const int num = Resource_LoadAnimation(path);
	// fprintf(stdout, "loadAsset '%s' : %d\n", path, num);
	return PyInt_FromLong(num);
}

static PyObject *yagahost_freeasset(PyObject *self, PyObject *args) {
	int num;

	if (!PyArg_ParseTuple(args, "i", &num)) {
		return 0;
	}
	// fprintf(stdout, "closeAsset %d\n", num);
	Resource_FreeAnimation(num);
	Py_RETURN_NONE;
}

static PyObject *yagahost_openasset(PyObject *self, PyObject *args) {
	const char *path;

	if (!PyArg_ParseTuple(args, "s", &path)) {
		return 0;
	}
	FILE *fp = Resource_Open(path);
	// fprintf(stdout, "openAsset '%s' : %p\n", path, (void *)fp);
	if (fp) {
		return PyFile_FromFile(fp, path, "r", fclose);
	}
	Py_RETURN_NONE;
}

static PyObject *yagahost_setscreenwindowed(PyObject *self, PyObject *args) {
	int flag;

	if (!PyArg_ParseTuple(args, "i", &flag)) {
		return 0;
	}
	System_SetScreenWindowed(flag);
	Py_RETURN_NONE;
}

static PyObject *yagahost_setscreensize(PyObject *self, PyObject *args) {
	int w, h, fmt;

	if (!PyArg_ParseTuple(args, "iii", &w, &h, &fmt)) {
		return 0;
	}
	System_SetScreenSize(w, h);
	_screenBuffer = (uint32_t *)malloc(w * h * sizeof(uint32_t));
	_screenW = w;
	_screenH = h;
	Py_RETURN_NONE;
}

static PyObject *yagahost_clearscreen(PyObject *self, PyObject *args) {
	memset(_screenBuffer, 0, _screenW * _screenH * sizeof(uint32_t));
	Py_RETURN_NONE;
}

static PyObject *yagahost_updatescreen(PyObject *self, PyObject *args) {
	System_UpdateScreen(_screenBuffer);
	Py_RETURN_NONE;
}

static PyObject *yagahost_setscreentitle(PyObject *self, PyObject *args) {
	char *name;

	if (!PyArg_ParseTuple(args, "s", &name)) {
		return 0;
	}
	System_SetScreenTitle(name);
	Py_RETURN_NONE;
}

static PyObject *yagahost_getanimationframescount(PyObject *self, PyObject *args) {
	int res;

	if (!PyArg_ParseTuple(args, "i", &res)) {
		return 0;
	}
	const int anim = Resource_GetAnimationIndex(res);
	if (!(anim < 0)) {
		const int frames_count = Animation_GetFramesCount(anim);
		return PyInt_FromLong(frames_count);
	}
	return PyInt_FromLong(0);
}

static PyObject *yagahost_getanimationframelayerscount(PyObject *self, PyObject *args) {
	int res, frame;

	if (!PyArg_ParseTuple(args, "ii", &res, &frame)) {
		return 0;
	}
	const int anim = Resource_GetAnimationIndex(res);
	if (!(anim < 0)) {
		const int layers_count = Animation_GetFrameLayersCount(anim, frame);
		return PyInt_FromLong(layers_count);
	}
	return PyInt_FromLong(0);
}

static PyObject *yagahost_getanimationframerect(PyObject *self, PyObject *args) {
	int res, frame;

	if (!PyArg_ParseTuple(args, "ii", &res, &frame)) {
		return 0;
	}
	const int anim = Resource_GetAnimationIndex(res);
	if (!(anim < 0)) {
		int x, y, w, h;
		Animation_GetFrameRect(anim, frame, &x, &y, &w, &h);
		PyObject *obj = PyDict_New();
		PyDict_SetItemString(obj, "x", PyInt_FromLong(x));
		PyDict_SetItemString(obj, "y", PyInt_FromLong(y));
		PyDict_SetItemString(obj, "w", PyInt_FromLong(w));
		PyDict_SetItemString(obj, "h", PyInt_FromLong(h));
		return obj;
	}
	Py_RETURN_NONE;
}

static PyObject *yagahost_getanimationframelayerrect(PyObject *self, PyObject *args) {
	int res, frame, layer;

	if (!PyArg_ParseTuple(args, "iii", &res, &frame, &layer)) {
		return 0;
	}
	const int anim = Resource_GetAnimationIndex(res);
	if (!(anim < 0)) {
		const struct layer_t *l = Animation_GetLayer(anim, frame, layer);
		PyObject *obj = PyDict_New();
		PyDict_SetItemString(obj, "x", PyInt_FromLong(l->x));
		PyDict_SetItemString(obj, "y", PyInt_FromLong(l->y));
		PyDict_SetItemString(obj, "w", PyInt_FromLong(l->w));
		PyDict_SetItemString(obj, "h", PyInt_FromLong(l->h));
		return obj;
	}
	Py_RETURN_NONE;
}

static PyObject *yagahost_seekanimationframe(PyObject *self, PyObject *args) {
	int res, frame;

	if (!PyArg_ParseTuple(args, "ii", &res, &frame)) {
		return 0;
	}
	const int anim = Resource_GetAnimationIndex(res);
	if (!(anim < 0)) {
		Animation_Seek(anim, frame);
	}
	Py_RETURN_NONE;
}

static PyObject *yagahost_drawanimationframe(PyObject *self, PyObject *args) {
	int res, frame;
	float opacity;
	int mask;
	int dx = 0;
	int dy = 0;

	if (!PyArg_ParseTuple(args, "iifiii", &res, &frame, &opacity, &mask, &dx, &dy)) {
		return 0;
	}
	const int anim = Resource_GetAnimationIndex(res);
	//fprintf(stdout, "drawAnimationFrame %d,%d anim %d dx:%d dy:%d\n", res, frame, anim, dx, dy);
	if (!(anim < 0)) {
		int x, y, w, h;
		struct surface_t surface = {
			_screenBuffer,
			_screenW,
			_screenH
		};
		const int alpha = (int)(opacity * 256);
		Animation_Draw(anim, &surface, dx, dy, mask, alpha, &x, &y, &w, &h);
		// render rect
		PyObject *obj = PyDict_New();
		PyDict_SetItemString(obj, "x", PyInt_FromLong(x));
		PyDict_SetItemString(obj, "y", PyInt_FromLong(y));
		PyDict_SetItemString(obj, "w", PyInt_FromLong(w));
		PyDict_SetItemString(obj, "h", PyInt_FromLong(h));
		return obj;
	}
	Py_RETURN_NONE;
}

static PyObject *yagahost_enableanimationframelayer(PyObject *self, PyObject *args) {
	int res, frame, state;
	const char *name;

	if (!PyArg_ParseTuple(args, "iisi", &res, &frame, &name, &state)) {
		return 0;
	}
	const int anim = Resource_GetAnimationIndex(res);
	//fprintf(stdout, "EnableAnimationFrameLayer %d,%d anim %d name %s state %d\n", res, frame, anim, name, state);
	if (!(anim < 0)) {
		Animation_SetLayer(anim, frame, name, state);
	}
	Py_RETURN_NONE;
}

static const struct {
	const char *ext;
	int (*play)(FILE *);
} _audioFormats[] = {
	{ "mp3", &Mixer_PlayMp3 },
	{ "wav", &Mixer_PlayWav },
	{ 0, 0 }
};

static PyObject *yagahost_playaudio(PyObject *self, PyObject *args) {
	int sound = -1;
	PyObject *file;
	const char *name;

	if (!PyArg_ParseTuple(args, "Os", &file, &name)) {
		return 0;
	}
	assert(PyFile_CheckExact(file));
	const char *ext = strrchr(name, '.');
	if (ext) {
		++ext;
		for (int i = 0; _audioFormats[i].ext; ++i) {
			if (strcasecmp(_audioFormats[i].ext, ext) == 0) {
				PyFile_IncUseCount((PyFileObject *)file);
				sound = (_audioFormats[i].play)(PyFile_AsFile(file));
				break;
			}
		}
	}
	return PyInt_FromLong(sound);
}

static PyObject *yagahost_stopaudio(PyObject *self, PyObject *args) {
	int sound;
	PyObject *file;

	if (!PyArg_ParseTuple(args, "Oi", &file, &sound)) {
		return 0;
	}
	if (!(sound < 0)) {
		assert(PyFile_CheckExact(file));
		assert(!(sound < 0));
		Mixer_Stop(sound);
		PyFile_DecUseCount((PyFileObject *)file);
	}
	Py_RETURN_NONE;
}

static PyObject *yagahost_isaudioplaying(PyObject *self, PyObject *args) {
	int sound;

	if (!PyArg_ParseTuple(args, "i", &sound)) {
		return 0;
	}
	if (!(sound < 0)) {
		if (Mixer_IsPlaying(sound)) {
			Py_RETURN_TRUE;
		}
	}
	Py_RETURN_FALSE;
}

static PyObject *yagahost_pollevent(PyObject *self, PyObject *args) {
	struct event_t ev;
	if (System_PollEvent(&ev)) {
		PyObject *obj = PyDict_New();
		PyDict_SetItemString(obj, "type", PyInt_FromLong(ev.type));
		switch (ev.type) {
		case EVENT_QUIT:
			break;
		case EVENT_MOUSE_MOTION:
			PyDict_SetItemString(obj, "x", PyInt_FromLong(ev.data.mouse_motion.x));
			PyDict_SetItemString(obj, "y", PyInt_FromLong(ev.data.mouse_motion.y));
			break;
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			PyDict_SetItemString(obj, "button", PyInt_FromLong(ev.data.mouse_button));
			break;
		case EVENT_KEY_DOWN:
		case EVENT_KEY_UP:
			PyDict_SetItemString(obj, "code", PyInt_FromLong(ev.data.key_code));
			break;
		case EVENT_WINDOW_FOCUS:
			PyDict_SetItemString(obj, "focus", PyInt_FromLong(ev.data.window_focus));
			break;
		}
		return obj;
	}
	Py_RETURN_NONE;
}

static PyObject *yagahost_loadcursor(PyObject *self, PyObject *args) {
	int cursor = -1;
	char *name;

	if (!PyArg_ParseTuple(args, "s", &name)) {
		return 0;
	}
	FILE *fp = Resource_Open(name);
	if (!fp) {
		fprintf(stderr, "Failed to open cursor '%s'\n", name);
	} else {
		const char *sep = strrchr(name, '.');
		if (sep) {
			const int anim = Animation_Load(fp, sep + 1);
			if (!(anim < 0)) {
				struct layer_t *layer = Animation_GetLayer(anim, 0, 0);
				if (layer) {
					cursor = System_LoadCursor(layer->rgba, layer->w, layer->h);
				}
				Animation_Free(anim);
			}
		}
		fclose(fp);
	}
	return PyInt_FromLong(cursor);
}

static PyObject *yagahost_setcursor(PyObject *self, PyObject *args) {
	int cursor;

	if (!PyArg_ParseTuple(args, "i", &cursor)) {
		return 0;
	}
	if (!(cursor < 0)) {
		System_SetCursor(cursor);
	}
	Py_RETURN_NONE;
}

static PyObject *yagahost_loadfont(PyObject *self, PyObject *args) {
	int font = -1;
	int res, first_ascii, last_ascii, space_ascii;

	if (!PyArg_ParseTuple(args, "iiii", &res, &first_ascii, &last_ascii, &space_ascii)) {
		return 0;
	}
	const int anim = Resource_GetAnimationIndex(res);
	if (!(anim < 0)) {
		struct layer_t *layer = Animation_GetLayer(anim, 0, 0);
		if (layer) {
			font = Font_Load(layer->rgba, layer->w, layer->h, first_ascii, last_ascii, space_ascii);
		}
	}
	return PyInt_FromLong(font);
}

static PyObject *yagahost_drawfontchar(PyObject *self, PyObject *args) {
	int font, chr, x, y;
	float opacity;

	if (!PyArg_ParseTuple(args, "iiiif", &font, &chr, &x, &y, &opacity)) {
		return 0;
	}
	struct surface_t surface = {
		_screenBuffer,
		_screenW,
		_screenH
	};
	const int alpha = (int)(opacity * 256);
	Font_DrawChar(font, chr, &surface, x, y, alpha);
	Py_RETURN_NONE;
}

static PyObject *yagahost_getfontcharrect(PyObject *self, PyObject *args) {
	int font, chr;

	if (!PyArg_ParseTuple(args, "ii", &font, &chr)) {
		return 0;
	}
	int x, y, w, h;
	Font_GetCharRect(font, chr, &x, &y, &w, &h);
	PyObject *obj = PyDict_New();
	PyDict_SetItemString(obj, "x", PyInt_FromLong(x));
	PyDict_SetItemString(obj, "y", PyInt_FromLong(y));
	PyDict_SetItemString(obj, "w", PyInt_FromLong(w));
	PyDict_SetItemString(obj, "h", PyInt_FromLong(h));
	return obj;
}

static PyObject *yagahost_playvideo(PyObject *self, PyObject *args) {
	PyObject *file;
	const char *name;

	if (!PyArg_ParseTuple(args, "Os", &file, &name)) {
		return 0;
	}
	assert(PyFile_CheckExact(file));
	PyFile_IncUseCount((PyFileObject *)file);
	const int video = Video_PlayVideo(PyFile_AsFile(file));
	return PyInt_FromLong(video);
}

static PyObject *yagahost_stopvideo(PyObject *self, PyObject *args) {
	int video;
	PyObject *file;

	if (!PyArg_ParseTuple(args, "Oi", &file, &video)) {
		return 0;
	}
	assert(PyFile_CheckExact(file));
	PyFile_DecUseCount((PyFileObject *)file);
	Video_StopVideo(video);
	Py_RETURN_NONE;
}

static PyObject *yagahost_isvideoplaying(PyObject *self, PyObject *args) {
	int video;

	if (!PyArg_ParseTuple(args, "i", &video)) {
		return 0;
	}
	if (Video_IsVideoPlaying(video)) {
		Py_RETURN_TRUE;
	} else {
		Py_RETURN_FALSE;
	}
}

static PyObject *yagahost_rendervideo(PyObject *self, PyObject *args) {
	int video;

	if (!PyArg_ParseTuple(args, "i", &video)) {
		return 0;
	}
	Video_RenderVideo(video);
	Py_RETURN_NONE;
}

static PyMethodDef yagahost_methods[] = {
	{ "HasAsset", yagahost_hasasset, METH_VARARGS, "" },
	{ "LoadAsset", yagahost_loadasset, METH_VARARGS, "" },
	{ "FreeAsset", yagahost_freeasset, METH_VARARGS, "" },
	{ "OpenAsset", yagahost_openasset, METH_VARARGS, "" },
	{ "SetScreenWindowed", yagahost_setscreenwindowed, METH_VARARGS, "" },
	{ "SetScreenSize", yagahost_setscreensize, METH_VARARGS, "" },
	{ "ClearScreen", yagahost_clearscreen, METH_VARARGS, "" },
	{ "UpdateScreen", yagahost_updatescreen, METH_VARARGS, "" },
	{ "SetScreenTitle", yagahost_setscreentitle, METH_VARARGS, "" },
	{ "GetAnimationFramesCount", yagahost_getanimationframescount, METH_VARARGS, "" },
	{ "GetAnimationFrameLayersCount", yagahost_getanimationframelayerscount, METH_VARARGS, "" },
	{ "GetAnimationFrameRect", yagahost_getanimationframerect, METH_VARARGS, "" },
	{ "GetAnimationFrameLayerRect", yagahost_getanimationframelayerrect, METH_VARARGS, "" },
	{ "SeekAnimationFrame", yagahost_seekanimationframe, METH_VARARGS, "" },
	{ "DrawAnimationFrame", yagahost_drawanimationframe, METH_VARARGS, "" },
	{ "EnableAnimationFrameLayer", yagahost_enableanimationframelayer, METH_VARARGS, "" },
	{ "PlayAudio", yagahost_playaudio, METH_VARARGS, "" },
	{ "StopAudio", yagahost_stopaudio, METH_VARARGS, "" },
	{ "IsAudioPlaying", yagahost_isaudioplaying, METH_VARARGS, "" },
	{ "PollEvent", yagahost_pollevent, METH_VARARGS, "" },
	{ "LoadCursor", yagahost_loadcursor, METH_VARARGS, "" },
	{ "SetCursor", yagahost_setcursor, METH_VARARGS, "" },
	{ "LoadFont", yagahost_loadfont, METH_VARARGS, "" },
	{ "DrawFontChar", yagahost_drawfontchar, METH_VARARGS, "" },
	{ "GetFontCharRect", yagahost_getfontcharrect, METH_VARARGS, "" },
	{ "PlayVideo", yagahost_playvideo, METH_VARARGS, "" },
	{ "StopVideo", yagahost_stopvideo, METH_VARARGS, "" },
	{ "IsVideoPlaying", yagahost_isvideoplaying, METH_VARARGS, "" },
	{ "RenderVideo", yagahost_rendervideo, METH_VARARGS, "" },
	{ 0, 0, 0, 0 }
};

static const struct {
	char *name;
	int value;
} _events[] = {
	{ "EVENT_QUIT", EVENT_QUIT },
	{ "EVENT_MOUSE_MOTION", EVENT_MOUSE_MOTION },
	{ "EVENT_MOUSE_BUTTON_DOWN", EVENT_MOUSE_BUTTON_DOWN },
	{ "EVENT_MOUSE_BUTTON_UP", EVENT_MOUSE_BUTTON_UP },
	{ "EVENT_KEY_DOWN", EVENT_KEY_DOWN },
	{ "EVENT_KEY_UP", EVENT_KEY_UP },
	{ "EVENT_WINDOW_FOCUS", EVENT_WINDOW_FOCUS },
	{ 0, 0 }
};

static const struct {
	char *name;
	int value;
} _keys[] = {
	{ "KEY_CODES_BEGIN", KEYCODE_FIRST },
	{ "KEY_ESCAPE", KEYCODE_ESCAPE },
	{ "KEY_F1", KEYCODE_F1 },
	{ "KEY_F2", KEYCODE_F2 },
	{ "KEY_F3", KEYCODE_F3 },
	{ "KEY_F4", KEYCODE_F4 },
	{ "KEY_F5", KEYCODE_F5 },
	{ "KEY_F6", KEYCODE_F6 },
	{ "KEY_F7", KEYCODE_F7 },
	{ "KEY_F8", KEYCODE_F8 },
	{ "KEY_F9", KEYCODE_F9 },
	{ "KEY_F10", KEYCODE_F10 },
	{ "KEY_F11", KEYCODE_F11 },
	{ "KEY_F12", KEYCODE_F12 },
	{ "KEY_BACKSPACE", KEYCODE_BACKSPACE },
	{ "KEY_TAB", KEYCODE_TAB },
	// { "KEY_CAPS", KEYCODE_CAPS },
	{ "KEY_ENTER", KEYCODE_ENTER },
	{ "KEY_SHIFT", KEYCODE_SHIFT },
	{ "KEY_CONTROL", KEYCODE_CONTROL },
	{ "KEY_ALT", KEYCODE_ALT },
	// { "KEY_PAUSE", KEYCODE_PAUSE },
	// { "KEY_INSERT", KEYCODE_INSERT },
	// { "KEY_DELETE", KEYCODE_DELETE },
	// { "KEY_HOME", KEYCODE_HOME },
	// { "KEY_END", KEYCODE_END },
	// { "KEY_PGUP", KEYCODE_PGUP },
	// { "KEY_PGDOWN", KEYCODE_PGDOWN },
	{ "KEY_UP", KEYCODE_UP },
	{ "KEY_DOWN", KEYCODE_DOWN },
	{ "KEY_LEFT", KEYCODE_LEFT },
	{ "KEY_RIGHT", KEYCODE_RIGHT },
	{ 0, 0 }
};

void inityagahost(void) { // PyMODINIT_FUNC
	PyObject *m;

	m = Py_InitModule("yagahost", yagahost_methods);
	if (!m) {
		return;
	}

	for (int i = 0; _events[i].name; ++i) {
		PyModule_AddIntConstant(m, _events[i].name, _events[i].value);
	}
	for (int i = 0; _keys[i].name; ++i) {
		PyModule_AddIntConstant(m, _keys[i].name, _keys[i].value);
	}
}
