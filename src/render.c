#include "render.h"

#include "math_helpers.h"

Renderer new_renderer(usize width, usize height, Camera_ cam, Vec3 light) {
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

void free_renderer(Renderer renderer) {
  xfree(renderer.depth_buffer);
}

void check_object_indices(usize vertices_len, usize *indices, usize indices_len) {
  ASSERT(indices_len % 3 == 0);
  for (usize i = 0; i < indices_len; ++i) {
    usize index = indices[i];
    ASSERT(index < vertices_len);
  }
}

usize cam_to_screen_x(const Renderer *renderer, f32 x) {
  return (usize)((x - renderer->cam.min_x) / renderer->x_ratio);
}

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

static inline Mat4x4 view_matrix(Vec3 cam_pos) {
  return (Mat4x4){{
      {0, 1.f, 0, -cam_pos.get[1]},
      {0, 0, 1.f, -cam_pos.get[2]},
      {-1.f, 0, 0, cam_pos.get[0]},
      {0, 0, 0, 1.f},
  }};
}

static inline Mat4x4 projection_matrix(f32 fov, f32 aspect_ratio, f32 near_clipping, f32 far_clipping) {
  return (Mat4x4){{
      {1.f / (aspect_ratio * tanf(fov / 2.f)), 0, 0, 0},
      {0, 1.f / (tanf(fov / 2.f)), 0, 0},
      {0, 0, fov / (fov - near_clipping), (-near_clipping * fov) / (fov - near_clipping)},
      {0, 0, -1.f, 0},
  }};
}

/// Maps a point from world coord to camera coord.
static inline Vec3 project_point(const Camera_ cam, Vec3 p) {
  Mat4x4 view_mat = view_matrix(cam.pos);
  Mat4x4 proj_mat = projection_matrix(cam.fov, cam.aspect_ratio, cam.near_clipping_dist, cam.far_clipping_dist);
  Vec3 result = p;
  result = transform(view_mat, result);
  result = transform(proj_mat, result);
  return result;
}

/// Helper function used in `is_in_triangle`.
static inline f32 sign(f32 x, f32 y, Vec3 p1, Vec3 p2) {
  return (x - p2.get[0]) * (p1.get[1] - p2.get[1]) - (p1.get[0] - p2.get[0]) * (y - p2.get[1]);
}

/// The Z of p0, p1, p2 is ignored.
static inline bool is_in_triangle(Vec3 p0, Vec3 p1, Vec3 p2, f32 x, f32 y) {
  f32 d0 = sign(x, y, p0, p1);
  f32 d1 = sign(x, y, p1, p2);
  f32 d2 = sign(x, y, p2, p0);
  bool has_neg = (d0 < 0) || (d1 < 0) || (d2 < 0);
  bool has_pos = (d0 > 0) || (d1 > 0) || (d2 > 0);
  return !(has_neg && has_pos);
}

/// From a projected triangle (calculated by `project_point`), calculate depth
/// on (x, y). If outside triangle returns infinity.
static inline f32 triangular_interpolate_z(Vec3 p0, Vec3 p1, Vec3 p2, f32 x, f32 y) {
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
static inline Vec3 triangle_normal(Vec3 p0, Vec3 p1, Vec3 p2) {
  return cross3(sub3(p2, p0), sub3(p1, p0));
}

/// The light level of a surface.
u8 surface_light_level(Vec3 light, Vec3 normal, u8 floor) {
  // angle = arccos ( (a . b) / (|a| |b|) )
  f32 angle = acosf(dot3(light, normal) / (abs3(normal) * abs3(light)));
  if (angle > to_rad(90))
    return floor;
  f32 light_level = (1 - (angle / to_rad(90))) * 255;
  f32 floor_ = (f32)floor;
  u8 x = (u8)(light_level * (255 - floor_) / 255 + floor_);
  return x > floor ? x : floor;
}

typedef void(draw_pixel_callback_t)(void *cx, usize width, usize height, usize x, usize y, f32 z, u8 light_level);

/// Generally you wouldn't want to call this function yourself, instead define a `draw_pixel_callback` function, and do
/// `DEF_DRAW_FUNCTIONS(prefix_, _affix, my_draw_pixel_callback`. See `DEF_DRAW_FUNCTIONS` for more information.
void draw_triangle(Renderer *renderer, Vec3 p0, Vec3 p1, Vec3 p2, Mat4x4 m, draw_pixel_callback_t draw_pixel_callback) {
  Vec3 p0_ = transform(m, p0);
  Vec3 p1_ = transform(m, p1);
  Vec3 p2_ = transform(m, p2);

  // The light level of this surface.
  u8 light_level = surface_light_level(renderer->light, triangle_normal(p0_, p1_, p2_), 20);

  // Project the triangle onto the camera plane.
  Vec3 p0_proj = project_point(renderer->cam, p0_);
  Vec3 p1_proj = project_point(renderer->cam, p1_);
  Vec3 p2_proj = project_point(renderer->cam, p2_);

  // Calculate the frame that the triangle occupies so we can skip sampling pixels outside of this frame.
  Camera_ cam = renderer->cam;
  // Camera coords.
  f32 min_x_cam = maxf(minf_x3(p0_proj.get[0], p1_proj.get[0], p2_proj.get[0]), cam.min_x);
  f32 max_x_cam = minf(maxf_x3(p0_proj.get[0], p1_proj.get[0], p2_proj.get[0]), cam.max_x);
  f32 min_y_cam = maxf(minf_x3(p0_proj.get[1], p1_proj.get[1], p2_proj.get[1]), cam.min_y);
  f32 max_y_cam = minf(maxf_x3(p0_proj.get[1], p1_proj.get[1], p2_proj.get[1]), cam.max_y);
  // Pixel coords.
  usize min_x = cam_to_screen_x(renderer, min_x_cam);
  usize max_x = cam_to_screen_x(renderer, max_x_cam);
  usize min_y = cam_to_screen_y(renderer, max_y_cam);
  usize max_y = cam_to_screen_y(renderer, min_y_cam);
  // Adds some extra pixels to the frame to compensate for floating point inaccuracies.
  min_x = saturating_subzu(min_x, 1);
  max_x = minzu(saturating_addzu(max_x, 1), renderer->width);
  min_y = saturating_subzu(min_y, 1);
  max_y = minzu(saturating_addzu(max_y, 1), renderer->height);

  // Sample and draw the pixels.
  for (usize y = min_y; y < max_y; ++y) {
    for (usize x = min_x; x < max_x; ++x) {
      f32 cam_x = screen_to_cam_x(renderer, x);
      f32 cam_y = screen_to_cam_y(renderer, y);
      f32 depth = triangular_interpolate_z(p0_proj, p1_proj, p2_proj, cam_x, cam_y);
      f32 *prev_depth = &renderer->depth_buffer[y * renderer->width + x];
      if (depth < *prev_depth) {
        *prev_depth = depth;
        if (draw_pixel_callback != NULL)
          draw_pixel_callback(
              renderer->draw_pixel_callback_cx, renderer->width, renderer->height, x, y, depth, light_level);
      }
    }
  }
}

typedef void(draw_triangle_callback_t)(Renderer *renderer, Vec3 p0, Vec3 p1, Vec3 p2, Mat4x4 m);

/// Generally you wouldn't want to call this function yourself, instead define a `draw_pixel_callback` function, and do
/// `DEF_DRAW_FUNCTIONS(prefix_, _affix, my_draw_pixel_callback)`. See `DEF_DRAW_FUNCTIONS` for more information.
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
/// `DEF_DRAW_FUNCTIONS(prefix_, _affix, my_draw_pixel_callback)`. See `DEF_DRAW_FUNCTIONS` for more information.
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
