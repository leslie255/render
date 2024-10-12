CC ?= clang
CFLAGS = -Wall -Wconversion --std=gnu2x
DEBUG_FLAGS = -g -O1 -DDEBUG
RELEASE_FLAGS = -O2
LDFLAGS = 

ifeq ($(RENDER_MODE),raylib)
	CFLAGS += -I./lib/raylib/src/ -DUSE_RAYLIB
	LDFLAGS += ./lib/raylib/src/libraylib.a
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S), Darwin)
    	LDFLAGS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
    endif
    ifeq ($(UNAME_S), Linux)
    	LDFLAGS += -ldl -lpthread
    endif
endif

ifeq ($(MODE),release)
	CFLAGS += $(RELEASE_FLAGS)
else
	CFLAGS += $(DEBUG_FLAGS)
endif

all: bin/main.o bin/ascii3d

libs:
	cd lib/raylib/src && make PLATFORM=PLATFORM_DESKTOP

clean:
	rm -rf bin/*

bin/main.o: src/main.c src/common.h src/debug_utils.h src/mat.h
	$(CC) $(CFLAGS) -c src/main.c -o bin/main.o

bin/ascii3d: bin/main.o
	$(CC) $(LDFLAGS) bin/main.o -o bin/ascii3d
