#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define global   static
#define internal static

typedef uint8_t  u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t  s32;

typedef s32      b32;

typedef float    f32;

typedef size_t   umm;

#define U32_MAX 0xFFFFFFFF

#define TAU32   6.28318530717958647692528676655900576838f
#include <math.h>
#include "vectors.h"

struct Rectangle2u
{
    v2u min;
    v2u max;
};

#define sin  sinf
#define cos  cosf
#define sqrt sqrtf

#define minimum(a, b)  ((a) < (b) ? (a) : (b))
#define maximum(a, b)  ((a) > (b) ? (a) : (b))
#define absolute(a)    (((a) < 0) ? -(a) : (a))
#define round(f)       ((s32)((f) + 0.5f))
#define trunc(f)       ((s32)(f))
#define lerp(t, a, b)  ((a) + (t) * ((b) - (a)))

#define array_count(a) (sizeof(a) / sizeof(a[0]))

#define i_expect(a)    do { \
    if (!(a)) { fprintf(stderr, "Expectance not met: " #a "\n"); __builtin_trap(); } } while (0)

struct State
{
    b32 initialized;
    u32 memorySize;
    u8 *memory;
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

internal v2
polar_to_cartesian(f32 r, f32 theta)
{
    v2 result;
    result.x = r * cos(theta);
    result.y = r * sin(theta);
    return result;
}

internal v2
cartesian_to_polar(f32 x, f32 y)
{
    v2 result;
    result.x = length(V2(x, y)); // r
    result.y = 0; // theta
    return result;
}
