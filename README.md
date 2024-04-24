# ASCII 3D

**ASCII 3D rendering from scratch in C**

https://github.com/leslie255/ascii3d/assets/105544303/d806f367-01a7-4df6-b836-3d72fa569800

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
$ make libs
$ make all RENDER_MODE=raylib
$ ./bin/ascii3d
```

The GUI version is used for debug during development as the ASCII render would just turn into a blob of unrecognizable shape if the rendering bugged out.

The GUI version only builds on macOS and Linux (but the ASCII version can run on anything that runs C and displays text!).

## TODOs

- Lights don't behave very well currently
- Camera parallax?

## LICENSE

This repo is licensed under GPLv3

