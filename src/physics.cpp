#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct Mover
{
    v2 acceleration;
    v2 velocity;
    v2 position;
};

struct PhysicState
{
    u32 ticks;
    
    u32 moverCount;
    u32 currentMover;
    Mover *movers;
};

internal inline void
apply_force(Mover *mover, v2 force)
{
    mover->acceleration += force;
}

internal inline void
update(Mover *mover, f32 maxVelocity = 0.0f)
{
    mover->velocity += mover->acceleration;
    mover->position += mover->velocity;
    if (maxVelocity != 0.0f)
    { 
        f32 maxVelSqr = maxVelocity * maxVelocity;
        f32 velMagSqr = length_squared(mover->velocity);
        if (velMagSqr > maxVelSqr)
        {
            f32 scale = maxVelSqr / velMagSqr;
            mover->velocity *= scale;
        }
    }
    mover->acceleration *= 0.0f;
}

internal inline void
bound_check(Image *image, Mover *mover)
{
    if ((mover->position.x < 0.0f) ||
        (mover->position.x >= (f32)image->width - 1.0f))
    {
        if (mover->position.x < 0.0f)
        {
            mover->position.x = 0.0f;
        }
        else
        {
            mover->position.x = (f32)image->width;
        }
        mover->velocity.x = -mover->velocity.x;
    }
    if ((mover->position.y < 0.0f) ||
        (mover->position.y >= (f32)image->height - 1.0f))
    {
        if (mover->position.y < 0.0f)
        {
            mover->position.y = 0.0f;
        }
        else
        {
            mover->position.y = (f32)image->height;
        }
        mover->velocity.y = -mover->velocity.y;
    }
}

DRAW_IMAGE(draw_image)
{
    PhysicState *physics = (PhysicState *)state->memory;
    if (!state->initialized)
    {
        //RandomSeriesPCG randomize = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        physics->moverCount = 32;
        physics->movers = allocate_array(Mover, physics->moverCount);
        physics->movers[0].position.x = (f32)image->width / 2.0f;
        physics->movers[0].position.y = (f32)image->height / 2.0f;
        
        state->initialized = true;
    }
    Mover *mover = physics->movers + physics->currentMover;
    
    v2 diff = mouse.pixelPosition - mover->position;
    f32 lengthDiff = 1.0f / length(diff);
    diff *= 0.1f * lengthDiff;
    
    apply_force(mover, diff);
    update(mover, 5.0f);
    
    physics->currentMover = (physics->currentMover + 1) % physics->moverCount;
    physics->movers[physics->currentMover] = *mover;
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    f32 divider = 1.0f / (2.0f * (f32)physics->moverCount);
    for (u32 currentIndex = physics->moverCount;
         currentIndex > 0;
         --currentIndex)
    {
        u32 moverIndex = (physics->currentMover + currentIndex) % physics->moverCount;
        Mover *mover = physics->movers + moverIndex;
        fill_circle(image, mover->position.x, mover->position.y, 20,
                    V4(0.6f, 0.7f, 0.6f, (f32)currentIndex * divider));
    }
    
    ++physics->ticks;
}
