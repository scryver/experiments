#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct PendulumState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    v2 origin;
    v2 bob;
    f32 len;
    
    f32 angle;
    f32 aVel;
    f32 aAcc;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(PendulumState) <= state->memorySize);
    PendulumState *pendulum = (PendulumState *)state->memory;
    if (!state->initialized)
    {
        pendulum->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        
        f32 len = 300.0f;
        pendulum->origin = V2((f32)image->width * 0.5f, 0.0f);
        pendulum->bob = V2((f32)image->width * 0.5f, len);
        pendulum->len = len;
        
        pendulum->angle = F32_TAU * 0.125f;
        
        state->initialized = true;
    }
    
    pendulum->bob = pendulum->origin + pendulum->len * V2(sin(pendulum->angle),
                                                          cos(pendulum->angle));
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    draw_line(image, round(pendulum->origin.x), round(pendulum->origin.y),
              round(pendulum->bob.x), round(pendulum->bob.y), V4(1, 1, 1, 1));
    fill_circle(image, round(pendulum->bob.x), round(pendulum->bob.y), 16, V4(1, 1, 1, 1));
    
    pendulum->angle += pendulum->aVel;
    if (pendulum->angle > F32_TAU)
    {
        pendulum->angle -= F32_TAU;
    }
    
    pendulum->aVel += pendulum->aAcc;
    pendulum->aAcc = -0.01f * sin(pendulum->angle);
    
    pendulum->aVel *= 0.99f;
    
    ++pendulum->ticks;
}
