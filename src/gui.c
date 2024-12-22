#include "gui.h"

#include <raylib.h>

GuiPainter new_gui_painter(usize width, usize height, f32 target_fps) {
  u8 *frame_buffer = xalloc(u8, width * height);
  return (GuiPainter){
      .shader_kind = SHADER_KIND_DEFAULT,
      .frame_buffer = frame_buffer,
      .width = width,
      .height = height,
      .debug_line_count = 0,
      .target_fps = target_fps,
  };
}

void free_gui_drawing_cx(GuiPainter cx) {
  xfree(cx.frame_buffer);
}

void gui_clear_frame(GuiPainter *cx) {
  memset(cx->frame_buffer, 0, cx->width * cx->height);
  cx->debug_line_count = 0;
  BeginDrawing();
}

static inline void gui_debug_println(GuiPainter *cx, const char *text) {
  i32 y = (i32)cx->debug_line_count * 20 + 10;
  ++cx->debug_line_count;
  DrawText(text, 10, y, 20, WHITE);
}

/// Calls raylib to paint the frame buffer into the window.
void gui_finish_frame(GuiPainter *cx, const Renderer *renderer) {
  // Run shader shader.
  for (usize y = 0; y < cx->height; ++y) {
    for (usize x = 0; x < cx->width; ++x) {
      u8 *light_level = &cx->frame_buffer[y * cx->width + x];
      apply_shader(cx->shader_kind, cx->width, cx->height, x, y, light_level, renderer->depth_buffer);
    }
  }

  Image image = (Image){
      .width = (i32)cx->width,
      .height = (i32)cx->height,
      .data = cx->frame_buffer,
      .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE,
      .mipmaps = 1,
  };
  Texture2D raylib_texture = LoadTextureFromImage(image);
  DrawTexture(raylib_texture, 0, 0, WHITE);
  gui_debug_println(cx, TextFormat("FPS: %.0f/%.0f", 1.f / GetFrameTime(), cx->target_fps));
  const char *shader;
  switch (cx->shader_kind) {
  case SHADER_KIND_DEFAULT:
    shader = "BORING";
    break;
  case SHADER_KIND_HIGHLIGHTED:
    shader = "HIGHLIGHTED";
    break;
  case SHADER_KIND_DEBUG_DEPTH:
    shader = "DEBUG DEPTH";
    break;
  case SHADER_KIND_DEBUG_DEPTH_HIGHLIGHTED:
    shader = "DEBUG DEPTH HIGHLIGHTED";
    break;
  case SHADER_KIND_HIGHLIGHT_ONLY:
    shader = "HIGHLIGHT ONLY";
    break;
  }
  gui_debug_println(cx, TextFormat("Shader: [R/Shift+R]: %s", shader));
  gui_debug_println(cx, TextFormat("FOV [+/-/0]: %.1f", to_deg(renderer->cam.fov)));
  gui_debug_println(cx,
                    TextFormat("Camera XYZ: %.02f %.02f %.02f",
                               renderer->cam.pos.get[0],
                               renderer->cam.pos.get[1],
                               renderer->cam.pos.get[2]));
  EndDrawing();
}

void gui_setup_window(GuiPainter *cx) {
  SetTraceLogLevel(LOG_ERROR); // Silence raylib logging.
  InitWindow((i32)cx->width, (i32)cx->height, "Render");
  SetTargetFPS(cx->target_fps == INFINITY ? 2147483647 : (i32)cx->target_fps);
}

[[maybe_unused]]
static inline bool is_shift_down() {
  return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

[[maybe_unused]]
static inline bool is_alt_down() {
  return IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
}

[[maybe_unused]]
static inline bool is_control_down() {
  return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
}

[[maybe_unused]]
static inline bool is_super_down() {
  return IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
}

void gui_handle_event(GuiPainter *cx, Renderer *renderer) {
  if (is_shift_down() && IsKeyPressed(KEY_R)) {
    select_prev_shader(&cx->shader_kind);
    return;
  }
  if (IsKeyPressed(KEY_R)) {
    select_next_shader(&cx->shader_kind);
    return;
  }
  if (IsKeyDown(KEY_EQUAL) || IsKeyDown(KEY_KP_ADD)) {
    renderer->cam.fov -= to_rad(1.f) / ((f32)GetFPS() / 60.f);
  }
  if (IsKeyDown(KEY_MINUS) || IsKeyDown(KEY_KP_SUBTRACT)) {
    renderer->cam.fov += to_rad(1.f) / ((f32)GetFPS() / 60.f);
  }
  if (IsKeyDown(KEY_ZERO) || IsKeyDown(KEY_KP_0)) {
    renderer->cam.fov = to_rad(90.f);
  }
  if (IsKeyDown(KEY_W)) {
    renderer->cam.pos.get[0] -= 0.1f / ((f32)GetFPS() / 60.f);
  }
  if (IsKeyDown(KEY_S)) {
    renderer->cam.pos.get[0] += 0.1f / ((f32)GetFPS() / 60.f);
  }
}

void gui_draw_pixel_callback(void *cx_, usize width, usize height, usize x, usize y, f32 z, u8 light_level) {
  GuiPainter *cx = cx_;
  cx->frame_buffer[y * width + x] = light_level;
}

DEF_DRAW_FUNCTIONS(, _gui, gui_draw_pixel_callback);

