#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "forces.h"
#include "random.h"
#include "drawing.cpp"

struct PhysicState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    u32 moverCount;
    u32 currentMover;
    Mover movers[8];
};

internal inline void
bound_check(Image *image, Mover *mover)
{
    if (mover->position.x >= (f32)(image->width - 1))
    {
        mover->position.x = (f32)(image->width - 1);
        mover->velocity.x *= -1.0f;
    } 
    else if (mover->position.x < 0.0f)
    {
        mover->position.x = 0.0f;
        mover->velocity.x *= -1.0f;
    }
    if (mover->position.y >= (f32)(image->height - 1))
    {
        mover->position.y = (f32)(image->height - 1);
        mover->velocity.y *= -1.0f;
    } 
    else if (mover->position.y < 0.0f)
    {
        mover->position.y = 0.0f;
        mover->velocity.y *= -1.0f;
    }
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(PhysicState) <= state->memorySize);
    PhysicState *physics = (PhysicState *)state->memory;
    if (!state->initialized)
    {
         physics->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        physics->moverCount = array_count(physics->movers);
        
        for (u32 moverIndex = 0; moverIndex < physics->moverCount; ++moverIndex)
        {
            Mover *mover = physics->movers + moverIndex;
            mover->position.x = random_unilateral(&physics->randomizer) * (f32)image->width;
            mover->position.y = random_unilateral(&physics->randomizer) * (f32)image->height;
            mover->velocity.x = random_bilateral(&physics->randomizer);
            mover->velocity.y = random_bilateral(&physics->randomizer);
            //mover->position.y = (f32)image->height / 2.0f;
            mover->mass = random_unilateral(&physics->randomizer) + 1.0f;
            mover->oneOverMass = 1.0f / mover->mass;
        }
        
        state->initialized = true;
    }
    
    Mover ground = {};
    ground.position.x = image->width / 2;
    ground.position.y = image->height / 2;
    ground.mass = 10.0f;
    ground.oneOverMass = 1.0f / ground.mass;
    
    for (u32 moverIndex = 0; moverIndex < physics->moverCount; ++moverIndex)
    {
        Mover *mover = physics->movers + moverIndex;
    
    //apply_force(mover, V2(0.0f, 0.3f*mover->mass));
    //apply_force(mover, V2(0.2f, 0.0f));
        //apply_friction(mover);
        //apply_drag(mover);
        //apply_gravity(mover, &ground);
        v2 attract = attraction_force(mover, &ground);
        apply_force(mover, attract);
        
        for (u32 nextMover = moverIndex + 1; nextMover < physics->moverCount; ++nextMover)
        {
            apply_gravity(mover, physics->movers + nextMover);
        }
        update(mover);
        bound_check(image, mover);
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    for (u32 moverIndex = 0;
         moverIndex < physics->moverCount;
         ++moverIndex)
    {
        Mover *mover = physics->movers + moverIndex;
        fill_circle(image, mover->position.x, mover->position.y,
                    (u32)(20.0f * mover->mass + 0.5f),
                    V4(0.6f, 0.7f, 0.6f, 1.0f));
    }
    ++physics->ticks;
}
