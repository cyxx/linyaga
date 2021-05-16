
#include <Python.h>
#include "intern.h"


static uint8_t _archiveHeader[24];
static const char *_archiveFilePath;
static uint32_t _archiveTOCpos;
static uint8_t *_archiveTOCbuf, *_archiveTOCbufend;
static FILE *_archiveFileHandle = 0;


static void open_archive() {
	static const uint8_t SIG[] = { 0x4D, 0x45, 0x49, 0x0C, 0x0B, 0x0A, 0x0B, 0x0E };

	_archiveFileHandle = fopen(_archiveFilePath, "rb");
	assert(_archiveFileHandle);

	fseek(_archiveFileHandle, 0, SEEK_END);
	uint32_t fileSize = ftell(_archiveFileHandle);
	fseek(_archiveFileHandle, -24, SEEK_END);

	fread(_archiveHeader, 24, 1, _archiveFileHandle);
	if (memcmp(_archiveHeader, SIG, sizeof(SIG)) == 0) {
		_archiveTOCpos = fileSize - READ_BE_UINT32(_archiveHeader + 8);

		uint32_t pos = _archiveTOCpos + READ_BE_UINT32(_archiveHeader + 12);
		fseek(_archiveFileHandle, pos, SEEK_SET);
		int sizeTOC = READ_BE_UINT32(_archiveHeader + 16);

		_archiveTOCbuf = (uint8_t *)malloc(sizeTOC);
		fread(_archiveTOCbuf, sizeTOC, 1, _archiveFileHandle);
		_archiveTOCbufend = _archiveTOCbuf + sizeTOC;

		int count = 0;
		for (const uint8_t *p = _archiveTOCbuf; p < _archiveTOCbufend; p += READ_BE_UINT32(p)) {
			fprintf(stdout, "TOC entry #%d type %c\n", count, p[17]);
			assert(strchr("zms", p[17]));
			++count;
		}
	}
}

static void close_archive() {
	if (_archiveFileHandle) {
		fclose(_archiveFileHandle);
		_archiveFileHandle = 0;
	}
	if (_archiveTOCbuf) {
		free(_archiveTOCbuf);
		_archiveTOCbuf = 0;
	}
}

static uint8_t *extract_file(const uint8_t *p) {
	uint32_t pos = _archiveTOCpos + READ_BE_UINT32(p + 4);
	fseek(_archiveFileHandle, pos, SEEK_SET);

	uint32_t size = READ_BE_UINT32(p + 8);
	uint8_t *buf = (uint8_t *)malloc(size);
	if (buf) {
		fread(buf, size, 1, _archiveFileHandle);
		const uint8_t type = p[16];
		assert(type == 2); // uncompressed data
	}
	return buf;
}

static void add_paths() {
	for (uint8_t *p = _archiveTOCbuf; p < _archiveTOCbufend; p += READ_BE_UINT32(p)) {
		if (p[17] != 'z') {
			continue;
		}
		char buf[512];
		int offset = _archiveTOCpos + READ_BE_UINT32(p + 4);
		snprintf(buf, sizeof(buf), "sys.path.append(r\"%s?%d\")", _archiveFilePath, offset);
		PyRun_SimpleString(buf);
	}
}

static void set_argv() {
//	char buf[512];
//	snprintf(buf, sizeof(buf), "import sys; del sys.path[%d:];", 0);
//	PyRun_SimpleString(buf);

	PyObject *argv = PyList_New(0);
	PyObject *obj = Py_BuildValue("s", _archiveFilePath);
	PyList_Append(argv, obj);
//	obj = Py_BuildValue("s", "-l"); // log
	obj = Py_BuildValue("s", "-w"); // window
	PyList_Append(argv, obj);

	PyObject *module = PyImport_ImportModule("sys");
	PyObject_SetAttrString(module, "argv", argv);
	if (PyErr_Occurred()) {
		PyErr_Print();
		PyErr_Clear();
	}

	PyRun_SimpleString("import sys; sys.path.append('yaga')");
}

static void unmarshall_modules() {
	PyObject *module = PyImport_ImportModule("marshal");
	PyObject *moduleDict = PyModule_GetDict(module);
	PyObject *loadMethod = PyDict_GetItemString(moduleDict, "load");
	PyObject *fileObject = PyFile_FromFile(_archiveFileHandle, _archiveFilePath, "rb+", 0);

	for (uint8_t *p = _archiveTOCbuf; p < _archiveTOCbufend; p += READ_BE_UINT32(p)) {
		if (p[17] != 'm') {
			continue;
		}
		const int offset = READ_BE_UINT32(p + 4);
		fseek(_archiveFileHandle, _archiveTOCpos + offset + 8, SEEK_SET);
		char *moduleName = (char *)p + 18;
		PyObject *objectCode = PyObject_CallFunction(loadMethod, "O", fileObject);
		PyObject *code = PyImport_ExecCodeModule(moduleName, objectCode);
		if (PyErr_Occurred()) {
			PyErr_Print();
			PyErr_Clear();
		}
		assert(code != 0);
	}
}

static void execute_scripts() {
	for (const uint8_t *p = _archiveTOCbuf; p < _archiveTOCbufend; p += READ_BE_UINT32(p)) {
		if (p[17] != 's') {
			continue;
		}
		char *scriptData = (char *)extract_file(p);
		PyRun_SimpleString(scriptData);
		if (PyErr_Occurred()) {
			PyErr_Print();
			PyErr_Clear();
		}
		free(scriptData);
	}
}

static void reset_env() {
	// putenv("PYTHONPATH=");
	// putenv("PYTHONHOME=");
}

void inityagahost();
void initzlib();

void Installer_Main(const char *filePath) {
	_archiveFilePath = filePath;
	open_archive();
	reset_env();
	Py_SetProgramName(_archiveFilePath);
	Py_Initialize();
	set_argv();
	add_paths();
	inityagahost();
	initzlib();
	unmarshall_modules();
	execute_scripts();

	close_archive();
	Py_Finalize();
}
