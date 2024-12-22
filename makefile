CC ?= clang
CFLAGS = -Wall -Wconversion --std=gnu2x
DEBUG_FLAGS = -g -O1 -DDEBUG
RELEASE_FLAGS = -O2
LDFLAGS = 

# Raylib-related setups
CFLAGS += -I./lib/raylib/src/ -DUSE_RAYLIB
LDFLAGS += ./lib/raylib/src/libraylib.a
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Darwin)
	LDFLAGS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
endif
ifeq ($(UNAME_S), Linux)
	LDFLAGS += -ldl -lpthread
endif

ifeq ($(MODE),release)
	CFLAGS += $(RELEASE_FLAGS)
else
	CFLAGS += $(DEBUG_FLAGS)
endif

libs:
	cd lib/raylib/src && make PLATFORM=PLATFORM_DESKTOP

cleanlibs:
	cd lib/raylib/src && make clean

all: bin/main.o bin/shaders.o bin/render.o bin/gui.o bin/demo

clean:
	rm -rf bin/*

bin/main.o: src/main.c src/shaders.h src/gui.h src/render.h src/common.h src/debug_utils.h src/linear_alg.h
	$(CC) $(CFLAGS) -c src/main.c -o bin/main.o

bin/render.o: src/render.h src/render.c src/common.h src/debug_utils.h src/linear_alg.h src/math_helpers.h
	$(CC) $(CFLAGS) -c src/render.c -o bin/render.o

bin/shaders.o: src/shaders.h src/shaders.c src/common.h src/debug_utils.h src/linear_alg.h src/math_helpers.h
	$(CC) $(CFLAGS) -c src/shaders.c -o bin/shaders.o

bin/gui.o: src/gui.h src/gui.o src/common.h src/common.h src/debug_utils.h src/linear_alg.h src/math_helpers.h
	$(CC) $(CFLAGS) -c src/gui.c -o bin/gui.o

bin/demo: bin/main.o bin/render.o bin/shaders.o bin/render.o bin/gui.o
	$(CC) $(LDFLAGS) bin/main.o bin/shaders.o bin/render.o bin/gui.o -o bin/demo
