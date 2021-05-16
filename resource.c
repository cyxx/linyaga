
#include <dirent.h>
#include <sys/stat.h>
#include <sys/param.h>
#include "animation.h"
#include "resource.h"
#include "zipfile.h"

static const char *_data_path;

static char **_filenames;
static int _filenames_count;

static void list_filenames(const char *dirname, int dirname_offset) {
	DIR *d = opendir(dirname);
	if (d) {
		struct dirent *de;
		while ((de = readdir(d)) != NULL) {
			if (de->d_name[0] == '.') {
				continue;
			}
			char path[MAXPATHLEN];
			snprintf(path, sizeof(path), "%s/%s", dirname, de->d_name);
			struct stat st;
			if (stat(path, &st) == 0) {
				if (S_ISDIR(st.st_mode)) {
					list_filenames(path, dirname_offset);
				} else {
					_filenames = (char **)realloc(_filenames, (_filenames_count + 1) * sizeof(char *));
					if (_filenames) {
						_filenames[_filenames_count] = strdup(path + dirname_offset);
						++_filenames_count;
					}
				}
			}
		}
		closedir(d);
	}
}

static const char *DIRS[] = { "interface", "movies", 0 };

#define MAX_ZIPFILES 10

static struct {
	char *name;
	struct zipfile_t *zf;
} _zipfiles[MAX_ZIPFILES];

static void list_datafiles(const char *path) {
	DIR *d = opendir(path);
	if (d) {
		int count = 0;
		struct dirent *de;
		while ((de = readdir(d)) != NULL) {
			if (de->d_name[0] == '.') {
				continue;
			}
			const char *ext = strrchr(de->d_name, '.');
			if (ext && strcasecmp(ext + 1, "he") == 0) {
				char hepath[MAXPATHLEN];
				snprintf(hepath, sizeof(hepath), "%s/%s", path, de->d_name);
				struct zipfile_t *zf = Zipfile_Open(hepath);
				if (zf) {
					assert(count < MAX_ZIPFILES);
					_zipfiles[count].name = strdup(de->d_name);
					_zipfiles[count].zf = zf;
					++count;
				}
				continue;
			}
			struct stat st;
			if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
				char dirpath[MAXPATHLEN];
				snprintf(dirpath, sizeof(dirpath), "%s/%s", path, de->d_name);
				for (int i = 0; DIRS[i]; ++i) {
					if (strcasecmp(de->d_name, DIRS[i]) == 0) {
						list_filenames(dirpath, strlen(path) + 1);
						break;
					}
				}
			}
		}
		closedir(d);
		fprintf(stdout, "Found %d files\n", _filenames_count);
		fprintf(stdout, "Found %d zipfiles\n", count);
	}
}

#define MAX_FILES 256

struct file_t {
	FILE *fp;
	int animation_num;
	struct file_t *next_free;
};

static struct file_t _files[MAX_FILES];
static struct file_t *_next_file;

static struct file_t *find_free_file() {
	struct file_t *file = _next_file;
	if (file) {
		_next_file = file->next_free;
		file->next_free = 0;
	} else {
		fprintf(stderr, "find_free_file MAX_FILES\n");
	}
	return file;
}

static void free_file(struct file_t *file) {
	file->next_free = _next_file;
	_next_file = file;
}

void Resource_Init() {
	const char *path = getenv("DATAPATH");
	if (!path) {
		path = ".";
	}
	fprintf(stdout, "Using %s as data files directory\n", path);
	_data_path = path;
	list_datafiles(path);
	_next_file = &_files[0];
	for (int i = 0; i < MAX_FILES - 1; ++i) {
		_files[i].next_free = &_files[i + 1];
	}
}

void Resource_Fini() {
	for (int i = 0; i < _filenames_count; ++i) {
		free(_filenames[i]);
	}
	_filenames_count = 0;
	free(_filenames);
	_filenames = 0;
}

static void fix_path(const char *name, char *buf) {
	int sep = 0;
	for (; *name; ++name) {
		if (*name == '\\' || *name == '/') {
			if (sep) {
				continue;
			}
			*buf++ = '/';
			sep = 1;
		} else {
			*buf++ = *name;
			sep = 0;
		}
	}
	*buf = 0;
}

static FILE *open_file(const char *original_name) {
	char name[MAXPATHLEN];
	fix_path(original_name, name);
	FILE *fp = 0;
	const char *sep = strchr(name, '/');
	if (sep) {
		const char *filepath = sep + 1;
		for (int i = 0; i < MAX_ZIPFILES && _zipfiles[i].name; ++i) {
			if (strncasecmp(_zipfiles[i].name, name, sep - name) == 0) {
				struct zipentry_t *ze = Zipfile_Find(_zipfiles[i].zf, filepath);
				// fprintf(stdout, "%s in zipfile %s, ze %p\n", name, _zipfiles[i].name, (void *)ze);
				if (ze && Zipfile_GetEntrySize(_zipfiles[i].zf, ze) != 0) {
					fp = Zipfile_OpenEntry(_zipfiles[i].zf, ze);
				}
				break;
			}
		}
	}
	if (!fp) {
		/* local */
		fp = fopen(name, "rb");
	}
	if (!fp) {
		char buf[MAXPATHLEN];
		snprintf(buf, sizeof(buf), "%s/%s", _data_path, name);
		fp = fopen(buf, "rb");
		/* case insensitive */
		if (!fp) {
			for (int i = 0; i < _filenames_count; ++i) {
				if (strcasecmp(buf + strlen(_data_path) + 1, _filenames[i]) == 0) {
					snprintf(buf, sizeof(buf), "%s/%s", _data_path, _filenames[i]);
					fp = fopen(buf, "rb");
					break;
				}
			}
		}
	}
	if (!fp) {
		fprintf(stderr, "Failed to open '%s'\n", name);
		return 0;
	}
	return fp;
}

int Resource_Exists(const char *name) {
	FILE *fp = open_file(name);
	if (fp) {
		fclose(fp);
		return 1;
	}
	return 0;
}

FILE *Resource_Open(const char *name) {
	return open_file(name);
}

int Resource_LoadAnimation(const char *name) {
	FILE *fp = open_file(name);
	if (!fp) {
		return -1;
	}
	struct file_t *file = find_free_file();
	if (!file) {
		fclose(fp);
		return -1;
	}
	const int num = file - _files;
	file->fp = fp;
	const char *ext = strrchr(name, '.');
	if (ext) {
		file->animation_num = Animation_Load(fp, ext + 1);
		// fprintf(stdout, "animation %s num %d asset %d\n", ext, num, file->animation_num);
	} else {
		file->animation_num = -1;
	}
	return num;
}

void Resource_FreeAnimation(int num) {
	struct file_t *file = &_files[num];
	if (file->fp) {
		fclose(file->fp);
		file->fp = 0;
	}
	if (!(file->animation_num < 0)) {
		Animation_Free(file->animation_num);
		file->animation_num = -1;
	}
	free_file(file);
}

int Resource_GetAnimationIndex(int num) {
	return _files[num].animation_num;
}
