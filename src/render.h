#pragma once

#include "common.h"
#include "linear_alg.h"

/// It's called `Camera_` because Raylib also has a `Camera_`.
/// For now camera always look in negative X direction.
/// But we still need an x value (could be any finite value) in the position vector for calculating the depth buffer.
typedef struct camera {
  /// Position of the camera.
  Vec3 pos;
  /// Min X of the near clipping plane.
  f32 min_x;
  /// Min Y of the near clipping plane.
  f32 min_y;
  /// Max X of the near clipping plane.
  f32 max_x;
  /// Max Y of the near clipping plane.
  f32 max_y;
  f32 fov;
  f32 aspect_ratio;
  f32 near_clipping_dist;
  f32 far_clipping_dist;
} Camera_;

/// SAFETY: Only use new_renderer to construct this.
typedef struct renderer {
  usize width;
  usize height;
  /// For converting between camera coords and pixel coords.
  f32 x_ratio;
  /// For converting between camera coords and pixel coords.
  f32 y_ratio;
  /// LEN: width * height.
  f32 *depth_buffer;
  Camera_ cam;
  Vec3 light;
  void *draw_pixel_callback_cx;
} Renderer;

Renderer new_renderer(usize width, usize height, Camera_ cam, Vec3 light);

void free_renderer(Renderer renderer);

void check_object_indices(usize vertices_len, usize *indices, usize indices_len);

usize cam_to_screen_x(const Renderer *renderer, f32 x);

/// Note that ordering of y is reversed!
usize cam_to_screen_y(const Renderer *renderer, f32 y);

f32 screen_to_cam_x(const Renderer *renderer, usize x);

f32 screen_to_cam_y(const Renderer *renderer, usize y);

void renderer_clear_frame(Renderer *renderer);

Vec3 transform(Mat4x4 m, Vec3 v);

typedef void(draw_pixel_callback_t)(void *cx, usize width, usize height, usize x, usize y, f32 z, u8 light_level);

/// Generally you wouldn't want to call this function yourself, instead define a `draw_pixel_callback` function, and do
/// `DEF_DRAW_FUNCTIONS(prefix_, _affix, my_draw_pixel_callback`. See `DEF_DRAW_FUNCTIONS` for more information.
void draw_triangle(Renderer *renderer, Vec3 p0, Vec3 p1, Vec3 p2, Mat4x4 m, draw_pixel_callback_t draw_pixel_callback);

typedef void(draw_triangle_callback_t)(Renderer *renderer, Vec3 p0, Vec3 p1, Vec3 p2, Mat4x4 m);

/// Generally you wouldn't want to call this function yourself, instead define a `draw_pixel_callback` function, and do
/// `DEF_DRAW_FUNCTIONS(prefix_, _affix, my_draw_pixel_callback)`. See `DEF_DRAW_FUNCTIONS` for more information.
void draw_object(Renderer *renderer,
                 const Vec3 *vertices,
                 const usize *indices,
                 usize indices_len,
                 Mat4x4 m,
                 draw_triangle_callback_t draw_triangle);

/// Generally you wouldn't want to call this function yourself, instead define a `draw_pixel_callback` function, and do
/// `DEF_DRAW_FUNCTIONS(prefix_, _affix, my_draw_pixel_callback)`. See `DEF_DRAW_FUNCTIONS` for more information.
void draw_object_indexless(Renderer *renderer,
                           const Vec3 *vertices,
                           usize vertices_len,
                           Mat4x4 m,
                           draw_triangle_callback_t draw_triangle);

/// This macro defines `draw_triangle_xxx`, `draw_object_xxx`, `draw_object_indexless_xxx` function in its header form.
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
#define DEF_DRAW_FUNCTIONS_HEADER(PREFIX, AFFIX, DRAW_PIXEL_CALLBACK)                                                  \
  void PREFIX##draw_triangle##AFFIX(Renderer *renderer, Vec3 p0, Vec3 p1, Vec3 p2, Mat4x4 m);                          \
  void PREFIX##draw_object##AFFIX(                                                                                     \
      Renderer *renderer, const Vec3 *vertices, const usize *indices, usize indices_len, Mat4x4 m);                    \
  void PREFIX##draw_object_indexless##AFFIX(Renderer *renderer, const Vec3 *vertices, usize vertices_len, Mat4x4 m);

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
  [[gnu::flatten]] void PREFIX##draw_triangle##AFFIX(Renderer *renderer, Vec3 p0, Vec3 p1, Vec3 p2, Mat4x4 m) {        \
    draw_triangle(renderer, p0, p1, p2, m, DRAW_PIXEL_CALLBACK);                                                       \
  }                                                                                                                    \
  [[gnu::flatten]] void PREFIX##draw_object##AFFIX(                                                                    \
      Renderer *renderer, const Vec3 *vertices, const usize *indices, usize indices_len, Mat4x4 m) {                   \
    draw_object(renderer, vertices, indices, indices_len, m, PREFIX##draw_triangle##AFFIX);                            \
  }                                                                                                                    \
  [[gnu::flatten]] void PREFIX##draw_object_indexless##AFFIX(                                                          \
      Renderer *renderer, const Vec3 *vertices, usize vertices_len, Mat4x4 m) {                                        \
    draw_object_indexless(renderer, vertices, vertices_len, m, PREFIX##draw_triangle##AFFIX);                          \
  }
