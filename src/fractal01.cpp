#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include <time.h>

#include "main.cpp"

#include "drawing.cpp"

struct FractalState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    u32 prevMouseDown;
};

internal void
draw_circle(Image *image, f32 x, f32 y, f32 r)
{
    if (r > 0.01f)
    {
    outline_circle(image, x, y, r, 1.0f, V4(0, 0, 0, 1));
        draw_circle(image, x + r, y, r * 0.5f);
        draw_circle(image, x - r, y, r * 0.5f);
        //draw_circle(image, x, y + r, r * 0.5f);
    }
}

DRAW_IMAGE(draw_image)
{
    FractalState *fractalState = (FractalState *)state->memory;
    if (!state->initialized)
    {
        fractalState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        state->initialized = true;
        
        fill_rectangle(image, 0, 0, image->width, image->height, V4(1, 1, 1, 1));
        draw_circle(image, 0.5f * image->width, 0.5f * image->height, 200.0f);
        }
    
    fractalState->prevMouseDown = mouse.mouseDowns;
    ++fractalState->ticks;
}
