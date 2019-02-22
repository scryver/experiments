#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include <time.h>

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct LSystemState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    u32 maxBufferSize;
    u8 buffer[2][4096];
    u32 currentBuffer;
    
    u32 currentY;
    
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
    i_expect(sizeof(LSystemState) <= state->memorySize);
    
    LSystemState *lSystem = (LSystemState *)state->memory;
    if (!state->initialized)
    {
        lSystem->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        lSystem->maxBufferSize = 4096;
        lSystem->currentBuffer = 0;
        lSystem->currentY = 0;
        
        state->initialized = true;
        }
    
    if (lSystem->currentY < image->height)
    {
    u32 nextBuffer = (lSystem->currentBuffer + 1) & 1;
        if (lSystem->buffer[lSystem->currentBuffer][0] == 0)
    {
        lSystem->buffer[nextBuffer][0] = 'A';
        }
    else
    {
            u8 *d = lSystem->buffer[nextBuffer];
        for (u32 i = 0; i < lSystem->maxBufferSize / 4; ++i)
            {
                u8 s = lSystem->buffer[lSystem->currentBuffer][i];
                
                if (s == 'A')
                {
                    *d++ = 'A';
                    *d++ = 'B';
                    *d++ = 'A';
                }
                else if (s == 'B')
                {
                    *d++ = 'B';
                    *d++ = 'B';
                    *d++ = 'B';
                }
                else
                {
                    break;
                }
            }
        }
    lSystem->currentBuffer = nextBuffer;
    
        u8 *s = lSystem->buffer[lSystem->currentBuffer];
        for (u32 x = 0; x < image->width; ++x)
        {
            v4 colour = V4(1, 1, 1, 1);
            if (*s == 'A')
            {
                colour = V4(1, 0, 0, 1);
                }
            else if (*s == 'B')
            {
                colour = V4(0, 0, 1, 1);
            }
            else
            {
                break;
            }
            draw_pixel(image, x, lSystem->currentY, colour);
        ++s;
            }
        ++lSystem->currentY;
    }
    
    lSystem->prevMouseDown = mouse.mouseDowns;
    ++lSystem->ticks;
}
