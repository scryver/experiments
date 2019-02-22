#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct OscillationState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    f32 amplitude;
    f32 angleStep;
    f32 angle;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(OscillationState) <= state->memorySize);
    OscillationState *oscillation = (OscillationState *)state->memory;
    if (!state->initialized)
    {
        oscillation->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        
        oscillation->amplitude = (f32)image->height * 0.5f;
        oscillation->angleStep = F32_TAU / 100.0f; // NOTE(michiel): In period per amount of frames
        
        state->initialized = true;
    }
    
    v2 center = V2((f32)image->width * 0.5f, (f32)image->height * 0.5f);
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    for (u32 xStep = 0; xStep < image->width + 22; xStep += 40)
    {
        f32 y = oscillation->amplitude * sin(2.0f * oscillation->angle + xStep * 0.01f)
            + 0.5f * oscillation->amplitude * sin( oscillation->angle + xStep * 0.01f);
        y *= 0.4f;
        y += center.y;
    fill_circle(image, xStep, round(y), 22, V4(1, 1, 1, 0.9f));
    }
    
    oscillation->angle += oscillation->angleStep;
    if (oscillation->angle > F32_TAU)
    {
        oscillation->angle -= F32_TAU;
    }
    
    ++oscillation->ticks;
}
