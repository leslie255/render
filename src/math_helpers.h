#pragma once

#include "common.h"

static inline u32 saturating_addu(u32 x, u32 y) {
  u32 result = x + y;
  if (result < x || result < y)
    result = 0xFFFFFFFF;
  return result;
}

static inline u32 saturating_subu(u32 x, u32 y) {
  u32 result = x - y;
  if (result > x)
    result = 0;
  return result;
}

static inline usize saturating_addzu(usize x, usize y) {
  usize result = x + y;
  if (result < x || result < y)
    result = SIZE_MAX;
  return result;
}

static inline usize saturating_subzu(usize x, usize y) {
  usize result = x - y;
  if (result > x)
    result = 0;
  return result;
}

static inline f32 pow2f(f32 x) {
  return x * x;
}

static inline f32 sigmoidf(f32 x) {
  return 1.f / (1.f + expf(-x));
}

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
