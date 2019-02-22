#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "forces.h"
#include "../libberdip/drawing.cpp"

struct Particle
{
    v2 pos;
    v2 vel;
    v2 acc;
    
    s32 lifeSpan;
};

struct ParticleSystem
{
    RandomSeriesPCG *randomizer;
    v2 origin;
    
    u32 maxParticles;
    u32 particleCount;
    Particle *particles;
};

internal void
init_particle(Particle *particle, v2 position, RandomSeriesPCG *random = 0)
{
    particle->pos = position;
    if (random)
    {
    particle->vel = V2(random_bilateral(random), random_bilateral(random));
        particle->vel *= 5.0f;
    }
    else
    {
        particle->vel = V2(0, 0);
    }
    particle->acc = V2(0, 0);
    particle->lifeSpan = 100;
}

internal inline void
update_particle(Particle *particle)
{
    particle->vel += particle->acc;
    particle->pos += particle->vel;
    particle->acc = V2(0, 0);
    
        particle->lifeSpan -= 1;
}

internal inline b32
particle_is_dead(Particle *particle)
{
    return particle->lifeSpan <= 0.0f;
}

internal inline void
apply_force(Particle *particle, v2 force)
{
    particle->acc += force;
}

internal inline void
render_particle(Image *image, Particle *particle, u32 maxLife = 100)
{
    fill_circle(image, round(particle->pos.x), round(particle->pos.y),
                5, V4(0.5f, 0.5f, 0.5f, (1.0f / (f32)maxLife) * particle->lifeSpan));
}

internal void
init_particle_system(ParticleSystem *system, u32 particleCount, v2 origin,
                     RandomSeriesPCG *randomizer = 0)
{
    system->randomizer = randomizer;
    system->origin = origin;
    
    system->maxParticles = particleCount;
    system->particleCount = 0;
    system->particles = allocate_array(Particle, particleCount);
}

internal inline Particle *
add_particle(ParticleSystem *system)
{
    Particle *particle = 0;
    if (system->particleCount < system->maxParticles)
    {
    particle = system->particles + system->particleCount++;
    init_particle(particle, system->origin, system->randomizer);
    }
    return particle;
}

internal inline void
remove_particle(ParticleSystem *system, u32 index)
{
    i_expect(index < system->maxParticles);
    i_expect(system->particleCount > 0);
    system->particles[index] = system->particles[--system->particleCount];
}

internal inline void
sort_particles(ParticleSystem *system, s32 startIndex, s32 endIndex)
{
    // NOTE(michiel): Oldest particles get moved to end of array
    if (startIndex < endIndex)
    {
         Particle *pivot = system->particles + endIndex;
        u32 index = startIndex;
        for (u32 j = startIndex; j < endIndex - 1; ++j)
        {
            Particle *test = system->particles + j;
            if (test->lifeSpan > pivot->lifeSpan)
            {
                Particle temp = system->particles[index];
                system->particles[index] = *test;
                *test = temp;
                ++index;
            }
        }
        Particle temp = *pivot;
        *pivot = system->particles[index];
        system->particles[index] = temp;
        
        sort_particles(system, startIndex, index - 1);
        sort_particles(system, index + 1, endIndex);
    }
}

internal inline void
apply_force(ParticleSystem *system, v2 force)
{
    for (u32 particleIndex = 0; particleIndex < system->particleCount; ++particleIndex)
    {
        Particle *particle = system->particles + particleIndex;
        apply_force(particle, force);
    }
}

internal void
update_particles(ParticleSystem *system)
{
    for (u32 particleIndex = 0; particleIndex < system->particleCount;)
    {
        Particle *particle = system->particles + particleIndex;
        update_particle(particle);
        if (particle_is_dead(particle))
        {
            remove_particle(system, particleIndex);
        }
        else
        {
            ++particleIndex;
        }
    }
    }

internal void
render_particles(Image *image, ParticleSystem *system)
{
    for (u32 particleIndex = 0; particleIndex < system->particleCount; ++particleIndex)
    {
        Particle *particle = system->particles + particleIndex;
        render_particle(image, particle);
    }
    }

struct ParticleState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    f32 secondsCount;
    
    ParticleSystem system;
};

DRAW_IMAGE(draw_image)
{
    v2 center = V2((f32)image->width * 0.5f, (f32)image->height * 0.5f);
    
    i_expect(sizeof(ParticleState) <= state->memorySize);
    ParticleState *particleState = (ParticleState *)state->memory;
    ParticleSystem *system = &particleState->system;
    
    if (!state->initialized)
    {
        particleState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        
        init_particle_system(system, 8192, V2(center.x, 200), &particleState->randomizer);
        
        state->initialized = true;
    }
    
    particleState->secondsCount += dt;
    
    sort_particles(system, 0, system->particleCount - 1);
    
    s32 removeCount = system->maxParticles - system->particleCount;
    removeCount = maximum(0, 60 - removeCount);
        system->particleCount -= removeCount;
    
    for (u32 i = 0; i < 60; ++i)
    {
        add_particle(system);
    }
    
    v2 gravity = V2(0, 0.07f);
    apply_force(system, gravity);
    
    update_particles(system);
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    render_particles(image, system);
    
    if (particleState->secondsCount >= 1.0f)
    {
        fprintf(stdout, "Frames: %d (%5.2fms)\n", particleState->ticks,
                (f32)particleState->ticks / 1000.0f);
        particleState->ticks = 0;
                particleState->secondsCount -= 1.0f;
    }
    
    ++particleState->ticks;
}
