#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "forces.h"
#include "drawing.cpp"

struct SpringState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    v2 origin;
    Mover bob;
    f32 restLength;
};

DRAW_IMAGE(draw_image)
{
    v2 center = V2((f32)image->width * 0.5f, (f32)image->height * 0.5f);
    
    i_expect(sizeof(SpringState) <= state->memorySize);
    SpringState *springState = (SpringState *)state->memory;
    if (!state->initialized)
    {
        springState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        
        f32 restLength = 400.0f;
        springState->origin = V2(center.x, 0);
        springState->bob = create_mover(V2(center.x, restLength + 50.0f), 10.0f);
        springState->restLength = restLength;
        
        state->initialized = true;
    }

    apply_spring(&springState->bob, springState->origin, springState->restLength);
    apply_drag(&springState->bob, 0.1f);
    
    update(&springState->bob);
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    draw_line(image, round(springState->origin.x), round(springState->origin.y),
              round(springState->bob.position.x), round(springState->bob.position.y),
              V4(1, 1, 1, 1));
    fill_circle(image, round(springState->bob.position.x), round(springState->bob.position.y),
                16, V4(1, 1, 1, 1));
    
    ++springState->ticks;
}
