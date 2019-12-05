#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "../libberdip/perlin.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct MaurerRose
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    f32 d;
    f32 n;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(MaurerRose) <= state->memorySize);
    
    v2 size = V2((f32)image->width, (f32)image->height);
    
    MaurerRose *maurer = (MaurerRose *)state->memory;
    if (!state->initialized)
    {
        // maurer->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        maurer->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        state->initialized = true;
    }
    
#if 1
    maurer->d = clamp(1.0f, maurer->d, 361.0f);
    maurer->n = clamp(1.0f, maurer->n, 21.0f);
#else
    maurer->d = 29.0f;
    maurer->n = 2.0f;
#endif
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    v2 linePoints[361];
    v2 center = 0.5f * size;
    for (u32 i = 0; i < array_count(linePoints); ++i)
    {
        f32 k = (f32)i * maurer->d;
        f32 l = sin(deg2rad(k*maurer->n));
        linePoints[i] = polar_to_cartesian(200.0f * l, deg2rad(k)) + center;
    }
    draw_lines(image, array_count(linePoints), linePoints, V4(1, 1, 1, 1));
    
    maurer->prevMouseDown = mouse.mouseDowns;
    maurer->seconds += dt;
    ++maurer->ticks;
    if ((maurer->ticks % 10) == 0)
    {
        fprintf(stdout, "D: %2.0f, N: %2.0f\n", maurer->d, maurer->n);
        
        ++maurer->d;
        if (maurer->d > 360.0f)
        {
            maurer->d = 1.0f;
            ++maurer->n;
            if (maurer->n > 20.0f)
            {
                state->closeProgram = true;
            }
        }
    }
    if (maurer->seconds > 1.0f)
    {
        maurer->seconds -= 1.0f;
        maurer->ticks = 0;
    }
}

