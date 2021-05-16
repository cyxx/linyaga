
#include <Python.h>
#include "animation.h"
#include "mixer.h"
#include "resource.h"
#include "sys.h"

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

static PyObject *yagahost_setscreensize(PyObject *self, PyObject *args) {
	int w, h, fmt;

	if (!PyArg_ParseTuple(args, "iii", &w, &h, &fmt)) {
		return 0;
	}
	//fprintf(stdout, "setScreenSize %d,%d,%d\n", w, h, fmt);
	System_set_screen_size(w, h, "", 1, "linear", false);
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
	System_update_screen(_screenBuffer, 1);
	Py_RETURN_NONE;
}

static PyObject *yagahost_setscreentitle(PyObject *self, PyObject *args) {
	char *name;

	if (!PyArg_ParseTuple(args, "s", &name)) {
		return 0;
	}
	System_set_screen_title(name);
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
		if (!(frame < 0)) {
			Animation_Seek(anim, frame);
		}
		int x, y, w, h;
		struct surface_t surface = {
			_screenBuffer,
			_screenW,
			_screenH
		};
		Animation_Draw(anim, &surface, dx, dy, mask, &x, &y, &w, &h);
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
	if (System_poll_event(&ev)) {
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

static PyMethodDef yagahost_methods[] = {
	{ "HasAsset", yagahost_hasasset, METH_VARARGS, "" },
	{ "LoadAsset", yagahost_loadasset, METH_VARARGS, "" },
	{ "FreeAsset", yagahost_freeasset, METH_VARARGS, "" },
	{ "OpenAsset", yagahost_openasset, METH_VARARGS, "" },
	{ "SetScreenSize", yagahost_setscreensize, METH_VARARGS, "" },
	{ "ClearScreen", yagahost_clearscreen, METH_VARARGS, "" },
	{ "UpdateScreen", yagahost_updatescreen, METH_VARARGS, "" },
	{ "SetScreenTitle", yagahost_setscreentitle, METH_VARARGS, "" },
	{ "GetAnimationFramesCount", yagahost_getanimationframescount, METH_VARARGS, "" },
	{ "DrawAnimationFrame", yagahost_drawanimationframe, METH_VARARGS, "" },
	{ "EnableAnimationFrameLayer", yagahost_enableanimationframelayer, METH_VARARGS, "" },
	{ "PlayAudio", yagahost_playaudio, METH_VARARGS, "" },
	{ "StopAudio", yagahost_stopaudio, METH_VARARGS, "" },
	{ "IsAudioPlaying", yagahost_isaudioplaying, METH_VARARGS, "" },
	{ "PollEvent", yagahost_pollevent, METH_VARARGS, "" },
	{ 0, 0, 0, 0 }
};

void inityagahost(void) { // PyMODINIT_FUNC
	PyObject *m;

	m = Py_InitModule("yagahost", yagahost_methods);
	if (!m) {
		return;
	}

	PyModule_AddIntConstant(m, "EVENT_QUIT", EVENT_QUIT);
	PyModule_AddIntConstant(m, "EVENT_MOUSE_MOTION", EVENT_MOUSE_MOTION);
	PyModule_AddIntConstant(m, "EVENT_MOUSE_BUTTON_DOWN", EVENT_MOUSE_BUTTON_DOWN);
	PyModule_AddIntConstant(m, "EVENT_MOUSE_BUTTON_UP", EVENT_MOUSE_BUTTON_UP);
	PyModule_AddIntConstant(m, "EVENT_KEY_DOWN", EVENT_KEY_DOWN);
	PyModule_AddIntConstant(m, "EVENT_KEY_UP", EVENT_KEY_UP);
	PyModule_AddIntConstant(m, "EVENT_WINDOW_FOCUS", EVENT_WINDOW_FOCUS);

	if (0) {
		System_Init();
		Animation_Init();
		Resource_Init();
	}
}
