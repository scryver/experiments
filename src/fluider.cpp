#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct FluidState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;

    f32 t;
    PerlinNoise noise;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(FluidState) <= state->memorySize);

    v2 size = V2((f32)image->width, (f32)image->height);

    FluidState *fluidState = (FluidState *)state->memory;
    if (!state->initialized)
    {
        fluidState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        //fluidState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        init_perlin_noise(&fluidState->noise, &fluidState->randomizer);

        state->initialized = true;
    }

    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));

    u32 tileSize = 20;
    v2 oneOverSize = 1.0f / size;
    f32 halfTileSize = 0.5f * (f32)tileSize;
    for (u32 y = 0; y < image->height; y += tileSize)
    {
        for (u32 x = 0; x < image->width; x += tileSize)
        {
            v2 center = V2((f32)x + halfTileSize, (f32)y + halfTileSize);
            f32 perlin = perlin_noise(&fluidState->noise, V3(2.55f * hadamard(center, oneOverSize), fluidState->t));
            v2 target = polar_to_cartesian(1.0f, perlin * F32_PI) * (halfTileSize - 2.0f) + center;

            perlin = clamp01(0.5f * perlin + 0.5f); // * 0.5f + 0.5f);
            fill_rectangle(image, x, y, tileSize, tileSize, V4(perlin, perlin, perlin, 1.0f));

            draw_line(image, center, target, V4(0.0f, 0.0f, 1.0f, 1.0f));
            fill_circle(image, target, 2.0f, V4(0.0f, 0.0f, 1.0f, 1.0f));
        }
    }

    fluidState->t += dt * 0.1f;
    if (fluidState->t >= 256.0f)
    {
        fprintf(stdout, "Repeat...\n");
        fluidState->t -= 512.0f;
    }

    fluidState->seconds += dt;
    ++fluidState->ticks;
    if (fluidState->seconds > 1.0f)
    {
        fluidState->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", fluidState->ticks,
                1000.0f / (f32)fluidState->ticks);
        fluidState->ticks = 0;
    }
}

