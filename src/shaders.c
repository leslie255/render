#include "shaders.h"
#include "math_helpers.h"

void select_next_shader(ShaderKind *shader_kind) {
  if (*shader_kind == 4)
    *shader_kind = 0;
  else
    *shader_kind += 1;
}

void select_prev_shader(ShaderKind *shader_kind) {
  if (*shader_kind == 0)
    *shader_kind = 4;
  else
    *shader_kind -= 1;
}

/// Apply shader onto one fragment.
void apply_shader(ShaderKind shader_kind,
                  usize width,
                  usize height,
                  usize x,
                  usize y,
                  u8 *fragment,
                  const f32 *depth_buffer) {
  switch (shader_kind) {
  case SHADER_KIND_DEFAULT:
    *fragment = shader_boring(width, height, x, y, *fragment, depth_buffer);
    break;
  case SHADER_KIND_HIGHLIGHTED:
    *fragment = shader_highlighted(width, height, x, y, *fragment, depth_buffer);
    break;
  case SHADER_KIND_DEBUG_DEPTH:
    *fragment = shader_debug_depth(width, height, x, y, *fragment, depth_buffer);
    break;
  case SHADER_KIND_DEBUG_DEPTH_HIGHLIGHTED:
    *fragment = shader_debug_depth_highlighted(width, height, x, y, *fragment, depth_buffer);
    break;
  case SHADER_KIND_HIGHLIGHT_ONLY:
    *fragment = shader_highlight_only(width, height, x, y, *fragment, depth_buffer);
    break;
  }
}

static inline u8 nabla_depth(usize width, usize height, usize x, usize y, const f32 *depth_buffer) {
  const u8 radius = 4;
  const u8 step_size = 2;
  // Derivative with respect to x.
  f32 dx = 0;
  f32 dy = 0;
  for (u8 y_eps = 0; y_eps < radius; y_eps += step_size) {
    for (u8 x_eps = 0; x_eps < radius; x_eps += step_size) {
      f32 dist = sqrtf(pow2f(x_eps) + pow2f(y_eps));
      f32 factor = dist / (f32)radius;
      dx += depth_buffer[y * width + (x - x_eps) % width] * factor;
      dx -= depth_buffer[y * width + (x + x_eps) % width] * factor;
      dy += depth_buffer[(y - y_eps) % height * width + x] * factor;
      dy -= depth_buffer[(y + y_eps) % height * width + x] * factor;
    }
  }
  dx *= pow2f((f32)step_size);
  dy *= pow2f((f32)step_size);
  dx /= pow2f((f32)radius);
  dy /= pow2f((f32)radius);
  f32 nabla_depth = sqrtf(dx * dx + dy * dy);
  return (u8)(nabla_depth * 255.f);
}

u8 shader_boring(usize width, usize height, usize x, usize y, u8 light_level, const f32 *depth_buffer) {
  return light_level;
}

u8 shader_highlighted(usize width, usize height, usize x, usize y, u8 light_level, const f32 *depth_buffer) {
  u8 highlight = nabla_depth(width, height, x, y, depth_buffer) / 4;
  return ((255 - light_level) < highlight) ? 255 : light_level + highlight;
}

u8 shader_debug_depth(usize width, usize height, usize x, usize y, u8 light_level, const f32 *depth_buffer) {
  const f32 small = 1.0f;
  f32 z = depth_buffer[y * height + x];
  f32 z_norm = logf(small + sigmoidf(z - 10.f)) / logf(1.f + small);
  return (u8)((1.f - z_norm) * 255.f);
}

u8 shader_debug_depth_highlighted(usize width,
                                  usize height,
                                  usize x,
                                  usize y,
                                  u8 _light_level,
                                  const f32 *depth_buffer) {
  const f32 small = 1.0f;
  f32 z = depth_buffer[y * height + x];
  f32 z_norm = logf(small + sigmoidf(z - 10.f)) / logf(1.f + small);
  u8 light_level = (u8)((1.f - z_norm) * 255.f);
  u8 highlight = nabla_depth(width, height, x, y, depth_buffer);
  return ((255 - light_level) < highlight) ? 255 : light_level + highlight;
}

u8 shader_highlight_only(usize width, usize height, usize x, usize y, u8 light_level, const f32 *depth_buffer) {
  return nabla_depth(width, height, x, y, depth_buffer);
}
