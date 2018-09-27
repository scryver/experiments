#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "random.h"
#include "forces.h"
#include "vehicle.h"
#include "drawing.cpp"

struct Food
{
    v2 position;
};

struct WorldFood
{
    u32 maxFoodCount;
    u32 foodCount;
    Food *foodies;
};

struct EvolveState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    u32 vehicleCount;
    Vehicle *vehicles;
    
    WorldFood noms;
};

internal inline void
init_food(Food *food, v2 position)
{
    food->position = position;
}

internal inline void
eat(Vehicle *vehicle, WorldFood *food)
{
    f32 recordSqr = F32_INF;
     s32 closestIndex = -1;
    
    for (u32 foodIndex = 0; foodIndex < food->foodCount; ++foodIndex)
    {
        Food *current = food->foodies + foodIndex;
        
    v2 direction = current->position - vehicle->mover.position;
        if (recordSqr > length_squared(direction))
        {
            recordSqr = length_squared(direction);
            closestIndex = foodIndex;
        }
    }
    
    if (closestIndex >= 0)
    {
        seek(vehicle, food->foodies[closestIndex].position);
        
        if (recordSqr < square(5.0f))
        {
            food->foodies[closestIndex] = food->foodies[--food->foodCount];
        }
    }
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(EvolveState) <= state->memorySize);
    EvolveState *evolveState = (EvolveState *)state->memory;
    if (!state->initialized)
    {
        //evolveState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        evolveState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        evolveState->vehicleCount = 1;
        evolveState->vehicles = allocate_array(Vehicle, evolveState->vehicleCount);
        
        for (u32 vehicleIndex = 0; vehicleIndex < evolveState->vehicleCount; ++vehicleIndex)
        {
            Vehicle *vehicle = evolveState->vehicles + vehicleIndex;
            init_vehicle(vehicle, V2(random_choice(&evolveState->randomizer, image->width), 
                                     random_choice(&evolveState->randomizer, image->height)),
                         3.0f, 0.2f);
        }
        
        evolveState->noms.maxFoodCount = evolveState->noms.foodCount = 10;
        evolveState->noms.foodies = allocate_array(Food, evolveState->noms.maxFoodCount);
        
        for (u32 foodIndex = 0; foodIndex < evolveState->noms.foodCount; ++foodIndex)
        {
            Food *food = evolveState->noms.foodies + foodIndex;
            init_food(food, V2(random_choice(&evolveState->randomizer, image->width), 
                                     random_choice(&evolveState->randomizer, image->height)));
        }
        
        state->initialized = true;
    }
    
    //v2 center = V2((f32)image->width * 0.5f, (f32)image->height * 0.5f);
    
    for (u32 vehicleIndex = 0; vehicleIndex < evolveState->vehicleCount; ++vehicleIndex)
    {
        Vehicle *vehicle = evolveState->vehicles + vehicleIndex;
        eat(vehicle, &evolveState->noms);
        update(&vehicle->mover, dt * 100.0f);
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    for (u32 vehicleIndex = 0; vehicleIndex < evolveState->vehicleCount; ++vehicleIndex)
    {
        Vehicle *vehicle = evolveState->vehicles + vehicleIndex;
        
        v2 dir = vehicle->mover.velocity;
        dir = normalize(dir);
        
        v2 front = V2(10, 0);
        v2 backUp = V2(-8, 5);
        v2 backDo = V2(-8, -5);
        
        if (length_squared(dir))
        {
            front = rotate(front, dir);
            backUp = rotate(backUp, dir);
            backDo = rotate(backDo, dir);
        }
        
        fill_triangle(image, front + vehicle->mover.position, backUp + vehicle->mover.position,
                      backDo + vehicle->mover.position, V4(1, 1, 0, 1));
    }
    
    for (u32 foodIndex = 0; foodIndex < evolveState->noms.foodCount; ++foodIndex)
    {
        Food *food = evolveState->noms.foodies + foodIndex;
        fill_rectangle(image, food->position.x - 5, food->position.y - 5, 10, 10,
                       V4(0, 1, 0, 1));
    }
    
    ++evolveState->ticks;
}
