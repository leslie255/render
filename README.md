# ASCII 3D

**ASCII 3D rendering from scratch in C**

## Building

```
$ mkdir bin/
$ make all MODE=release
$ ./bin/ascii3d
```

This project also supports rendering into a GUI window with with [raylib](https://github.com/raysan5/raylib).
The GUI version could be built with `RENDER_MODE=raylib`:

```
$ mkdir bin/
$ make all RENDER_MODE=raylib
$ ./bin/ascii3d
```

The GUI version is used for debug during development as the ASCII render would just turn into a blob of unrecognizable shape if the rendering bugged out.

The GUI version only builds on macOS and Linux (but the ASCII version can run on anything that runs C and displays text!).

## LICENSE

This repo is licensed under GPLv3

