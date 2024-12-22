#include <time.h>

#include "common.h"
#include "mat.h"
#include "teapot.h"
#include "render.h"
#include "gui.h"

#include <sys/time.h>
#include <raylib.h>

[[maybe_unused]]
static const Vec3 cube_vertices[] = {
    // clang-format off
    {{-5.0f, -5.0f, -5.0f}},
    {{ 5.0f, -5.0f, -5.0f}},
    {{ 5.0f,  5.0f, -5.0f}},
    {{-5.0f,  5.0f, -5.0f}},
    {{-5.0f, -5.0f,  5.0f}},
    {{ 5.0f, -5.0f,  5.0f}},
    {{ 5.0f,  5.0f,  5.0f}},
    {{-5.0f,  5.0f,  5.0f}},
    // clang-format on
};

[[maybe_unused]]
static const usize cube_indices[] = {
    0, 3, 2, //
    2, 1, 0, //
    4, 5, 6, //
    6, 7, 4, //
    7, 3, 0, //
    0, 4, 7, //
    1, 2, 6, //
    6, 5, 1, //
    0, 1, 5, //
    5, 4, 0, //
    2, 3, 7, //
    7, 6, 2, //
};

static inline u64 current_ms() {
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return (u64)(t.tv_sec) * 1000 + (u64)(t.tv_nsec / 1000000);
}

static inline Mat4x4 rotation_for_current_time() {
  u64 ms = current_ms();
  const u64 rotation_period_ms = 5 * 1000;
  // Convert current time to a fraction of the period
  f32 fraction_of_period = (ms % rotation_period_ms) / (f32)rotation_period_ms;
  // Convert fraction to radians (2 * PI radians in a full circle)
  f32 rad = fraction_of_period * 2.0f * (f32)M_PI;
  return mat3x3to4x4(rotate3d_z(rad));
}

i32 main() {

  const f32 fps = 60.f;
  const usize width = 800;
  const usize height = 800;

  Vec3 light = {{-10, 5, -1}};
  Camera_ cam = {
      .pos = {{10, 0, 0}},
      .min_x = -2.0f,
      .min_y = -2.0f,
      .max_x = +2.0f,
      .max_y = +2.0f,
      .fov = to_rad(90.f),
      .aspect_ratio = 1.f,
      .near_clipping_dist = 0.1f,
      .far_clipping_dist = 100.f,
  };
  Renderer renderer = new_renderer(width, height, cam, light);

  Mat4x4 base_transform = mat4x4_id;
  base_transform = mul4x4(translate3d((Vec3){{0, 0, -0.7f}}), base_transform);
  base_transform = mul4x4(mat3x3to4x4(rotate3d_x(to_rad(20))), base_transform);

  GuiPainter gui_painter = new_gui_painter(width, height, fps);
  renderer.draw_pixel_callback_cx = &gui_painter;
  gui_setup_window(&gui_painter);

  while (!WindowShouldClose()) {
    gui_handle_event(&gui_painter, &renderer);

    renderer_clear_frame(&renderer);
    gui_clear_frame(&gui_painter);

    // Render stuff.
    Mat4x4 transform = mul4x4(rotation_for_current_time(), base_transform);
    draw_object_indexless_gui(&renderer, teapot, ARR_LEN(teapot), transform);

    // Finish frame.
    gui_finish_frame(&gui_painter, &renderer);
  }

  return 0;
}
