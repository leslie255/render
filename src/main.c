#include "common.h"
#include "debug_utils.h"
#include "mat.h"

#ifdef USE_RAYLIB
#include <raylib.h>
#endif

#include <unistd.h>

/// For now camera always look in negative X direction.
typedef struct camera {
  Vec3 pos;
  f32 min_x;
  f32 min_y;
  f32 dx;
  f32 dy;
} Camera_;

/// Project a point to the camera.
/// Returns a Vec3, in which the x and y values are the on-screen position, z is the depth.
Vec3 project_point(Camera_ cam, Vec3 p) {
  return (Vec3){p.get[1], p.get[2], cam.pos.get[0] - p.get[0]};
}

/// Normal vector of a triangle.
Vec3 triangle_normal(Vec3 p0, Vec3 p1, Vec3 p2) {
  return cross3(sub3(p2, p0), sub3(p1, p0));
}

f32 absf(f32 x) {
  return (x >= 0 ? x : -x);
}

f32 area(f32 x1, f32 y1, f32 x2, f32 y2, f32 x3, f32 y3) {
  return absf(x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2)) / 2.0f;
}

/// The Z of p0, p1, p2 is ignored.
bool is_in_triangle(Vec3 p0, Vec3 p1, Vec3 p2, f32 x, f32 y) {
  f32 area0 = area(p0.get[0], p0.get[1], p1.get[0], p1.get[1], p2.get[0], p2.get[1]);
  f32 area1 = area(x, y, p1.get[0], p1.get[1], p2.get[0], p2.get[1]);
  f32 area2 = area(p0.get[0], p0.get[1], x, y, p2.get[0], p2.get[1]);
  f32 area3 = area(p0.get[0], p0.get[1], p1.get[0], p1.get[1], x, y);
  f32 d = area0 - (area1 + area2 + area3);
  return absf(d) < 1e-3;
}

/// From a projected triangle (calculated by `project_point`), calculate depth on (x, y).
/// If outside triangle returns infinity.
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

  f32 w0 = ((p1.get[1] - p2.get[1]) * (x         - p2.get[0]) + (p2.get[0] - p1.get[0]) * (y         - p2.get[1])) /
           ((p1.get[1] - p2.get[1]) * (p0.get[0] - p2.get[0]) + (p2.get[0] - p1.get[0]) * (p0.get[1] - p2.get[1]));
  f32 w1 = ((p2.get[1] - p0.get[1]) * (x         - p2.get[0]) + (p0.get[0] - p2.get[0]) * (y         - p2.get[1])) /
           ((p1.get[1] - p2.get[1]) * (p0.get[0] - p2.get[0]) + (p2.get[0] - p1.get[0]) * (p0.get[1] - p2.get[1]));
  f32 w2 = 1 - w0 - w1;

  // 1/z is linear to (x, y), z is not.
  f32 z0 = 1 / p0.get[2];
  f32 z1 = 1 / p1.get[2];

  f32 z2 = 1 / p2.get[2];
  f32 z_ = w0 * z0 + w1 * z1 + w2 * z2;

  return 1 / z_;
}

/// The light level of a surface.
u8 surface_light_level(Vec3 light, Vec3 normal) {
  // return 255;
  //  angle = arccos ( (a . b) / (|a| |b|) )
  f32 angle = acosf(dot3(light, normal) / (abs3(normal) * abs3(light)));
  if (angle > to_rad(90))
    angle = to_rad(180) - angle;
  f32 light_level = 1 - (angle / to_rad(90));
  // light_level = sqrtf(light_level); // make it brigher
  return (u8)(light_level * 255);
}

typedef struct frame {
  usize width;
  usize height;
  f32 *depth_buffer;
  /// Greyscale, 0 is black, 255 is white.
  u8 *pixels;
} Frame;

char char_for_light_level(u8 light_level) {
  static const char grayscale[] = ".-+#@";
  usize i = (usize)light_level / (256 / sizeof(grayscale) - 1);
  if (i > sizeof(grayscale) - 2)
    i = sizeof(grayscale) - 2;
  return grayscale[i];
}

typedef struct scene {
  Vec3 *vertices;
  usize vertices_len;
  usize *indices;
  usize indices_len;
  Camera_ cam;
  Vec3 light;
} Scene;

void apply_transform(Mat3x3 m, Vec3 *vertices, usize vertices_len) {
  for (usize i = 0; i < vertices_len; ++i) {
    Vec3 *p = &vertices[i];
    *p = mul3x3_3(m, *p);
  }
}

typedef struct renderer {
  Vec3 *projs_buffer;
  u8 *light_buffer;
  Scene scene;
  void *draw_pixel_callback_cx;
} Renderer;

Renderer new_renderer(const Scene scene) {
  ASSERT(scene.indices_len % 3 == 0);
  for (usize i = 0; i < scene.indices_len; i++) {
    ASSERT(scene.indices[i] < scene.vertices_len);
  }
  Vec3 *projs_buffer = xalloc(Vec3, scene.vertices_len);
  u8 *light_buffer = xalloc(u8, scene.indices_len / 3);
  return (Renderer){
      .projs_buffer = projs_buffer,
      .light_buffer = light_buffer,
      .scene = scene,
  };
}

void free_renderer(Renderer renderer) {
  xfree(renderer.projs_buffer);
}

typedef void(draw_pixel_callback_t)(void *cx, usize width, usize height, usize x, usize y, f32 depth, u8 light_level);
typedef void(draw_eol_callback_t)(void *cx, usize width, usize height, usize y);

/// Frame buffer must be of size `(width * 2 + 1) * height`.
__attribute__((__always_inline__)) void render(Renderer *renderer, usize width, usize height,
                                               draw_pixel_callback_t draw_pixel_callback,
                                               draw_eol_callback_t draw_eol_callback) {
  // Projections.
  for (usize i = 0; i < renderer->scene.vertices_len; ++i) {
    Vec3 p = project_point(renderer->scene.cam, renderer->scene.vertices[i]);
    renderer->projs_buffer[i] = p;
  }
  // Lights.
  for (usize i_ = 0; i_ < renderer->scene.indices_len; i_ += 3) {
    usize i = renderer->scene.indices[i_ + 0];
    usize j = renderer->scene.indices[i_ + 1];
    usize k = renderer->scene.indices[i_ + 2];
    Vec3 p0 = renderer->scene.vertices[i];
    Vec3 p1 = renderer->scene.vertices[j];
    Vec3 p2 = renderer->scene.vertices[k];
    Vec3 normal = triangle_normal(p0, p1, p2);
    u8 light_level = surface_light_level(renderer->scene.light, normal);
    renderer->light_buffer[i_ / 3] = light_level;
  }

  // Draw frame.
  for (usize pixel_y = 0; pixel_y < width; ++pixel_y) {
    f32 camera_y =
        renderer->scene.cam.min_y + renderer->scene.cam.dy - renderer->scene.cam.dy * ((f32)pixel_y / (f32)height);
    for (usize pixel_x = 0; pixel_x < height; ++pixel_x) {
      f32 camera_x = renderer->scene.cam.min_x + renderer->scene.cam.dx * ((f32)pixel_x / (f32)width);
      u8 light_level = 0;
      f32 min_depth = INFINITY;
      for (usize i_ = 0; i_ < renderer->scene.indices_len; i_ += 3) {
        usize i = renderer->scene.indices[i_ + 0];
        usize j = renderer->scene.indices[i_ + 1];
        usize k = renderer->scene.indices[i_ + 2];
        Vec3 p0_proj = renderer->projs_buffer[i];
        Vec3 p1_proj = renderer->projs_buffer[j];
        Vec3 p2_proj = renderer->projs_buffer[k];
        f32 depth = triangular_interpolate_z(p0_proj, p1_proj, p2_proj, camera_x, camera_y);
        if (depth < min_depth) {
          min_depth = depth;
          light_level = renderer->light_buffer[i_ / 3];
        }
      }
      if (draw_pixel_callback != NULL)
        draw_pixel_callback(renderer->draw_pixel_callback_cx, width, height, pixel_x, pixel_y, min_depth, light_level);
    }
    if (draw_eol_callback != NULL)
      draw_eol_callback(renderer->draw_pixel_callback_cx, width, height, pixel_y);
  }
}

__attribute__((__always_inline__)) void draw_pixel_ascii(void *cx, usize width, usize height, usize x, usize y,
                                                         f32 depth, u8 light_level) {
  char *frame_buffer = cx;
  char c = (depth == INFINITY) ? ' ' : char_for_light_level(light_level);
  usize i = (y * (width * 2 + 1)) + x * 2;
  frame_buffer[i + 0] = c;
  frame_buffer[i + 1] = c;
}

__attribute__((__always_inline__)) void draw_eol_ascii(void *cx, usize width, usize height, usize y) {
  char *frame_buffer = cx;
  frame_buffer[(y * (width * 2 + 1)) + width * 2] = '\n';
}

void render_ascii(Renderer *renderer, usize width, usize height, char *frame_buffer) {
  renderer->draw_pixel_callback_cx = frame_buffer;
  render(renderer, width, height, draw_pixel_ascii, draw_eol_ascii);
}

__attribute__((__always_inline__)) void draw_pixel_pixel(void *cx, usize width, usize height, usize x, usize y,
                                                         f32 depth, u8 light_level) {
  u8 *frame_buffer = cx;
  frame_buffer[y * width + x] = light_level;
}

/// Frame buffer must be of size `width*height`.
void render_pixels(Renderer *renderer, usize width, usize height, u8 *frame_buffer) {
  renderer->draw_pixel_callback_cx = frame_buffer;
  render(renderer, width, height, draw_pixel_pixel, NULL);
}

i32 main() {
  Vec3 vertices[] = {
      {-5, -5, -5}, //
      {-5, -5, 5},  //
      {-5, 5, -5},  //
      {-5, 5, 5},   //
      {5, -5, -5},  //
      {5, -5, 5},   //
      {5, 5, -5},   //
      {5, 5, 5},    //
  };
  usize indices[] = {
      0, 1, 2, // 0
      1, 2, 3, // 1
      4, 2, 6, // 2
      0, 4, 2, // 3
      4, 7, 6, // 4
      4, 5, 7, // 5
      4, 5, 0, // 6
      0, 1, 5, // 7
      1, 3, 5, // 8
      7, 3, 5, // 9
      6, 3, 2, // 10
      6, 3, 7, // 11
  };

  Vec3 light = {0, -1, 0};
  Camera_ cam = {
      .pos = {100, 0, 0},
      .min_x = -10,
      .min_y = -10,
      .dx = 20,
      .dy = 20,
  };

  Renderer renderer = new_renderer((Scene){
      .vertices = vertices,
      .vertices_len = ARR_LEN(vertices),
      .indices = indices,
      .indices_len = ARR_LEN(indices),
      .cam = cam,
      .light = light,
  });

  apply_transform(rotate3d_y(to_rad(30)), renderer.scene.vertices, renderer.scene.vertices_len);
  apply_transform(rotate3d_x(to_rad(20)), renderer.scene.vertices, renderer.scene.vertices_len);

  const f32 fps = 60;

  Mat3x3 m = rotate3d_z(to_rad(180.0f / fps));

#ifndef USE_RAYLIB
    usize width = 30;
    usize height = 30;
    usize frame_buffer_size = (width * 2 + 1) * height;
    char *frame_buffer = xalloc(char, frame_buffer_size);

    for (;;) {
      apply_transform(m, renderer.scene.vertices, renderer.scene.vertices_len);
      render_ascii(&renderer, width, height, frame_buffer);
      fwrite(frame_buffer, frame_buffer_size, 1, stdout);
      usleep((useconds_t)(1e6 / fps));
    }
#else
    usize width = 800;
    usize height = 800;
    u8 *frame_buffer = xalloc(u8, width * height);
    SetTraceLogLevel(LOG_ERROR);
    InitWindow((i32)width, (i32)height, "Render");
    while (!WindowShouldClose()) {
      apply_transform(m, renderer.scene.vertices, renderer.scene.vertices_len);
      render_pixels(&renderer, width, height, frame_buffer);
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
      usleep((useconds_t)(1e6 / fps));
    }
#endif

  return 0;
}
