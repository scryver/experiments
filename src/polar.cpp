#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "random.h"
#include "drawing.cpp"

struct PolarState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    f32 angle;
    f32 radius;
    f32 angleVel;
    f32 angleAcc;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(PolarState) <= state->memorySize);
    PolarState *polar = (PolarState *)state->memory;
    if (!state->initialized)
    {
        polar->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        
        polar->radius = 150.0f;
        polar->angleAcc = 0.001f;
        
        state->initialized = true;
    }
    
    v2 center = V2((f32)image->width * 0.5f, (f32)image->height * 0.5f);
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    v2 xy = polar_to_cartesian(polar->radius, polar->angle) + center;
    draw_line(image, round(center.x), round(center.y), round(xy.x), round(xy.y),
              V4(1, 1, 1, 1));
    fill_circle(image, round(xy.x), round(xy.y), 20.0f, V4(1, 1, 1, 1));
              
    polar->angle += polar->angleVel;
    if (polar->angle > TAU32)
    {
        polar->angle -= TAU32;
    }
    polar->angleVel += polar->angleAcc;
    if (polar->angleVel > 1.0f)
    {
        polar->angleVel = 1.0f;
    }
    if (polar->angleVel < 0.0f)
    {
        polar->angleVel = 0.0f;
    }
    if (polar->radius > 20.0f)
    {
        polar->radius -= 2.0f;
    }
    else
    {
        polar->radius = 150.0f;
    }
    
    ++polar->ticks;
}
