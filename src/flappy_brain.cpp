#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/std_file.c"

#include "aitraining.h"
#include "neurons.h"

#include "../libberdip/drawing.cpp"

struct Bird
{
    v2 position;
    v2 velocity;
    v2 acceleration;
    
    f32 radius;
    b32 alive;
    
    f32 score;
    f32 fitness;
    Neural brain;
    // NOTE(michiel): Inputs:
    // 0: position y
    // 1: velocity y
    // 2-6-10: closest pipes min x
    // 3-7-11: closest pipes max x
    // 4-8-12: closest pipes gap top
    // 5-9-13: closest pipes gap bottom
    //
    // NOTE(michiel): Outputs:
    // 0: jump (> 0.5)
};

struct Pipe
{
    v2 gapMin;
    v2 gapMax;
    
    b32 active;
};

struct FlappyState
{
    Arena arena;
    
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    
    u32 cycles;
    
    v2 gravity;
    
    u32 populationCount;
    Bird birds[4096];
    u32 pipeCount;
    Pipe pipes[16];
    
    u32 generation;
    u32 distance;
    
    b32 sliding;
    u32 prevMouseDown;
};

internal inline void
init_pipe(FlappyState *state, v2 screenSize, f32 gapSize = 100.0f)
{
    i_expect(state->pipeCount < 16);
    Pipe *pipe = state->pipes + state->pipeCount++;
    state->pipeCount &= 15;
    
    pipe->gapMin.x = screenSize.x;
    pipe->gapMax.x = pipe->gapMin.x + 50.0f;
    pipe->gapMin.y = random_choice(&state->randomizer, round(screenSize.y * 0.5f));
    pipe->gapMax.y = pipe->gapMin.y + gapSize;
    
    pipe->active = true;
}

internal inline void
update_pipe(Pipe *pipe, f32 dt)
{
    pipe->gapMin.x += -100.0f * dt;
    pipe->gapMax.x += -100.0f * dt;
    
    if (pipe->gapMax.x < 0.0f)
    {
        pipe->active = false;
    }
}

internal inline void
init_bird(RandomSeriesPCG *random, Bird *bird, v2 screenSize, Neural *brainToCopy = 0)
{
    bird->position.x = 50.0f;
    bird->position.y = screenSize.y * 0.5f;
    bird->radius = 8.0f;
    bird->alive = true;
    bird->score = 0.1f;
    
    if (brainToCopy)
    {
        neural_copy(brainToCopy, &bird->brain);
    }
    else
    {
        u32 hiddenCounts[] = {100};
        init_neural_network(&bird->brain, 14, array_count(hiddenCounts), hiddenCounts, 2);
        randomize_weights(random, &bird->brain);
    }
}

internal void
thinking(Bird *bird, u32 pipeCount, Pipe **pipes, v2 oneOverScreenSize)
{
    f32 inputs[14] = {};
    inputs[0] = bird->position.y * oneOverScreenSize.y;
    inputs[1] = bird->velocity.y * oneOverScreenSize.y;
    
    i_expect(bird->brain.inputCount == (pipeCount * 4) + 2);
    
    u32 index = 2;
    for (u32 pipeIndex = 0; pipeIndex < pipeCount; ++pipeIndex)
    {
        // TODO(michiel): I don't know about this double pointer thingie, feels weird
        Pipe *pipe = pipes[pipeIndex];
        if (pipe)
        {
            inputs[index++] = pipe->gapMin.x * oneOverScreenSize.x;
            inputs[index++] = pipe->gapMax.x * oneOverScreenSize.x;
            inputs[index++] = pipe->gapMin.y * oneOverScreenSize.y;
            inputs[index++] = pipe->gapMax.y * oneOverScreenSize.y;
        }
        else
        {
            inputs[index++] = -1.0f;
            inputs[index++] = -1.0f;
            inputs[index++] = -1.0f;
            inputs[index++] = -1.0f;
        }
    }
    
    predict(&bird->brain, array_count(inputs), inputs);
    
    if (bird->brain.outputs[0] > bird->brain.outputs[1])
    {
        bird->acceleration = V2(0, -3000.0f);
    }
    else
    {
        bird->acceleration = V2(0, 0);
    }
}

internal inline void
update_bird(Bird *bird, v2 force, f32 dt)
{
    bird->score *= 1.1f;
    bird->velocity += (bird->acceleration + force) * dt;
    bird->velocity *= 0.99f;
    bird->velocity.x = minimum(maximum(bird->velocity.x, -500.0f), 500.0f);
    bird->velocity.y = minimum(maximum(bird->velocity.y, -500.0f), 500.0f);
    bird->position += bird->velocity * dt;
}

internal inline void
check_borders(Bird *bird, v2 minBorder, v2 maxBorder)
{
    if (bird->position.x < (minBorder.x + bird->radius))
    {
        bird->position.x = minBorder.x + bird->radius;
    }
    if (bird->position.y < (minBorder.y + bird->radius))
    {
        bird->position.y = minBorder.y + bird->radius;
    }
    if (bird->position.x > (maxBorder.x - bird->radius))
    {
        bird->position.x = maxBorder.x - bird->radius;
    }
    if (bird->position.y > (maxBorder.y - bird->radius))
    {
        bird->position.y = maxBorder.y - bird->radius;
    }
}

internal inline b32
bird_hit_pipe(Bird *bird, Pipe *pipe)
{
    b32 result = false;
    
    if ((bird->position.x >= (pipe->gapMin.x - bird->radius)) &&
        (bird->position.x <= (pipe->gapMax.x + bird->radius)) &&
        ((bird->position.y <= (pipe->gapMin.y + bird->radius)) ||
         (bird->position.y >= (pipe->gapMax.y - bird->radius))))
    {
        result = true;
    }
    
    return result;
}

internal inline void
draw_bird(Image *image, Bird *bird)
{
    fill_circle(image, bird->position.x, bird->position.y, bird->radius,
                V4(1, 1, 1, 0.3f));
}

internal inline void
draw_pipe(Image *image, Pipe *pipe, v4 colour)
{
    fill_rectangle(image, pipe->gapMin.x, 0, pipe->gapMax.x - pipe->gapMin.x, pipe->gapMin.y,
                   colour);
    fill_rectangle(image, pipe->gapMin.x, pipe->gapMax.y, pipe->gapMax.x - pipe->gapMin.x,
                   image->height - pipe->gapMax.y, colour);
}

internal void
calculate_fitness(u32 birdCount, Bird *birds)
{
    f32 sum = 0.0f;
    for (u32 birdIndex = 0; birdIndex < birdCount; ++birdIndex)
    {
        Bird *bird = birds + birdIndex;
        sum += bird->score;
    }
    f32 oneOverSum = 1.0f / sum;
    for (u32 birdIndex = 0; birdIndex < birdCount; ++birdIndex)
    {
        Bird *bird = birds + birdIndex;
        bird->fitness = bird->score * oneOverSum;
    }
}

internal Bird *
pick_one(RandomSeriesPCG *random, u32 birdCount, Bird *parentBirds)
{
    s32 index = 0;
    
    f32 r = random_unilateral(random);
    
    while ((r > 0.0f) && (index < birdCount))
    {
        r = r - parentBirds[index].fitness;
        ++index;
    }
    return parentBirds + index - 1;
}

NEURON_MUTATE(mutation)
{
    RandomSeriesPCG *random = (RandomSeriesPCG *)user;
    if (random_unilateral(random) < 0.04f)
    {
        f32 offset = slow_gaussian(random) * 0.4f;
        return a + offset;
    }
    else
    {
        return a;
    }
}

internal void
next_generation(Arena *arena, RandomSeriesPCG *random, u32 birdCount, Bird *parentBirds,
                v2 screenSize, f32 mutationRate, f32 mutationFactor)
{
    TempMemory temp = temporary_memory(arena);
    
    calculate_fitness(birdCount, parentBirds);
    
    Bird *nextGen = arena_allocate_array(arena, Bird, birdCount);
    Bird *best = 0;
    
    for (u32 birdIndex = 0; birdIndex < birdCount; ++birdIndex)
    {
        Bird *bird = parentBirds + birdIndex;
        if (!best ||
            (best->score < bird->score))
        {
            best = bird;
        }
    }
    i_expect(best);
    
    for (u32 birdIndex = 0; birdIndex < birdCount; ++birdIndex)
    {
        Bird *bird = nextGen + birdIndex;
        init_neural_network(&bird->brain, best->brain.inputCount, best->brain.layerCount - 1,
                            best->brain.layerSizes, best->brain.outputCount);
        
        if (random_unilateral(random) < 0.2f)
        {
            neural_copy(&best->brain, &bird->brain);
        }
        else
        {
            Bird *parentA = pick_one(random, birdCount, parentBirds);
            Bird *parentB = pick_one(random, birdCount, parentBirds);
            
            neural_copy(&parentA->brain, &bird->brain);
            if (random_unilateral(random) < 0.2f)
            {
                neural_mixin(random, &best->brain, &bird->brain);
            }
            else if (random_unilateral(random) < 0.7f)
            {
                neural_mixin(random, &parentB->brain, &bird->brain);
            }
        }
        neural_mutate(random, &bird->brain, mutation, random);
    }
    
    for (u32 birdIndex = 0; birdIndex < birdCount; ++birdIndex)
    {
        Bird *source = nextGen + birdIndex;
        Bird *bird = parentBirds + birdIndex;
        
        init_bird(random, bird, screenSize, &source->brain);
        destroy_neural_network(&source->brain);
    }
    
    destroy_temporary(temp);
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(FlappyState) <= state->memorySize);
    
    v2 size = V2((f32)image->width, (f32)image->height);
    
    FlappyState *flappy = (FlappyState *)state->memory;
    if (!state->initialized)
    {
        // flappy->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        flappy->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        // NOTE(michiel): Height = 10 meter
        flappy->gravity.y = 9.81f * (f32)image->height / 10.0f;
        
        for (u32 birdIndex = 0; birdIndex < array_count(flappy->birds); ++birdIndex)
        {
            Bird *bird = flappy->birds + birdIndex;
            
            init_bird(&flappy->randomizer, bird, size);
        }
        
        flappy->cycles = 10;
        
        state->initialized = true;
    }
    
    for (u32 cycle = 0; cycle < flappy->cycles; ++cycle)
    {
        f32 innerDt = 0.01f; // dt / (f32)flappy->cycles;
        u32 maxX = 0;
        for (u32 pipe = 0; pipe < 16; ++pipe)
        {
            Pipe *p = flappy->pipes + pipe;
            if (p->active)
            {
                update_pipe(p, innerDt);
                
                if (maxX < round(p->gapMax.x))
                {
                    maxX = round(p->gapMax.x);
                }
            }
        }
        
        if (//(random_unilateral(&flappy->randomizer) < 0.005f) &&
            (maxX < image->width - 150.0f))
        {
            init_pipe(flappy, size, 120.0f);
        }
        
        Pipe *closestPipes[3] = {};
        f32 birdMinX = flappy->birds[0].position.x - flappy->birds[0].radius;
        for (u32 pipeIndex = 0; pipeIndex < array_count(flappy->pipes); ++pipeIndex)
        {
            Pipe *pipe = flappy->pipes + pipeIndex;
            if ((pipe->active) &&
                (pipe->gapMax.x >= birdMinX))
            {
                if (!closestPipes[0])
                {
                    closestPipes[0] = pipe;
                }
                else if (!closestPipes[1])
                {
                    if (closestPipes[0]->gapMin.x > pipe->gapMin.x)
                    {
                        closestPipes[1] = closestPipes[0];
                        closestPipes[0] = pipe;
                    }
                    else
                    {
                        closestPipes[1] = pipe;
                    }
                }
                else if (!closestPipes[2])
                {
                    if (closestPipes[0]->gapMin.x > pipe->gapMin.x)
                    {
                        closestPipes[2] = closestPipes[1];
                        closestPipes[1] = closestPipes[0];
                        closestPipes[0] = pipe;
                    }
                    else if (closestPipes[1]->gapMin.x > pipe->gapMin.x)
                    {
                        closestPipes[2] = closestPipes[1];
                        closestPipes[1] = pipe;
                    }
                    else
                    {
                        closestPipes[2] = pipe;
                    }
                }
                else if (closestPipes[0]->gapMin.x > pipe->gapMin.x)
                {
                    closestPipes[2] = closestPipes[1];
                    closestPipes[1] = closestPipes[0];
                    closestPipes[0] = pipe;
                }
                else if (closestPipes[1]->gapMin.x > pipe->gapMin.x)
                {
                    closestPipes[2] = closestPipes[1];
                    closestPipes[1] = pipe;
                }
                else if (closestPipes[2]->gapMin.x > pipe->gapMin.x)
                {
                    closestPipes[2] = pipe;
                }
            }
        }
        
        u32 aliveBirds = 0;
        v2 oneOverSize = 1.0f / size;
        for (u32 birdIndex = 0; birdIndex < array_count(flappy->birds); ++birdIndex)
        {
            Bird *bird = flappy->birds + birdIndex;
            for (u32 pipeIndex = 0; pipeIndex < array_count(flappy->pipes); ++pipeIndex)
            {
                Pipe *pipe = flappy->pipes + pipeIndex;
                if (pipe->active)
                {
                    if (bird_hit_pipe(bird, pipe))
                    {
                        bird->alive = false;
                        break;
                    }
                }
            }
            
            if (bird->alive)
            {
                thinking(bird, array_count(closestPipes), closestPipes, oneOverSize);
                update_bird(bird, flappy->gravity, innerDt);
                check_borders(bird, V2(0, 0), size);
            }
            
            aliveBirds += bird->alive ? 1 : 0;
        }
        
        ++flappy->distance;
        if (aliveBirds == 0)
        {
            ++flappy->generation;
            fprintf(stdout, "Generation %5u: Best: %u\n", flappy->generation, flappy->distance);
            flappy->distance = 0;
            
            next_generation(&flappy->arena, &flappy->randomizer, 
                            array_count(flappy->birds), flappy->birds, size, 0.1f, 0.1f);
            for (u32 pipeIndex = 0; pipeIndex < array_count(flappy->pipes); ++pipeIndex)
            {
                Pipe *pipe = flappy->pipes + pipeIndex;
                pipe->active = false;
            }
            //flappy->pipeCount = 0;
        }
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    for (u32 pipe = 0; pipe < 16; ++pipe)
    {
        Pipe *p = flappy->pipes + pipe;
        v4 colour = V4(0.4f, 0.5f, 0.1f, 1.0f);
        if (p->active)
        {
            draw_pipe(image, p, colour);
        }
    }
    
    for (u32 birdIndex = 0, drawn = 0;
         (birdIndex < array_count(flappy->birds)) && (drawn < 100); 
         ++birdIndex)
    {
        Bird *bird = flappy->birds + birdIndex;
        if (bird->alive)
        {
            draw_bird(image, bird);
            ++drawn;
        }
    }
    
    v2u sliderAt = V2U(196, image->height - 40);
    v2u sliderSize = V2U(image->width - 400, 12);
    
    f32 sliderBob = (f32)flappy->cycles / 200.0f;
    v2 sliderHandle = V2(sliderAt.x + round(sliderBob * (f32)sliderSize.x),
                         sliderAt.y + sliderSize.y / 2 - 1);
    f32 sliderRadius = (f32)sliderSize.y * 0.5f + 2.0f;
    
    // TODO(michiel): Check on distance (for roundness (: )
    if (((f32)mouse.pixelPosition.x >= (sliderHandle.x - sliderRadius)) &&
        ((f32)mouse.pixelPosition.x <= (sliderHandle.x + sliderRadius)) &&
        ((f32)mouse.pixelPosition.y >= (sliderHandle.y - sliderRadius)) &&
        ((f32)mouse.pixelPosition.y <= (sliderHandle.y + sliderRadius)))
    {
        // NOTE(michiel): Mouse is inside handle
        if ((mouse.mouseDowns & Mouse_Left) &&
            !(flappy->prevMouseDown & Mouse_Left))
        {
            flappy->sliding = true;
        }
    }
    
    if (flappy->sliding)
    {
        f32 mouseAt = (f32)mouse.pixelPosition.x;
        mouseAt -= sliderAt.x;
        if (mouseAt < 0.0f)
        {
            mouseAt = 0.0f;
        }
        else if (mouseAt > sliderSize.x)
        {
            mouseAt = sliderSize.x;
        }
        
        flappy->cycles = round((mouseAt / sliderSize.x) * 200.0f);
        
        if (!(mouse.mouseDowns & Mouse_Left) &&
            (flappy->prevMouseDown & Mouse_Left))
        {
            flappy->sliding = false;
        }
    }
    
    sliderBob = (f32)flappy->cycles / 200.0f;
    sliderHandle = V2(sliderAt.x + round(sliderBob * (f32)sliderSize.x),
                      sliderAt.y + sliderSize.y / 2 - 1);
    sliderRadius = (f32)sliderSize.y * 0.5f + 2.0f;
    
    fill_rectangle(image, sliderAt.x - 4, sliderAt.y - 4, sliderSize.x + 8, sliderSize.y + 8,
                   V4(0.2f, 0.2f, 0.2f, 1.0f));
    fill_rectangle(image, sliderAt.x, sliderAt.y, sliderSize.x, sliderSize.y,
                   V4(0.4f, 0.4f, 0.4f, 1.0f));
    fill_circle(image, round(sliderHandle.x), round(sliderHandle.y),round(sliderRadius),
                V4(0.7f, 0.7f, 0.7f, 1.0f));
    
    flappy->prevMouseDown = mouse.mouseDowns;
    flappy->seconds += dt;
    ++flappy->ticks;
}
