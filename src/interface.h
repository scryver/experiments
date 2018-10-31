#include <stdint.h>
#include <float.h>
#include <stddef.h>
#include <stdio.h>

#ifndef __has_builtin                   // Optional of course.
#define __has_builtin(x)                0  // Compatibility with non-clang compilers.
#endif

#if (!__has_builtin(__builtin_sinf) || !__has_builtin(__builtin_cosf) || !__has_builtin(__builtin_sqrtf))
#error No maths!
#include <math.h>
#endif

#define global   static
#define internal static

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t  s32;
typedef int64_t  s64;

typedef s32      b32;

typedef float    f32;
typedef double   f64;

typedef size_t   umm;

#define U32_MAX 0xFFFFFFFF

#define enum32(name) u32

#define F32_SIGN_MASK                   0x80000000
#define F32_EXP_MASK                    0x7F800000
#define F32_FRAC_MASK                   0x007FFFFF

#define F32_MAX   FLT_MAX  // NOTE(michiel): Sign bit 0, Exponent (8bit) 254,     Mantissa all 1's
#define F32_MIN  -FLT_MAX  // NOTE(michiel): Sign bit 0, Exponent (8bit) 254,     Mantissa all 1's
#define F32_INF   3.403e38 // NOTE(michiel): Sign bit 0, Exponent (8bit) all 1's, Mantissa all 0's
#define F32_MINF -3.403e38 // NOTE(michiel): Sign bit 0, Exponent (8bit) all 1's, Mantissa all 0's

#define TAU32   6.28318530717958647692528676655900576838f

internal inline f32
fast_expf(f32 x, u32 approx = 12)
{
    f32 result = 1.0f;
    result += x / (f32)(1 << approx);
    u32 approx4 = approx >> 2;
    u32 approxR = approx & 0x3;
    for (u32 guess = 0; guess < approx4; ++guess)
    {
        result *= result;
        result *= result;
        result *= result;
        result *= result;
    }
    for (u32 guess = 0; guess < approxR; ++guess)
    {
        result *= result;
    }
    return result;
}

#define sin  __builtin_sinf
#define cos  __builtin_cosf
#define sqrt __builtin_sqrtf
#define exp  __builtin_expf
//#define exp  fast_expf
#define log   __builtin_logf

#define kilobytes(a)   (1024ULL * a)
#define megabytes(a)   (1024ULL * kilobytes(a))
#define gigabytes(a)   (1024ULL * megabytes(a))

#define minimum(a, b)  ((a) < (b) ? (a) : (b))
#define maximum(a, b)  ((a) > (b) ? (a) : (b))
#define absolute(a)    (((a) < 0) ? -(a) : (a))
#define square(a)      ((a) * (a))
#define round(f)       ((s32)((f) + ((f) < 0.0f ? -0.5f : 0.5f)))
//#define round          __builtin_roundf

#if 0
inline s32
round_f32_to_s32(f32 f)
{
    s32 result = _mm_cvtss_si32(_mm_set_ss(f));
    return result;
}

inline u32
round_f32_to_u32(f32 f)
{
    u32 result = (u32)_mm_cvtss_si32(_mm_set_ss(f));
    return result;
}
#endif

#define trunc(f)       ((s32)(f))
#define lerp(t, a, b)  ((a) + (t) * ((b) - (a)))

#define array_count(a) (sizeof(a) / sizeof(a[0]))

#define i_expect(a)    do { \
    if (!(a)) { fprintf(stderr, "%s:%d::Expectance not met: '%s'\n", __FILE__, __LINE__, #a); __builtin_trap(); } } while (0)

#define INVALID_CODE_PATH    i_expect(!"Invalid code path!");
#define INVALID_DEFAULT_CASE default: { i_expect(!"Invalid default case!"); } break

internal inline f32 
clamp01(f32 value)
{
    if (value < 0.0f) 
    {
        value = 0.0f;
    }
    if (value > 1.0f)
    {
        value = 1.0f;
    }
    return value;
}

#include "vectors.h"
#include "complex.h"
#include "multithread.h"

struct Rectangle2u
{
    v2u min;
    v2u max;
};

struct State
{
    b32 initialized;
    u32 memorySize;
    u8 *memory;
    
    PlatformWorkQueue *workQueue;
};

struct Image
{
    u32 width;
    u32 height;
    u32 *pixels;
};

enum MouseButtons
{
    Mouse_Left      = 0x01,
    Mouse_Middle    = 0x02,
    Mouse_Right     = 0x04,
    Mouse_Extended1 = 0x08,
    Mouse_Extended2 = 0x10,
};
struct Mouse
{
    v2 pixelPosition;
    u32 mouseDowns;
};

#define DRAW_IMAGE(name) void name(State *state, Image *image, Mouse mouse, f32 dt)
typedef DRAW_IMAGE(DrawImage);

internal inline f32
map(f32 value, f32 fromMin, f32 fromMax, f32 toMin, f32 toMax)
{
    f32 result;
    result = (value - fromMin) / (fromMax - fromMin);
    result = result * (toMax - toMin) + toMin;
    return result;
}

internal inline f64
map(f64 value, f64 fromMin, f64 fromMax, f64 toMin, f64 toMax)
{
    f64 result;
    result = (value - fromMin) / (fromMax - fromMin);
    result = result * (toMax - toMin) + toMin;
    return result;
}

internal inline v2
map(v2 value, v2 fromMin, v2 fromMax, v2 toMin, v2 toMax)
{
    v2 result;
    result.x = map(value.x, fromMin.x, fromMax.x, toMin.x, toMax.x);
    result.y = map(value.y, fromMin.y, fromMax.y, toMin.y, toMax.y);
    return result;
}

internal inline v2
polar_to_cartesian(f32 r, f32 theta)
{
    v2 result;
    result.x = r * cos(theta);
    result.y = r * sin(theta);
    return result;
}
