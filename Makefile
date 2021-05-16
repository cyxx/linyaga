
SDL_CFLAGS := `sdl2-config --cflags`
SDL_LIBS   := `sdl2-config --libs`

PYTHON_DIR := -I$(PYTHON_INSTALLDIR)/include/python2.2/
PYTHON_LIB := -L$(PYTHON_INSTALLDIR)/lib/python2.2/config/ -lpython2.2

CPPFLAGS += -Wall -Wpedantic -Wno-unused-result -MMD $(PYTHON_DIR) -g $(SDL_CFLAGS) -D_GNU_SOURCE -Ithird_party/ -O

SRCS = animation.c animation_mng.c animation_rle.c font.c installer.c main.c mixer.c resource.c sys_sdl2.c yagahost.c zipfile.c zlib.c

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

yagaboot: $(OBJS)
	$(CC) -export-dynamic -o $@ $^ $(PYTHON_LIB) $(SDL_LIBS) -lm -ldl -pthread -lutil -lz

clean:
	rm -f $(OBJS) $(DEPS) yagaboot

-include $(DEPS)
