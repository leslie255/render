# RENDER

**3D rendering from scratch in C**

https://github.com/leslie255/ascii3d/assets/105544303/d806f367-01a7-4df6-b836-3d72fa569800

https://github.com/leslie255/render/assets/105544303/e12e372d-9f73-490e-a3a0-b7ba03988bac

## Building

Render in GUI window (using [raylib](https://github.com/raysan5/raylib) to draw the pixels into a window):

```
$ mkdir bin/
$ make libs
$ make all MODE=release RENDER_MODE=raylib
$ ./bin/ascii3d
```

The GUI version currently only builds on macOS and Linux due to dependency managing, but the ASCII supports pretty much anything.

You could also try using [yeb](https://github.com/leslie255/yeb) for building, which is a build system in C that bootstraps itself from a single header file:

```
# Bootstrap yeb
$ cc build.c && ./a.out
# ASCII:
$ ./yeb/yeb --release       
$ ./bin/render
# GUI:
$ ./yeb/yeb libs
$ ./yeb/yeb --release --gui
$ ./bin/render
```

## LICENSE

This repo is licensed under GPLv3
