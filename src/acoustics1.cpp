#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "forces.h"
#include "../libberdip/drawing.cpp"

struct Springy
{
    Mover dynamics;
    v2 origin;
    f32 restLength;
};

struct Acoustic1
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    Springy doubleSpring;
    Springy doubleMass;
    Springy lengthySpring;
    Springy inBetween;
};

internal void
init_springy(Springy *spring, v2 origin, f32 restLength, f32 mass = 0.001f)
{
    spring->restLength = restLength;
    spring->origin = origin;
    spring->dynamics = create_mover(V2(origin.x, origin.y + restLength), mass);
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(Acoustic1) <= state->memorySize);
    
    //v2 size = V2((f32)image->width, (f32)image->height);
    
    Acoustic1 *acoust = (Acoustic1 *)state->memory;
    if (!state->initialized)
    {
        // acoust->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        acoust->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        init_springy(&acoust->doubleSpring, V2(20, 20), 200.0f);
        init_springy(&acoust->doubleMass, V2(120, 20), 200.0f, 0.002f);
        init_springy(&acoust->lengthySpring, V2(220, 20), 400.0f);
        init_springy(&acoust->inBetween, V2(320, 20), 200.0f);
        
        apply_force(&acoust->doubleSpring.dynamics, V2(0, -100.0f));
        apply_force(&acoust->doubleMass.dynamics, V2(0, -100.0f));
        apply_force(&acoust->lengthySpring.dynamics, V2(0, -100.0f));
        apply_force(&acoust->inBetween.dynamics, V2(0, -100.0f));
        
        state->initialized = true;
    }
    
    apply_spring(&acoust->doubleSpring.dynamics, acoust->doubleSpring.origin,
                 acoust->doubleSpring.restLength);
    apply_spring(&acoust->doubleSpring.dynamics, acoust->doubleSpring.origin,
                 acoust->doubleSpring.restLength);
    
    apply_spring(&acoust->doubleMass.dynamics, acoust->doubleMass.origin,
                 acoust->doubleMass.restLength);
    
    apply_spring(&acoust->lengthySpring.dynamics, acoust->lengthySpring.origin,
                 acoust->lengthySpring.restLength);
    
    apply_spring(&acoust->inBetween.dynamics, acoust->inBetween.origin,
                 acoust->inBetween.restLength);
    apply_spring(&acoust->inBetween.dynamics, acoust->inBetween.origin + V2(0, 2.0f * acoust->inBetween.restLength),
                 acoust->inBetween.restLength);
    
    update(&acoust->doubleSpring.dynamics, dt);
    update(&acoust->doubleMass.dynamics, dt);
    update(&acoust->lengthySpring.dynamics, dt);
    update(&acoust->inBetween.dynamics, dt);
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    draw_line(image, round(acoust->doubleSpring.origin.x) - 10, round(acoust->doubleSpring.origin.y), round(acoust->doubleSpring.origin.x) + 10, round(acoust->doubleSpring.origin.y), V4(1, 1, 1, 1));
    draw_line(image, round(acoust->doubleSpring.origin.x), round(acoust->doubleSpring.origin.y),
              round(acoust->doubleSpring.dynamics.position.x), round(acoust->doubleSpring.dynamics.position.y), 
              V4(1, 1, 1, 1));
    fill_circle(image, round(acoust->doubleSpring.dynamics.position.x),
                round(acoust->doubleSpring.dynamics.position.y), 10, V4(1, 1, 1, 1));
    
    draw_line(image, round(acoust->doubleMass.origin.x) - 10, round(acoust->doubleMass.origin.y), round(acoust->doubleMass.origin.x) + 10, round(acoust->doubleMass.origin.y), V4(1, 1, 1, 1));
    draw_line(image, round(acoust->doubleMass.origin.x), round(acoust->doubleMass.origin.y),
              round(acoust->doubleMass.dynamics.position.x), round(acoust->doubleMass.dynamics.position.y), 
              V4(1, 1, 1, 1));
    fill_circle(image, round(acoust->doubleMass.dynamics.position.x),
                round(acoust->doubleMass.dynamics.position.y), 10, V4(1, 1, 1, 1));
    
    draw_line(image, round(acoust->lengthySpring.origin.x) - 10, round(acoust->lengthySpring.origin.y), round(acoust->lengthySpring.origin.x) + 10, round(acoust->lengthySpring.origin.y), V4(1, 1, 1, 1));
    draw_line(image, round(acoust->lengthySpring.origin.x), round(acoust->lengthySpring.origin.y),
              round(acoust->lengthySpring.dynamics.position.x), round(acoust->lengthySpring.dynamics.position.y), 
              V4(1, 1, 1, 1));
    fill_circle(image, round(acoust->lengthySpring.dynamics.position.x),
                round(acoust->lengthySpring.dynamics.position.y), 10, V4(1, 1, 1, 1));
    
    draw_line(image, round(acoust->inBetween.origin.x) - 10, round(acoust->inBetween.origin.y), round(acoust->inBetween.origin.x) + 10, round(acoust->inBetween.origin.y), V4(1, 1, 1, 1));
    draw_line(image, round(acoust->inBetween.origin.x), round(acoust->inBetween.origin.y),
              round(acoust->inBetween.dynamics.position.x), round(acoust->inBetween.dynamics.position.y), 
              V4(1, 1, 1, 1));
    fill_circle(image, round(acoust->inBetween.dynamics.position.x),
                round(acoust->inBetween.dynamics.position.y), 10, V4(1, 1, 1, 1));
    draw_line(image, round(acoust->inBetween.origin.x), round(acoust->inBetween.origin.y + 2*acoust->inBetween.restLength),
              round(acoust->inBetween.dynamics.position.x), round(acoust->inBetween.dynamics.position.y), 
              V4(1, 1, 1, 1));
    draw_line(image, round(acoust->inBetween.origin.x) - 10, round(acoust->inBetween.origin.y + 2*acoust->inBetween.restLength), round(acoust->inBetween.origin.x) + 10, round(acoust->inBetween.origin.y + 2*acoust->inBetween.restLength), V4(1, 1, 1, 1));
    
    acoust->prevMouseDown = mouse.mouseDowns;
    acoust->seconds += dt;
    ++acoust->ticks;
    if (acoust->seconds > 1.0f)
    {
        acoust->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", acoust->ticks,
                1000.0f / (f32)acoust->ticks);
        acoust->ticks = 0;
    }
}
