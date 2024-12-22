#pragma once

#include "common.h"

// There are no vertex shaders rn, only fragment shaders.

typedef enum shader_kind {
  SHADER_KIND_DEFAULT,
  SHADER_KIND_HIGHLIGHTED,
  SHADER_KIND_DEBUG_DEPTH,
  SHADER_KIND_DEBUG_DEPTH_HIGHLIGHTED,
  SHADER_KIND_HIGHLIGHT_ONLY,
} ShaderKind;

void select_next_shader(ShaderKind *shader_kind);

void select_prev_shader(ShaderKind *shader_kind);

u8 shader_boring(usize width, usize height, usize x, usize y, u8 light_level, const f32 *depth_buffer);

u8 shader_highlighted(usize width, usize height, usize x, usize y, u8 light_level, const f32 *depth_buffer);

u8 shader_debug_depth(usize width, usize height, usize x, usize y, u8 light_level, const f32 *depth_buffer);

u8 shader_debug_depth_highlighted(usize width,
                                  usize height,
                                  usize x,
                                  usize y,
                                  u8 _light_level,
                                  const f32 *depth_buffer);

u8 shader_highlight_only(usize width, usize height, usize x, usize y, u8 light_level, const f32 *depth_buffer);

void apply_shader(ShaderKind shader_kind,
                  usize width,
                  usize height,
                  usize x,
                  usize y,
                  u8 *fragment,
                  const f32 *depth_buffer);
