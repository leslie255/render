# ASCII 3D

**ASCII 3D rendering from scratch in C**

https://github.com/leslie255/ascii3d/assets/105544303/92b3561a-9c7a-43e6-a040-08f327935f81

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

