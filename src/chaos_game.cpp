#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct ChaosState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;

    v2 points[3];
    v2 lastPoint;

    u32 prevMouseDown;
};

internal void
choose_triangle(RandomSeriesPCG *random, v2 size, v2 *points)
{
    points[0].x = random_unilateral(random) * size.x;
    points[0].y = random_unilateral(random) * size.y;
    points[1].x = random_unilateral(random) * size.x;
    points[1].y = random_unilateral(random) * size.y;
    points[2].x = random_unilateral(random) * size.x;
    points[2].y = random_unilateral(random) * size.y;
}

internal v2
choose_point(RandomSeriesPCG *random, v2 *points)
{
    u32 point = random_choice(random, 3);
    v2 result = points[point];

    v2 goTo = points[(point + 1) % 3];
    v2 diff = goTo - result;
    diff *= random_unilateral(random);
    result += diff;

    goTo = points[(point + 2) % 3];
    diff = goTo - result;
    diff *= random_unilateral(random);
    result += diff;

    return result;
}

internal void
restart(Image *image, ChaosState *state)
{
    v2 size = V2((f32)image->width, (f32)image->height);

    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));

    choose_triangle(&state->randomizer, size, state->points);
    state->lastPoint = choose_point(&state->randomizer, state->points);

    fill_rectangle(image, state->points[0].x, state->points[0].y, 1, 1, V4(1, 1, 1, 1));
    fill_rectangle(image, state->points[1].x, state->points[1].y, 1, 1, V4(1, 1, 1, 1));
    fill_rectangle(image, state->points[2].x, state->points[2].y, 1, 1, V4(1, 1, 1, 1));
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(ChaosState) <= state->memorySize);

    //v2 size = V2((f32)image->width, (f32)image->height);

    ChaosState *chaos = (ChaosState *)state->memory;
    if (!state->initialized)
    {
        // chaos->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        chaos->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);

        restart(image, chaos);

        state->initialized = true;
    }

    if (is_pressed(&mouse, Mouse_Left))
    {
        restart(image, chaos);
    }

    for (u32 x = 0; x < 10; ++x)
    {
        fill_rectangle(image, round32(chaos->lastPoint.x), round32(chaos->lastPoint.y), 1, 1,
                       V4(1, 1, 1, 1));

        u32 index = random_choice(&chaos->randomizer, 3);
        v2 goTo = chaos->points[index];
        v2 diff = goTo - chaos->lastPoint;
        chaos->lastPoint += diff * 0.5f;
    }

    chaos->seconds += dt;
    ++chaos->ticks;
    if (chaos->seconds > 1.0f)
    {
        chaos->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", chaos->ticks, 1000.0f / (f32)chaos->ticks);
    }
}
