#pragma once

#include "common.h"
#include "shaders.h"
#include "render.h"

#include <raylib.h>

/// Manages drawing the frame buffer with raylib and handling GUI events.
typedef struct gui_painter {
  ShaderKind shader_kind;
  u8 *frame_buffer;
  usize width;
  usize height;
  f32 target_fps;
  usize debug_line_count;
  Texture2D raylib_texture;
} GuiPainter;

GuiPainter new_gui_painter(usize width, usize height, f32 target_fps);

void free_gui_drawing_cx(GuiPainter cx);

void gui_clear_frame(GuiPainter *cx);

void gui_finish_frame(GuiPainter *cx, const Renderer *renderer);

void gui_setup_window(GuiPainter *cx);

void gui_handle_event(GuiPainter *cx, Renderer *renderer);

void gui_draw_pixel_callback(void *cx_, usize width, usize height, usize x, usize y, f32 z, u8 light_level);

DEF_DRAW_FUNCTIONS_HEADER(, _gui, draw_pixel_callback_gui);

