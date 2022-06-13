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

    Wheel wheel;
    Bar rotToUp;
    Bar upDown;
    Bar leftRight;

    u32 plotCount;
    v2 *wheelXPlot;
    v2 *wheelYPlot;
    v2 *upDownPlot;
    v2 *leftRightPlot;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(BasicState) <= state->memorySize);

    //v2 size = V2((f32)image->width, (f32)image->height);

    BasicState *basics = (BasicState *)state->memory;
    if (!state->initialized)
    {
        // basics->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        basics->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);

        basics->wheel.pos = V2(100.0f, 300.0f);
        basics->wheel.radius = 50.0f;
        basics->wheel.dir = V2(0, -1);
        basics->wheel.angularVel = 1.0f * F32_TAU;

        basics->rotToUp.startPos = get_wheel_connection(&basics->wheel);
        basics->rotToUp.length = 110.0f;
        basics->rotToUp.dir = V2(0, -1);
        basics->rotToUp.flags = Bar_LockX;

        basics->upDown.startPos = get_bar_connection(&basics->rotToUp);
        basics->upDown.length = 50.0f;
        basics->upDown.dir = V2(0, -1);
        basics->upDown.flags = Bar_LockX;

        basics->leftRight.startPos = get_bar_connection(&basics->upDown);
        basics->leftRight.length = 150.0f;
        basics->leftRight.dir = normalize(V2(1, -0.1f));
        basics->leftRight.flags = Bar_LockY;

        basics->plotCount = 120;
        basics->wheelXPlot = arena_allocate_array(gMemoryArena, v2, basics->plotCount, default_memory_alloc());
        basics->wheelYPlot = arena_allocate_array(gMemoryArena, v2, basics->plotCount, default_memory_alloc());
        basics->upDownPlot = arena_allocate_array(gMemoryArena, v2, basics->plotCount, default_memory_alloc());
        basics->leftRightPlot = arena_allocate_array(gMemoryArena, v2, basics->plotCount, default_memory_alloc());

        for (u32 x = 0; x < basics->plotCount; ++x)
        {
            v2 *wxPoint = basics->wheelXPlot + x;
            v2 *wyPoint = basics->wheelYPlot + x;
            v2 *udPoint = basics->upDownPlot + x;
            v2 *lrPoint = basics->leftRightPlot + x;

            f32 xFactor = (f32)x / (f32)basics->plotCount;

            wxPoint->x = get_wheel_connection(&basics->wheel).x;
            wxPoint->y = 370.0f + xFactor * 200.0f;

            wyPoint->x = 170.0f + xFactor * 300.0f;
            wyPoint->y = get_wheel_connection(&basics->wheel).y;

            udPoint->x = 300.0f + xFactor * 300.0f;
            udPoint->y = get_bar_connection(&basics->upDown).y;

            lrPoint->x = get_bar_connection(&basics->leftRight).x;
            lrPoint->y = 160.0f + xFactor * 300.0f;
        }

        state->initialized = true;
    }

    update_wheel(&basics->wheel, dt);
    update_bar(&basics->rotToUp, get_wheel_connection(&basics->wheel, -5.0f), dt);
    update_bar(&basics->upDown, get_bar_connection(&basics->rotToUp), dt);
    update_bar(&basics->leftRight, get_bar_connection(&basics->upDown), dt);

    for (s32 x = basics->plotCount - 1; x > 0; --x)
    {
        basics->wheelXPlot[x].x = basics->wheelXPlot[x - 1].x;
        basics->wheelYPlot[x].y = basics->wheelYPlot[x - 1].y;
        basics->upDownPlot[x].y = basics->upDownPlot[x - 1].y;
        basics->leftRightPlot[x].x = basics->leftRightPlot[x - 1].x;
    }
    basics->wheelXPlot[0].x = get_wheel_connection(&basics->wheel).x;
    basics->wheelYPlot[0].y = get_wheel_connection(&basics->wheel).y;
    basics->upDownPlot[0].y = get_bar_connection(&basics->upDown).y;
    basics->leftRightPlot[0].x = get_bar_connection(&basics->leftRight).x;

    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));

    draw_wheel(image, &basics->wheel,   V4(1, 1, 0, 1));
    draw_bar(image, &basics->rotToUp,   V4(0, 0, 1, 1));
    draw_bar(image, &basics->upDown,    V4(1, 0, 0, 1));
    draw_bar(image, &basics->leftRight, V4(1, 1, 1, 1));

    draw_lines(image, basics->plotCount, basics->wheelXPlot, V4(1, 1, 0, 1));
    draw_lines(image, basics->plotCount, basics->wheelYPlot, V4(1, 1, 0, 1));
    draw_lines(image, basics->plotCount, basics->upDownPlot, V4(1, 0, 0, 1));
    draw_lines(image, basics->plotCount, basics->leftRightPlot, V4(1, 1, 1, 1));

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
