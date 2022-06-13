#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct Wheel
{
    v2 pos;
    f32 radius;
    v2 dir;
    f32 angularVel;
    f32 angularS;
};

internal inline v2
get_wheel_connection(Wheel *wheel, f32 offset = -3.0f)
{
    v2 result = {};
    result = wheel->pos + (wheel->radius + offset) * wheel->dir;
    return result;
}

enum BarFlags
{
    Bar_LockX = 0x1,
    Bar_LockY = 0x2,
};
struct Bar
{
    v2 startPos;
    f32 length;
    v2 dir;
    u32 flags;
};

internal inline v2
get_bar_connection(Bar *bar)
{
    v2 result = {};
    result = bar->startPos + bar->length * bar->dir;
    return result;
}

internal void
update_wheel(Wheel *wheel, f32 dt)
{
    f32 angStep = wheel->angularVel * dt;
    wheel->angularS += angStep;
    if (wheel->angularS > F32_TAU)
    {
        wheel->angularS -= F32_TAU;
    }
    v2 sincos = sincos_pi(wheel->angularS);
    wheel->dir.x = sincos.y;
    wheel->dir.y = sincos.x;
}

internal void
update_bar(Bar *bar, v2 newStart, f32 dt)
{
    v2 curEnd = get_bar_connection(bar);
    v2 newEnd = curEnd + (newStart - bar->startPos);

    if (bar->flags & Bar_LockX)
    {
        if (newEnd.x != curEnd.x)
        {
            newEnd.x = curEnd.x;
            if (curEnd.y < newStart.y)
            {
                newEnd.y = newStart.y - square_root(square(bar->length) - square(curEnd.x - newStart.x));
            }
            else
            {
                newEnd.y = newStart.y + square_root(square(bar->length) - square(curEnd.x - newStart.x));
            }
        }
    }
    else if (bar->flags & Bar_LockY)
    {
        if (curEnd.y != newEnd.y)
        {
            if (curEnd.x < bar->startPos.x)
            {
                newEnd.x = newStart.x - square_root(square(bar->length) - square(curEnd.y - newStart.y));
            }
            else
            {
                newEnd.x = newStart.x + square_root(square(bar->length) - square(curEnd.y - newStart.y));
            }
            newEnd.y = curEnd.y;
        }
    }

    bar->startPos = newStart;
    bar->dir = normalize(newEnd - newStart);
}

internal void
draw_wheel(Image *image, Wheel *wheel, v4 colour = V4(1, 1, 1, 1))
{
    fill_circle(image, round32(wheel->pos.x), round32(wheel->pos.y),
                round32(wheel->radius), colour);
}

internal void
draw_bar(Image *image, Bar *bar, v4 colour = V4(1, 1, 1, 1))
{
    v2 end = get_bar_connection(bar);
    draw_line(image, round32(bar->startPos.x), round32(bar->startPos.y),
              round32(end.x), round32(end.y), colour);
}

struct BasicState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(BasicState) <= state->memorySize);

    v2 size = V2((f32)image->width, (f32)image->height);

    BasicState *basics = (BasicState *)state->memory;
    if (!state->initialized)
    {
        basics->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        //basics->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);

        state->initialized = true;
    }

    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));

    f32 a = 38.0f;
    f32 b = 41.5f;
    f32 c = 39.3f;
    f32 d = 40.1f;
    f32 e = 55.8f;
    f32 f = 39.4f;
    f32 g = 36.7f;
    f32 h = 65.7f;
    f32 i = 49.0f;
    f32 j = 50.0f;
    f32 k = 61.9f;
    f32 l =  7.8f;
    f32 m = 15.0f;
    unused(b);
    unused(c);
    unused(d);
    unused(e);
    unused(f);
    unused(g);
    unused(h);
    unused(i);
    unused(j);
    unused(k);

    v2 midPoint = size * 0.5f;
    v2 fixedPoint = midPoint - V2(a, l);

    outline_circle(image, round32(midPoint.x), round32(midPoint.y), (u32)m, 1.0f, V4(1, 1, 1, 1));
    fill_circle(image, round32(midPoint.x), round32(midPoint.y), 2, V4(1, 0, 0, 1));
    fill_circle(image, round32(fixedPoint.x), round32(fixedPoint.y), 2, V4(1, 0, 0, 1));

    //draw_line(image, round(midPoint.x), round(midPoint.y), round(midPoint.x - a)

    basics->seconds += dt;
    ++basics->ticks;
    if (basics->seconds > 1.0f)
    {
        basics->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", basics->ticks,
                1000.0f / (f32)basics->ticks);
        basics->ticks = 0;
    }
}
