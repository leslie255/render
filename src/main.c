#include "teapot.h"
#include "common.h"
#include "mat.h"

#ifdef USE_RAYLIB
#include <sys/time.h>
#include <raylib.h>
#endif

#if defined(_WIN32) || defined(WIN32)
#include "windows.h"
#else
#include "unistd.h"
#endif

static inline usize maxzu(usize a, usize b) {
  return (a > b) ? a : b;
}

static inline usize minzu(usize a, usize b) {
  return (a < b) ? a : b;
}

static inline f32 minf(f32 a, f32 b) {
  return (a < b) ? a : b;
}

static inline f32 maxf(f32 a, f32 b) {
  return (a > b) ? a : b;
}

/// Min value between 3 floats.
static inline f32 minf_x3(f32 a, f32 b, f32 c) {
  f32 min = a;
  if (b < min)
    min = b;
  if (c < min)
    min = c;
  return min;
}

/// Max value between 3 floats.
static inline f32 maxf_x3(f32 a, f32 b, f32 c) {
  f32 max = a;
  if (b > max)
    max = b;
  if (c > max)
    max = c;
  return max;
}

/// For now camera always look in negative X direction.
/// But we still need an x value (could be any finite value) in the position vector for calculating the depth buffer.
typedef struct camera {
  Vec3 pos;
  f32 min_x;
  f32 min_y;
  f32 max_x;
  f32 max_y;
} Camera_;

/// Because raylib also has a `Camera` typedef.
#define Camera Camera_

/// Maps a point from world coord to camera coord.
Vec3 project_point(Camera cam, Vec3 p) {
  return (Vec3){p.get[1], p.get[2], cam.pos.get[0] - p.get[0]};
}

/// Helper function used in `is_in_triangle`.
f32 sign(f32 x, f32 y, Vec3 p1, Vec3 p2) {
  return (x - p2.get[0]) * (p1.get[1] - p2.get[1]) - (p1.get[0] - p2.get[0]) * (y - p2.get[1]);
}

/// The Z of p0, p1, p2 is ignored.
bool is_in_triangle(Vec3 p0, Vec3 p1, Vec3 p2, f32 x, f32 y) {
  f32 d0 = sign(x, y, p0, p1);
  f32 d1 = sign(x, y, p1, p2);
  f32 d2 = sign(x, y, p2, p0);
  bool has_neg = (d0 < 0) || (d1 < 0) || (d2 < 0);
  bool has_pos = (d0 > 0) || (d1 > 0) || (d2 > 0);
  return !(has_neg && has_pos);
}

/// From a projected triangle (calculated by `project_point`), calculate depth
/// on (x, y). If outside triangle returns infinity.
f32 triangular_interpolate_z(Vec3 p0, Vec3 p1, Vec3 p2, f32 x, f32 y) {
  if (!is_in_triangle(p0, p1, p2, x, y))
    return INFINITY;

  // reference: https://codeplea.com/triangular-interpolation

  // Weights.
  // w0 = ((y1 - y2)(px - x2) + (x2 - x1)(py - y2)) /
  //      ((y1 - y2)(x0 - x2) + (x2 - x1)(y0 - y2))
  // w1 = ((y2 - y1)(px - x2) + (x0 - x2)(py - y2)) /
  //      ((y1 - y2)(x0 - x2) + (x2 - x1)(y0 - y2))
  // w2 = 1 - w0 - w1
  // ... where:
  //    x0 ~ x2 => p{0~2}.get[0]
  //    y0 ~ y2 => p{0~2}.get[1]
  //    p{x|y}  => {x|y}

  f32 w0 = ((p1.get[1] - p2.get[1]) * (x - p2.get[0]) + (p2.get[0] - p1.get[0]) * (y - p2.get[1])) /
           ((p1.get[1] - p2.get[1]) * (p0.get[0] - p2.get[0]) + (p2.get[0] - p1.get[0]) * (p0.get[1] - p2.get[1]));
  f32 w1 = ((p2.get[1] - p0.get[1]) * (x - p2.get[0]) + (p0.get[0] - p2.get[0]) * (y - p2.get[1])) /
           ((p1.get[1] - p2.get[1]) * (p0.get[0] - p2.get[0]) + (p2.get[0] - p1.get[0]) * (p0.get[1] - p2.get[1]));
  f32 w2 = 1 - w0 - w1;

  // 1/z is linear to (x, y), z is not.
  f32 z0 = 1 / p0.get[2];
  f32 z1 = 1 / p1.get[2];

  f32 z2 = 1 / p2.get[2];
  f32 z_ = w0 * z0 + w1 * z1 + w2 * z2;

  return 1 / z_;
}

/// Normal vector of a triangle.
Vec3 triangle_normal(Vec3 p0, Vec3 p1, Vec3 p2) {
  return cross3(sub3(p2, p0), sub3(p1, p0));
}

/// The light level of a surface.
u8 surface_light_level(Vec3 light, Vec3 normal, u8 floor) {
  //  angle = arccos ( (a . b) / (|a| |b|) )
  f32 angle = acosf(dot3(light, normal) / (abs3(normal) * abs3(light)));
  if (angle > to_rad(90))
    return floor;
  f32 light_level = (1 - (angle / to_rad(90))) * 255;
  f32 floor_ = (f32)floor;
  u8 x = (u8)(light_level * (255 - floor_) / 255 + floor_);
  return x > floor ? x : floor;
}

typedef struct frame {
  usize width;
  usize height;
  f32 *depth_buffer;
  /// Greyscale, 0 is black, 255 is white.
  u8 *pixels;
} Frame;

/// SAFETY: Only use new_renderer to construct this.
typedef struct renderer {
  Vec3 *projs_buffer;
  usize width;
  usize height;
  /// For converting between camera coords and pixel coords.
  f32 x_ratio;
  /// For converting between camera coords and pixel coords.
  f32 y_ratio;
  /// LEN: width * height.
  f32 *depth_buffer;
  Camera cam;
  Vec3 light;
  void *draw_pixel_callback_cx;
} Renderer;

Renderer new_renderer(usize width, usize height, Camera cam, Vec3 light) {
  ASSERT(cam.max_x > cam.min_x);
  ASSERT(cam.max_y > cam.min_y);
  return (Renderer){
      .depth_buffer = xalloc(f32, width * height),
      .width = width,
      .height = height,
      .x_ratio = (cam.max_x - cam.min_x) / (f32)width,
      .y_ratio = (cam.max_y - cam.min_x) / (f32)height,
      .cam = cam,
      .light = light,
  };
}

void check_object_indices(usize vertices_len, usize *indices, usize indices_len) {
  ASSERT(indices_len % 3 == 0);
  for (usize i = 0; i < indices_len; ++i) {
    usize index = indices[i];
    ASSERT(index < vertices_len);
  }
}

void free_renderer(Renderer renderer) {
  xfree(renderer.projs_buffer);
}

usize cam_to_screen_x(const Renderer *renderer, f32 x) {
  return (usize)((x - renderer->cam.min_x) / renderer->x_ratio);
}

/// Note that ordering of y is reversed!
usize cam_to_screen_y(const Renderer *renderer, f32 y) {
  f32 dy = renderer->cam.max_y - renderer->cam.min_y;
  return (usize)((dy - (y - renderer->cam.min_y)) / renderer->y_ratio);
}

f32 screen_to_cam_x(const Renderer *renderer, usize x) {
  return (f32)x * renderer->x_ratio + renderer->cam.min_x;
}

f32 screen_to_cam_y(const Renderer *renderer, usize y) {
  f32 dy = renderer->cam.max_y - renderer->cam.min_y;
  return dy - (f32)y * renderer->y_ratio + renderer->cam.min_y;
}

void renderer_clear_frame(Renderer *renderer) {
  for (usize i = 0; i < renderer->width * renderer->height; ++i) {
    renderer->depth_buffer[i] = INFINITY;
  }
}

Vec3 transform(Mat4x4 m, Vec3 v) {
  return vec4to3(mul4x4_4(m, vec3to4(v)));
}

typedef void(draw_pixel_callback_t)(void *cx, usize width, usize height, usize x, usize y, f32 depth, u8 light_level);

/// Generally you wouldn't want to call this function yourself, instead define a `draw_pixel_callback` function, and do
/// `DEF_DRAW_FUNCTIONS(_affix, my_draw_pixel_callback`. See `DEF_DRAW_FUNCTIONS` for more information.
void draw_triangle(Renderer *renderer, Vec3 p0, Vec3 p1, Vec3 p2, Mat4x4 m, draw_pixel_callback_t draw_pixel_callback) {
  Vec3 p0_ = transform(m, p0);
  Vec3 p1_ = transform(m, p1);
  Vec3 p2_ = transform(m, p2);

  // Project the triangle onto the camera plane.
  Vec3 p0_proj = project_point(renderer->cam, p0_);
  Vec3 p1_proj = project_point(renderer->cam, p1_);
  Vec3 p2_proj = project_point(renderer->cam, p2_);

  // Calculate the frame that the triangle occupies so we can skip sampling pixels outside of this frame.
  Camera cam = renderer->cam;
  // Camera coords.
  f32 min_x_cam = maxf(minf_x3(p0_proj.get[0], p1_proj.get[0], p2_proj.get[0]), cam.min_x);
  f32 max_x_cam = minf(maxf_x3(p0_proj.get[0], p1_proj.get[0], p2_proj.get[0]), cam.max_x);
  f32 min_y_cam = maxf(minf_x3(p0_proj.get[1], p1_proj.get[1], p2_proj.get[1]), cam.min_y);
  f32 max_y_cam = minf(maxf_x3(p0_proj.get[1], p1_proj.get[1], p2_proj.get[1]), cam.max_y);
  // Pixel coords.
  // Adds some extra pixels to the frame to compensate for floating point inaccuracies.
  usize min_x = maxzu(cam_to_screen_x(renderer, min_x_cam) - 1, 0);
  usize max_x = minzu(cam_to_screen_x(renderer, max_x_cam) + 1, renderer->width);
  usize min_y = maxzu(cam_to_screen_y(renderer, max_y_cam) - 1, 0);
  usize max_y = minzu(cam_to_screen_y(renderer, min_y_cam) + 1, renderer->height);

  // The light level of this surface.
  u8 light_level = surface_light_level(renderer->light, triangle_normal(p0_, p1_, p2_), 20);

  // Sample and draw the pixels.
  for (usize y = min_y; y < max_y; ++y) {
    for (usize x = min_x; x < max_x; ++x) {
      f32 cam_x = screen_to_cam_x(renderer, x);
      f32 cam_y = screen_to_cam_y(renderer, y);
      f32 depth = triangular_interpolate_z(p0_proj, p1_proj, p2_proj, cam_x, cam_y);
      f32 *prev_depth = &renderer->depth_buffer[y * renderer->width + x];
      if (depth < *prev_depth) {
        *prev_depth = depth;
        // u8 light_level = (u8)(255 - (depth - 90) / 20 * 255);
        if (draw_pixel_callback != NULL)
          draw_pixel_callback(
              renderer->draw_pixel_callback_cx, renderer->width, renderer->height, x, y, depth, light_level);
      }
    }
  }
}

typedef void(draw_triangle_callback_t)(Renderer *renderer, Vec3 p0, Vec3 p1, Vec3 p2, Mat4x4 m);

/// Generally you wouldn't want to call this function yourself, instead define a `draw_pixel_callback` function, and do
/// `DEF_DRAW_FUNCTIONS(_affix, my_draw_pixel_callback`. See `DEF_DRAW_FUNCTIONS` for more information.
void draw_object(Renderer *renderer,
                 const Vec3 *vertices,
                 const usize *indices,
                 usize indices_len,
                 Mat4x4 m,
                 draw_triangle_callback_t draw_triangle) {
  for (usize i = 0; i < indices_len; i += 3) {
    Vec3 p0 = vertices[indices[i + 0]];
    Vec3 p1 = vertices[indices[i + 1]];
    Vec3 p2 = vertices[indices[i + 2]];
    draw_triangle(renderer, p0, p1, p2, m);
  }
}

/// Generally you wouldn't want to call this function yourself, instead define a `draw_pixel_callback` function, and do
/// `DEF_DRAW_FUNCTIONS(_affix, my_draw_pixel_callback`. See `DEF_DRAW_FUNCTIONS` for more information.
void draw_object_indexless(Renderer *renderer,
                           const Vec3 *vertices,
                           usize vertices_len,
                           Mat4x4 m,
                           draw_triangle_callback_t draw_triangle) {
  for (usize i = 0; i < vertices_len; i += 3) {
    Vec3 p0 = vertices[i + 0];
    Vec3 p1 = vertices[i + 1];
    Vec3 p2 = vertices[i + 2];
    draw_triangle(renderer, p0, p1, p2, m);
  }
}

/// This macro defines `draw_triangle_xxx`, `draw_object_xxx`, `draw_object_indexless_xxx` function.
/// These functions are monomorphosized versions of `draw_triangle`, `draw_object`, `draw_object_indexless`, which are
/// generic over a `draw_pixel_callback` function.
/// On GCC and Clang, the monomorphosation process should have zero overhead.
///
/// Example:
///
/// ```
/// void my_draw_pixel_callback(void *cx, usize width, usize height, usize x, usize y, f32 depth, u8 light_level) {
///   // ...
/// }
///
/// DEF_DRAW_FUNCTIONS(my_, _function, my_draw_pixel_callback);
/// ```
///
/// The above would define `my_draw_triangle_function`, `my_draw_object_function`, `my_draw_object_indexless_function`.
#define DEF_DRAW_FUNCTIONS(PREFIX, AFFIX, DRAW_PIXEL_CALLBACK)                                                         \
  __attribute__((flatten)) void PREFIX##draw_triangle##AFFIX(                                                          \
      Renderer *renderer, Vec3 p0, Vec3 p1, Vec3 p2, Mat4x4 m) {                                                       \
    draw_triangle(renderer, p0, p1, p2, m, DRAW_PIXEL_CALLBACK);                                                       \
  }                                                                                                                    \
  __attribute__((flatten)) void PREFIX##draw_object##AFFIX(                                                            \
      Renderer *renderer, const Vec3 *vertices, const usize *indices, usize indices_len, Mat4x4 m) {                   \
    draw_object(renderer, vertices, indices, indices_len, m, PREFIX##draw_triangle##AFFIX);                            \
  }                                                                                                                    \
  __attribute__((flatten)) void PREFIX##draw_object_indexless##AFFIX(                                                  \
      Renderer *renderer, const Vec3 *vertices, usize vertices_len, Mat4x4 m) {                                        \
    draw_object_indexless(renderer, vertices, vertices_len, m, PREFIX##draw_triangle##AFFIX);                          \
  }

void draw_pixel_callback_gui(void *cx, usize width, usize height, usize x, usize y, f32 depth, u8 light_level) {
  u8 *frame_buffer = cx;
  frame_buffer[y * width + x] = light_level;
}

DEF_DRAW_FUNCTIONS(, _gui, draw_pixel_callback_gui);

char char_for_light_level(u8 light_level) {
  static const char grayscale[] = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'.";
  usize i = (usize)light_level / (256 / sizeof(grayscale) - 1);
  if (i > sizeof(grayscale) - 2)
    i = sizeof(grayscale) - 2;
  return grayscale[i];
}

void draw_pixel_callback_tui(void *cx, usize width, usize height, usize x, usize y, f32 depth, u8 light_level) {
  char *frame_buffer = cx;
  char c = char_for_light_level(light_level);
  usize i = (y * (width * 2 + 1)) + x * 2;
  frame_buffer[i + 0] = c;
  frame_buffer[i + 1] = c;
}

DEF_DRAW_FUNCTIONS(, _tui, draw_pixel_callback_tui);

[[maybe_unused]]
static const Vec3 cube_vertices[] = {
    // clang-format off
    {-5.0f, -5.0f, -5.0f},
    { 5.0f, -5.0f, -5.0f},
    { 5.0f,  5.0f, -5.0f},
    {-5.0f,  5.0f, -5.0f},
    {-5.0f, -5.0f,  5.0f},
    { 5.0f, -5.0f,  5.0f},
    { 5.0f,  5.0f,  5.0f},
    {-5.0f,  5.0f,  5.0f},
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

i32 main() {
  Vec3 light = {-10, 5, -1};
  Camera cam = {
      .pos = {100, 0, 0},
      .min_x = -2.0f,
      .min_y = -2.0f,
      .max_x = +2.0f,
      .max_y = +2.0f,
  };

#ifdef USE_RAYLIB
  const f32 fps = 60;
  const usize width = 800;
  const usize height = 800;
#else
  const f32 fps = 24;
  const usize width = 100;
  const usize height = 100;
#endif

  const useconds_t sleep_duration = (useconds_t)(1e6 / fps);

  Renderer renderer = new_renderer(width, height, cam, light);

  Mat4x4 m = mat4x4_id;

  m = mul4x4(translate3d((Vec3){0, 0, -0.7f}), m);
  m = mul4x4(mat3x3to4x4(rotate3d_x(to_rad(20))), m);

  Mat4x4 rotate = mat3x3to4x4(rotate3d_z(to_rad(90.0f) / fps));

#ifndef USE_RAYLIB
  usize frame_buffer_size = (width * 2 + 1) * height;
  char *frame_buffer = xalloc(char, frame_buffer_size);
  renderer.draw_pixel_callback_cx = frame_buffer;

  for (;;) {
    m = mul4x4(rotate, m);
    renderer_clear_frame(&renderer);
    memset(frame_buffer, ' ', frame_buffer_size);
    draw_object_indexless_tui(&renderer, teapot, ARR_LEN(teapot), m);
    for (usize y = 0; y < height; ++y) {
      frame_buffer[(y * (width * 2 + 1)) + width * 2] = '\n';
    }
    fwrite(frame_buffer, 1, frame_buffer_size, stdout);
    fflush(stdout);
    usleep(sleep_duration);
  }
#else
  u8 *frame_buffer = xalloc(u8, width * height);
  renderer.draw_pixel_callback_cx = frame_buffer;
  SetTraceLogLevel(LOG_ERROR);
  InitWindow((i32)width, (i32)height, "Render");
  while (!WindowShouldClose()) {
    m = mul4x4(rotate, m);
    renderer_clear_frame(&renderer);
    memset(frame_buffer, 0, width * height);
    draw_object_indexless_gui(&renderer, teapot, ARR_LEN(teapot), m);
    Texture texture = LoadTextureFromImage((Image){
        .width = (i32)width,
        .height = (i32)height,
        .data = frame_buffer,
        .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE,
        .mipmaps = 1,
    });
    BeginDrawing();
    DrawTexture(texture, 0, 0, WHITE);
    EndDrawing();
    usleep(sleep_duration);
  }
#endif

  return 0;
}
