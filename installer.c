
#include <Python.h>
#include "intern.h"


static char *_archive_path;
static uint32_t _archive_offset;
static uint8_t *_archive_toc, *_archive_tocend;
static FILE *_archive_fp;


static void open_archive() {
	static const uint8_t SIG[] = { 0x4D, 0x45, 0x49, 0x0C, 0x0B, 0x0A, 0x0B, 0x0E };

	_archive_fp = fopen(_archive_path, "rb");
	assert(_archive_fp);

	fseek(_archive_fp, 0, SEEK_END);
	const uint32_t fileSize = ftell(_archive_fp);
	fseek(_archive_fp, -24, SEEK_END);

	uint8_t buf[24];
	fread(buf, 24, 1, _archive_fp);
	assert(memcmp(buf, SIG, sizeof(SIG)) == 0);
	_archive_offset = fileSize - READ_BE_UINT32(buf + 8);

	const uint32_t pos = _archive_offset + READ_BE_UINT32(buf + 12);
	fseek(_archive_fp, pos, SEEK_SET);
	const int size_toc = READ_BE_UINT32(buf + 16);
	_archive_toc = (uint8_t *)malloc(size_toc);
	if (!_archive_toc) {
		fprintf(stderr, "Failed to allocate %d bytes\n", size_toc);
	} else {
		fread(_archive_toc, size_toc, 1, _archive_fp);
		_archive_tocend = _archive_toc + size_toc;
		for (const uint8_t *p = _archive_toc; p < _archive_tocend; p += READ_BE_UINT32(p)) {
			// fprintf(stdout, "TOC entry #%d type %c\n", count, p[17]);
			assert(strchr("zms", p[17]));
		}
	}
}

static void close_archive() {
	if (_archive_fp) {
		fclose(_archive_fp);
		_archive_fp = 0;
	}
	if (_archive_toc) {
		free(_archive_toc);
		_archive_toc = 0;
	}
}

static char *extract_script(uint32_t offset, uint32_t size) {
	char *buf = (char *)malloc(size + 1);
	if (!buf) {
		fprintf(stderr, "Failed to allocate %d bytes\n", size);
	} else {
		fseek(_archive_fp, offset, SEEK_SET);
		fread(buf, size, 1, _archive_fp);
		buf[size] = 0;
	}
	return buf;
}

static void add_paths() {
	for (uint8_t *p = _archive_toc; p < _archive_tocend; p += READ_BE_UINT32(p)) {
		if (p[17] != 'z') {
			continue;
		}
		char buf[512];
		const uint32_t offset = _archive_offset + READ_BE_UINT32(p + 4);
		snprintf(buf, sizeof(buf), "import sys; sys.path.append(r\"%s?%d\")", _archive_path, offset);
		PyRun_SimpleString(buf);
	}
	PyRun_SimpleString("import sys; sys.path.append('yaga')");
}

static void set_argv(int argc, char *args[]) {
	PyObject *argv = PyList_New(0);
	for (int i = 0; i < argc; ++i) {
		PyObject *obj = Py_BuildValue("s", args[i]);
		PyList_Append(argv, obj);
	}

	PyObject *module = PyImport_ImportModule("sys");
	PyObject_SetAttrString(module, "argv", argv);
	if (PyErr_Occurred()) {
		PyErr_Print();
		PyErr_Clear();
	}
}

static void unmarshall_modules() {
	PyObject *module = PyImport_ImportModule("marshal");
	PyObject *module_dict = PyModule_GetDict(module);
	PyObject *load_method = PyDict_GetItemString(module_dict, "load");
	PyObject *file_object = PyFile_FromFile(_archive_fp, _archive_path, "rb+", 0);

	for (uint8_t *p = _archive_toc; p < _archive_tocend; p += READ_BE_UINT32(p)) {
		if (p[17] != 'm') {
			continue;
		}
		const uint32_t offset = _archive_offset + READ_BE_UINT32(p + 4) + 8;
		fseek(_archive_fp, offset, SEEK_SET);
		char *moduleName = (char *)p + 18;
		PyObject *object_code = PyObject_CallFunction(load_method, "O", file_object);
		PyImport_ExecCodeModule(moduleName, object_code);
		if (PyErr_Occurred()) {
			PyErr_Print();
			PyErr_Clear();
		}
	}
}

static void execute_scripts() {
	for (uint8_t *p = _archive_toc; p < _archive_tocend; p += READ_BE_UINT32(p)) {
		if (p[17] != 's') {
			continue;
		}
		const uint32_t offset = _archive_offset + READ_BE_UINT32(p + 4);
		const uint32_t size = READ_BE_UINT32(p + 8);
		assert(p[16] == 2); /* uncompressed data */
		char *script = extract_script(offset, size);
		PyRun_SimpleString(script);
		if (PyErr_Occurred()) {
			PyErr_Print();
			PyErr_Clear();
		}
		free(script);
	}
}

void inityagahost();
void initzlib();

void Installer_Main(int argc, char *argv[]) {
	_archive_path = argv[0];
	open_archive();
	Py_SetProgramName(_archive_path);
	Py_Initialize();
	set_argv(argc, argv);
	add_paths();
	inityagahost();
	initzlib();
	unmarshall_modules();
	execute_scripts();
	close_archive();
	Py_Finalize();
}
