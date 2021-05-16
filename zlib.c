
#include <Python.h>
#include <zlib.h>
#include "intern.h"

static PyObject *_zlibException;

static void ZlibFormatException(z_stream *z_str, const char *str, int code) {
	if (z_str->msg) {
		PyErr_Format(_zlibException, "Error %d %s: %.200s", code, str, z_str->msg);
	} else {
		PyErr_Format(_zlibException, "Error %d %s", code, str);
	}
}

static PyObject *ZlibDecompress(PyObject *self, PyObject *args) {
	uint32_t len, wbits, bufsize;
	char *buf;

	wbits = MAX_WBITS;
	bufsize = 16 * 1024;

	if (!PyArg_ParseTuple(args, "s#", &buf, &len)) {
		return 0;
	}

	PyObject *py_string = PyString_FromStringAndSize(0, bufsize);
	if (!py_string) {
		return 0;
	}

	z_stream z_str;
	z_str.avail_in = len;
	z_str.avail_out = bufsize;
	z_str.zalloc = 0;
	z_str.zfree = 0;
	z_str.next_in = (Bytef *)buf;
	z_str.next_out = (Bytef *)&((PyStringObject *)py_string)->ob_sval[0];
	int ret = inflateInit2(&z_str, wbits);
	if (ret == Z_MEM_ERROR) {
		PyErr_SetString(PyExc_MemoryError, "Out of memory while decompressing data");
	} else if (ret != Z_OK) {
		inflateEnd(&z_str);
		ZlibFormatException(&z_str, "while preparing to decompress data", ret);
	} else {
		do {
			PyThreadState *state = PyEval_SaveThread();
			ret = inflate(&z_str, Z_FINISH);
			PyEval_RestoreThread(state);
			switch (ret) {
			case Z_STREAM_END:
				break;
			case Z_BUF_ERROR:
				if (z_str.avail_out > 0) {
					inflateEnd(&z_str);
					PyErr_Format(_zlibException, "Error %i while decompressing data", Z_BUF_ERROR);
					goto ret_decref;
				}
			case Z_OK:
				if (_PyString_Resize(&py_string, bufsize * 2) == -1) {
					inflateEnd(&z_str);
					goto ret_decref;
				}
				z_str.next_out = (Bytef *)&((PyStringObject *)py_string)->ob_sval[bufsize];
				z_str.avail_out = bufsize;
				bufsize *= 2;
				break;
			default:
				inflateEnd(&z_str);
				ZlibFormatException(&z_str, "while decompressing data", ret);
				goto ret_decref;
			}
		} while (ret != Z_STREAM_END);
		ret = inflateEnd(&z_str);
		if (ret == Z_OK) {
			_PyString_Resize(&py_string, z_str.total_out);
			return py_string;
		}
		ZlibFormatException(&z_str, "while finishing data decompression", ret);
	}
ret_decref:
	assert(0);
	Py_DECREF(py_string);
	return 0;
}

static PyMethodDef ZlibMethods[] = {
	{ "decompress", ZlibDecompress, 1, "" },
	{ 0, 0, 0, 0 }
};

void initzlib() {
	PyObject *module = Py_InitModule("zlib", ZlibMethods);
	PyObject *dict = PyModule_GetDict(module);

	_zlibException = PyErr_NewException("zlib.error", 0, 0);
	if (_zlibException) {
		PyDict_SetItemString(dict, "error", _zlibException);
	}
}
