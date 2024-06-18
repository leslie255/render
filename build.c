#include "yeb.h"

bool is_release = false;
bool is_gui = false;

void cc(Cmd *cmd) {
  CMD_APPEND(cmd, "cc");
}

void cflags(Cmd *cmd) {
  CMD_APPEND(cmd, "-Wall -Wconversion --std=gnu17");
  if (is_release)
    CMD_APPEND(cmd, "-O2");
  else
    CMD_APPEND(cmd, "-O1 -g -DDEBUG");
  if (is_gui)
    CMD_APPEND(cmd, "-I./lib/raylib/src/ -DUSE_RAYLIB");
}

void out_path(Cmd *cmd) {
  if (is_gui)
    CMD_APPEND(cmd, "bin/render-gui");
  else
    CMD_APPEND(cmd, "bin/render");
}

void ldflags(Cmd *cmd) {
  if (is_gui) {
    CMD_APPEND(cmd, "./lib/raylib/src/libraylib.a");
#if defined(__WIN32__)
    printf("Unsupported OS!");
    exit(1);
#elif __linux__
    CMD_APPEND(cmd, "-ldl", "-lpthread");
#elif __APPLE__
    CMD_APPEND(cmd, "-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo");
#else
    printf("Unsupported OS!");
    exit(1);
#endif
  }
}

Cmd build_main() {
  Cmd cmd = {0};
  cc(&cmd);
  cflags(&cmd);
  ldflags(&cmd);
  CMD_APPEND(&cmd, "src/main.c -o");
  out_path(&cmd);
  return cmd;
}

Cmd mkdir_bin() {
  Cmd cmd = {0};
  CMD_APPEND(&cmd, "mkdir -p bin/");
  return cmd;
}

Cmd build_raylib() {
  Cmd cmd = {0};
  CMD_APPEND(&cmd, "cd lib/raylib/src && make PLATFORM=PLATFORM_DESKTOP");
  return cmd;
}

int main(int argc, char **argv) {
  yeb_bootstrap();
  Options opts = parse_argv(argc, argv);
  is_release = opts_get(opts, "--release").exists;
  is_gui = opts_get(opts, "--gui").exists;
  execute(mkdir_bin());
  execute(build_main());
  return 0;
}
