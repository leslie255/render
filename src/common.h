#pragma once

#include <assert.h>
#include <execinfo.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;
typedef float f32;
typedef double f64;
typedef size_t usize;
typedef ssize_t isize;

#define PUT_ON_HEAP(X) memcpy(malloc(sizeof(X)), &X, sizeof(X))

#define ARR_LEN(X) (sizeof(X) / sizeof((X)[0]))

static inline void print_stacktrace() {
  void *callstack[128];
  int frames = backtrace(callstack, 128);
  char **strs = backtrace_symbols(callstack, frames);
  for (u32 i = 0; i < frames; i++) {
    fprintf(stderr, "%s\n", strs[i]);
  }
  free(strs);
}

/// Assert with stacktrace on failure.
#define ASSERT(COND)                                                                                                   \
  (((COND))                                                                                                            \
       ? 0                                                                                                             \
       : (fprintf(stderr, "[%s@%s:%d] Assertion failed: (%s) == false\n", __FUNCTION__, __FILE__, __LINE__, #COND),    \
          print_stacktrace(),                                                                                          \
          exit(1)))
/// Assert with stacktrace and print message on failure.
#define ASSERT_PRINTF(COND, ...)                                                                                       \
  (((COND))                                                                                                            \
       ? 0                                                                                                             \
       : (fprintf(stderr, "[%s@%s:%d] Assertion failed: (%s) == false\n", __FUNCTION__, __FILE__, __LINE__, #COND),    \
          fprintf(stderr, __VA_ARGS__),                                                                                \
          print_stacktrace(),                                                                                          \
          exit(1)))

#ifdef DEBUG
/// Assert only in debug mode with stacktrace on failure.
#define DEBUG_ASSERT(COND)                                                                                             \
  (((COND))                                                                                                            \
       ? 0                                                                                                             \
       : (fprintf(stderr, "[%s@%s:%d] Assertion failed: (%s) == false\n", __FUNCTION__, __FILE__, __LINE__, #COND),    \
          print_stacktrace(),                                                                                          \
          exit(1)))
/// Assert only in debug mode with stacktrace and print message on failure.
#define DEBUG_ASSERT_PRINTF(COND, ...)                                                                                 \
  (((COND))                                                                                                            \
       ? 0                                                                                                             \
       : (fprintf(stderr, "[%s@%s:%d] Assertion failed: (%s) == false\n", __FUNCTION__, __FILE__, __LINE__, #COND),    \
          fprintf(stderr, __VA_ARGS__),                                                                                \
          print_stacktrace(),                                                                                          \
          exit(1)))
#else
#define DEBUG_ASSERT(COND) 0
#define DEBUG_ASSERT_PRINTF(COND, ...) 0
#endif

#define PANIC() (fprintf(stderr, "[%s@%s:%d] PANIC\n", __FUNCTION__, __FILE__, __LINE__), print_stacktrace(), exit(1))
#define PANIC_PRINTF(...)                                                                                              \
  (fprintf(stderr, "[%s@%s:%d] PANIC\n", __FUNCTION__, __FILE__, __LINE__),                                            \
   fprintf(stderr, __VA_ARGS__),                                                                                       \
   print_stacktrace(),                                                                                                 \
   exit(1))

/// Return `0` to the caller if value is `0`
#define TRY_NULL(X)                                                                                                    \
  {                                                                                                                    \
    if ((X) == 0) {                                                                                                    \
      return 0;                                                                                                        \
    }                                                                                                                  \
  }

#define PTR_CAST(TY, X) (*(TY *)&(X))

#define FIXME(...) (printf("[%s@%s:%d] FIXME:", __FUNCTION__, __FILE__, __LINE__), printf(__VA_ARGS__), printf("\n"))

#define TODO() (printf("[%s@%s:%d] TODO\n", __FUNCTION__, __FILE__, __LINE__), print_stacktrace(), exit(1))
#define TODO_FUNCTION()                                                                                                \
  (printf("[%s@%s:%d] TODO: function not implemented\n", __FUNCTION__, __FILE__, __LINE__), print_stacktrace(), exit(1))

__attribute__((always_inline)) static inline void *xalloc_(usize len) {
  void *p = malloc(len);
  ASSERT(p != NULL);
  return p;
}

__attribute__((always_inline)) static inline void *xrealloc_(void *p, usize len) {
  DEBUG_ASSERT(p != NULL);
  p = realloc(p, len);
  ASSERT(p != NULL);
  return p;
}

__attribute__((always_inline)) static inline void xfree(void *p) {
  free(p);
}

#define xalloc(TY, COUNT) ((TY *restrict)xalloc_(sizeof(TY) * (COUNT)))
#define xrealloc(P, TY, COUNT) ((TY *)xrealloc_((P), sizeof(TY) * (COUNT)))
