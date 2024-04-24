#include "common.h"
#include "debug_utils.h"

typedef struct mat3x3 {
  f32 get[3][3];
} Mat3x3;

typedef struct vec3 {
  f32 get[3];
} Vec3;

static inline void println_mat3x3(Mat3x3 m);
static inline void print_vec3(Vec3 v);
static inline void println_vec3(Vec3 v);

/// |v|
static inline f32 abs3(Vec3 v);

static inline Vec3 add3(Vec3 x, Vec3 y);

static inline Vec3 sub3(Vec3 x, Vec3 y);

static inline Mat3x3 add3x3(Mat3x3 x, Mat3x3 y);

static inline Mat3x3 sub3x3(Mat3x3 x, Mat3x3 y);

/// [ _ _ _ ] [ _ _ _ ]   [ _ _ _ ]
/// [ _ _ _ ] [ _ _ _ ] = [ _ _ _ ]
/// [ _ _ _ ] [ _ _ _ ]   [ _ _ _ ]
static inline Mat3x3 mul3x3(Mat3x3 x, Mat3x3 y);

/// [ _ _ _ ] [ _ ]   [ _ ]
/// [ _ _ _ ] [ _ ] = [ _ ]
/// [ _ _ _ ] [ _ ]   [ _ ]
static inline Vec3 mul3x3_3(Mat3x3 x, Vec3 y);

/// Cross product.
/// [ _ ]   [ _ ]   [ _ ]
/// [ _ ] X [ _ ] = [ _ ]
/// [ _ ]   [ _ ]   [ _ ]
static inline Vec3 cross3(Vec3 x, Vec3 y);

/// [ _ ]
/// [ _ ] . [ _ _ _ ] = _
/// [ _ ]
static inline f32 dot3(Vec3 x, Vec3 y);

/// 3x3 matrix that performs a rotation along the x axis.
/// Angle in radian.
static inline Mat3x3 rotate3d_x(f32 th);

/// 3x3 matrix that performs a rotation along the x axis.
/// Angle in radian.
static inline Mat3x3 rotate3d_y(f32 th);

/// 3x3 matrix that performs a rotation along the x axis.
/// Angle in radian.
static inline Mat3x3 rotate3d_z(f32 th);

/// Convert a angle expressed in degree to radian.
static inline f32 to_rad(f32 deg);

/// Convert a angle expressed in radian to degree.
static inline f32 to_deg(f32 rad);

// ---------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------ Implementation -----------------------------------------------------

static inline void println_mat3x3(Mat3x3 m) {
  for (usize y = 0; y < 3; ++y) {
    printf("%f %f %f\n", m.get[0][y], m.get[1][y], m.get[2][y]);
  }
}

static inline void print_vec3(Vec3 v) {
  printf("(%f, %f, %f)", v.get[0], v.get[1], v.get[2]);
}

static inline void println_vec3(Vec3 v) {
  print_vec3(v);
  printf("\n");
}

static inline f32 abs3(Vec3 v) {
  return sqrtf((v.get[0] * v.get[0] + v.get[1] * v.get[1] + v.get[2] * v.get[2]));
}

static inline Vec3 add3(Vec3 x, Vec3 y) {
  return (Vec3){
      x.get[0] + y.get[0],
      x.get[1] + y.get[1],
      x.get[2] + y.get[2],
  };
}

static inline Vec3 sub3(Vec3 x, Vec3 y) {
  return (Vec3){
      x.get[0] - y.get[0],
      x.get[1] - y.get[1],
      x.get[2] - y.get[2],
  };
}

static inline Mat3x3 add3x3(Mat3x3 x, Mat3x3 y) {
  return (Mat3x3){{
      {x.get[0][0] + y.get[0][0], x.get[0][1] + y.get[0][1], x.get[0][2] + y.get[0][2]},
      {x.get[1][0] + y.get[1][0], x.get[1][1] + y.get[1][1], x.get[1][2] + y.get[1][2]},
      {x.get[2][0] + y.get[2][0], x.get[2][1] + y.get[2][1], x.get[2][2] + y.get[2][2]},
  }};
}

static inline Mat3x3 sub3x3(Mat3x3 x, Mat3x3 y) {
  return (Mat3x3){{
      {x.get[0][0] - y.get[0][0], x.get[0][1] - y.get[0][1], x.get[0][2] - y.get[0][2]},
      {x.get[1][0] - y.get[1][0], x.get[1][1] - y.get[1][1], x.get[1][2] - y.get[1][2]},
      {x.get[2][0] - y.get[2][0], x.get[2][1] - y.get[2][1], x.get[2][2] - y.get[2][2]},
  }};
}

static inline Mat3x3 mul3x3(Mat3x3 x, Mat3x3 y) {
  // [ x00 x01 x02 ]   [ y00 y01 y02 ]
  // [ x10 x11 x12 ] * [ y10 y11 y12 ]
  // [ x20 x21 x22 ]   [ y20 y21 y22 ]
  //   [ (x00 y00 + x01 y10 + x02 y20) (x00 y01 + x01 y11 + x02 y21) (x00 y20 + x01 y21 + x02 y22) ]
  // = [ (x10 y00 + x11 y10 + x12 y20) (x10 y01 + x11 y11 + x12 y21) (x10 y20 + x11 y21 + x12 y22) ]
  //   [ (x20 y00 + x21 y10 + x22 y20) (x20 y01 + x21 y11 + x22 y21) (x20 y20 + x21 y21 + x22 y22) ]
  return (Mat3x3){{
      {
          // row 0
          x.get[0][0] * y.get[0][0] + x.get[0][1] * y.get[1][0] + x.get[0][2] * y.get[2][0], // col 0
          x.get[0][0] * y.get[0][1] + x.get[0][1] * y.get[1][1] + x.get[0][2] * y.get[2][1], // col 1
          x.get[0][0] * y.get[0][2] + x.get[0][1] * y.get[1][2] + x.get[0][2] * y.get[2][2], // col 2
      },
      {
          // row 1
          x.get[1][0] * y.get[0][0] + x.get[1][1] * y.get[1][0] + x.get[1][2] * y.get[2][0], // col 0
          x.get[1][0] * y.get[0][1] + x.get[1][1] * y.get[1][1] + x.get[1][2] * y.get[2][1], // col 1
          x.get[1][0] * y.get[0][2] + x.get[1][1] * y.get[1][2] + x.get[1][2] * y.get[2][2], // col 2
      },
      {
          // row 2
          x.get[2][0] * y.get[0][0] + x.get[2][1] * y.get[1][0] + x.get[2][2] * y.get[2][0], // col 0
          x.get[2][0] * y.get[0][1] + x.get[2][1] * y.get[1][1] + x.get[2][2] * y.get[2][1], // col 1
          x.get[2][0] * y.get[0][2] + x.get[2][1] * y.get[1][2] + x.get[2][2] * y.get[2][2], // col 2
      },
  }};
}

static inline Vec3 mul3x3_3(Mat3x3 x, Vec3 y) {
  // [ x00 x01 x02 ]   [ y0 ]   [ x00 y0 + x01 y1 + x02 y2 ]
  // [ x10 x11 x12 ] * [ y1 ] = [ x10 y0 + x11 y1 + x12 y2 ]
  // [ x20 x21 x22 ]   [ y2 ]   [ x20 y0 + x21 y1 + x22 y2 ]
  return (Vec3){{
      x.get[0][0] * y.get[0] + x.get[0][1] * y.get[1] + x.get[0][2] * y.get[2],
      x.get[1][0] * y.get[0] + x.get[1][1] * y.get[1] + x.get[1][2] * y.get[2],
      x.get[2][0] * y.get[0] + x.get[2][1] * y.get[1] + x.get[2][2] * y.get[2],
  }};
}

static inline Vec3 cross3(Vec3 x, Vec3 y) {
  // [ x0 ]   [ y0 ]   [ x1 y2 - x2 y1 ]
  // [ x1 ] X [ y1 ] = [ x2 y0 - x0 y2 ]
  // [ x2 ]   [ y2 ]   [ x0 y1 - x1 y0 ]
  return (Vec3){{
      x.get[1] * y.get[2] - x.get[2] * y.get[1],
      x.get[2] * y.get[0] - x.get[0] * y.get[2],
      x.get[0] * y.get[1] - x.get[1] * y.get[0],
  }};
}

static inline f32 dot3(Vec3 x, Vec3 y) {
  // [ x0 ]
  // [ x1 ] X [ y0 y1 y2] = x0 y0 + x1 y1 + x2 y2
  // [ x2 ]
  return x.get[0] * y.get[0] + x.get[1] * y.get[1] + x.get[2] * y.get[2];
}

static inline Mat3x3 rotate3d_x(f32 th) {
  return (Mat3x3){{
      {1, 0, 0},
      {0, cosf(th), -sinf(th)},
      {0, sinf(th), cosf(th)},
  }};
}

static inline Mat3x3 rotate3d_y(f32 th) {
  return (Mat3x3){{
      {cosf(th), 0, sinf(th)},
      {0, 1, 0},
      {-sinf(th), 0, cosf(th)},
  }};
}

static inline Mat3x3 rotate3d_z(f32 th) {
  return (Mat3x3){{
      {cosf(th), -sinf(th), 0},
      {sinf(th), cosf(th), 0},
      {0, 0, 1},
  }};
}

static inline f32 to_rad(f32 deg) {
  return deg / 180 * 3.14159265358979323846264338327950288f;
}

static inline f32 to_deg(f32 rad) {
  return rad * 180 / 3.14159265358979323846264338327950288f;
}
