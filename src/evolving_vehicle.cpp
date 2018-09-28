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

struct EvolveVehicle
{
    Mover mover;
    
    f32 maxForce;
    f32 maxSpeed;
    
    f32 health;
    
    u32 dnaCount;
    f32 *dna;
};

struct EvolveState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    
    u32 vehicleCount;
    EvolveVehicle *vehicles;
    
    WorldFood noms;
    WorldFood poison;
};

internal inline void
init_food(Food *food, v2 position)
{
    food->position = position;
}

internal void
init_vehicle(RandomSeriesPCG *random, EvolveVehicle *vehicle, 
             v2 pos, f32 maxSpeed = 4.0f, f32 maxForce = 0.1f)
{
    vehicle->mover = create_mover(pos);
    
    vehicle->maxForce = maxForce;
    vehicle->maxSpeed = maxSpeed;
    vehicle->mover.velocity = V2(random_bilateral(random) * maxSpeed * 0.5f,
                                   random_bilateral(random) * maxSpeed * 0.5f);
    
    vehicle->health = 1.0f;
    
    vehicle->dnaCount = 4;
    vehicle->dna = allocate_array(f32, vehicle->dnaCount);
    
    vehicle->dna[0] = random_bilateral(random) * 5.0f; // NOTE(michiel): Food weight
    vehicle->dna[1] = random_bilateral(random) * 5.0f; // NOTE(michiel): Poison weight
    vehicle->dna[2] = random_unilateral(random) * 100.0f; // NOTE(michiel): Food perception
    vehicle->dna[3] = random_unilateral(random) * 100.0f; // NOTE(michiel): Poison perception
}

internal inline v2
eat(EvolveVehicle *vehicle, WorldFood *food, f32 healthMod = 0.0f, f32 perceptionRadius = 100.0f)
{
    v2 result = {};
    f32 recordSqr = F32_INF;
    f32 maxDistSqr = square(perceptionRadius);
    Food closest = {};
    
    for (u32 foodIndex = 0; foodIndex < food->foodCount;)
    {
        Food *current = food->foodies + foodIndex;
        
    v2 direction = current->position - vehicle->mover.position;
        f32 dirDistSqr = length_squared(direction);
        if (dirDistSqr < square(vehicle->maxSpeed))
        {
             *current = food->foodies[--food->foodCount];
            vehicle->health += healthMod;
        }
        else
        {
            if ((recordSqr > dirDistSqr) &&
                (maxDistSqr > dirDistSqr))
            {
                recordSqr = dirDistSqr;
                closest = *current;
            }
            ++foodIndex;
        }
    }
    
    if (recordSqr < F32_INF)
    {
            result = seeking_force(&vehicle->mover, closest.position, vehicle->maxSpeed,
                               vehicle->maxForce);
        }
    
    return result;
}

internal v2
within_boundary_force(Mover *mover, v2 minBound, v2 maxBound, f32 maxSpeed, f32 maxForce)
{
    v2 desired = {};
    
    if (mover->position.x < minBound.x)
    {
        desired = V2(maxSpeed, mover->velocity.y);
    }
    else if (mover->position.x > maxBound.x)
    {
        desired = V2(-maxSpeed, mover->velocity.y);
    }
    
    if (mover->position.y < minBound.y)
    {
        desired = V2(mover->velocity.x, maxSpeed);
    }
    else if (mover->position.y > maxBound.y)
    {
        desired = V2(mover->velocity.x, -maxSpeed);
    }
    
    if ((desired.x != 0.0f) ||
        (desired.y != 0.0f))
    {
    desired = set_length(desired, maxSpeed);
        desired = desired - mover->velocity;
        if (length_squared(desired) > maxForce)
        {
             desired = set_length(desired, maxForce);
        }
    }
    return desired;
}

internal void
behaviour(EvolveVehicle *vehicle, WorldFood *food, WorldFood *poison)
{
    v2 foodSteer = eat(vehicle, food, 0.2f, vehicle->dna[2]);
    v2 poisonSteer = eat(vehicle, poison, -0.5f, vehicle->dna[3]);
    
    foodSteer *= vehicle->dna[0];
    poisonSteer *= vehicle->dna[1];
    
    apply_force(&vehicle->mover, foodSteer);
    apply_force(&vehicle->mover, poisonSteer);
}

internal v2
random_in_bounds(RandomSeriesPCG *random, v2 minBound, v2 maxBound)
{
    v2 result = {};
    
    v2 size = maxBound - minBound;
    result.x = random_unilateral(random) * size.x + minBound.x;
    result.y = random_unilateral(random) * size.y + minBound.y;
    
    return result;
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(EvolveState) <= state->memorySize);
    EvolveState *evolveState = (EvolveState *)state->memory;
    
    f32 border = 25.0f;
    v2 minBound = V2(border, border);
    v2 maxBound = V2((f32)image->width - 2.0f * border, (f32)image->height - 2.0f * border);
    
    if (!state->initialized)
    {
        //evolveState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        evolveState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        evolveState->vehicleCount = 20;
        evolveState->vehicles = allocate_array(EvolveVehicle, evolveState->vehicleCount);
        
        for (u32 vehicleIndex = 0; vehicleIndex < evolveState->vehicleCount; ++vehicleIndex)
        {
            EvolveVehicle *vehicle = evolveState->vehicles + vehicleIndex;
            init_vehicle(&evolveState->randomizer, vehicle,
                         random_in_bounds(&evolveState->randomizer, minBound + 2, maxBound - 2),
                         4.0f, 0.7f);
        }
        
        evolveState->noms.maxFoodCount = evolveState->noms.foodCount = 50;
        evolveState->noms.foodies = allocate_array(Food, evolveState->noms.maxFoodCount);
        
        for (u32 foodIndex = 0; foodIndex < evolveState->noms.foodCount; ++foodIndex)
        {
            Food *food = evolveState->noms.foodies + foodIndex;
            init_food(food, random_in_bounds(&evolveState->randomizer, minBound, maxBound));
        }
        
        evolveState->poison.maxFoodCount = evolveState->poison.foodCount = 10;
        evolveState->poison.foodies = allocate_array(Food, evolveState->poison.maxFoodCount);
        
        for (u32 foodIndex = 0; foodIndex < evolveState->poison.foodCount; ++foodIndex)
        {
            Food *food = evolveState->poison.foodies + foodIndex;
            init_food(food, random_in_bounds(&evolveState->randomizer, minBound, maxBound));
        }
        
        state->initialized = true;
    }
    
    //v2 center = V2((f32)image->width * 0.5f, (f32)image->height * 0.5f);
    
    for (u32 vehicleIndex = 0; vehicleIndex < evolveState->vehicleCount; )
    {
        EvolveVehicle *vehicle = evolveState->vehicles + vehicleIndex;
        v2 boundaryForce = within_boundary_force(&vehicle->mover, minBound, maxBound,
                                                 vehicle->maxSpeed, vehicle->maxForce);
        apply_force(&vehicle->mover, 20.0f * boundaryForce);
        behaviour(vehicle, &evolveState->noms, &evolveState->poison);
        update(&vehicle->mover, dt, vehicle->maxSpeed);
        
        vehicle->health -= 0.1f * dt;
        if (vehicle->health <= 0.0f)
        {
            *vehicle = evolveState->vehicles[--evolveState->vehicleCount];
        }
        else
        {
            ++vehicleIndex;
        }
    }
    
    {
        // NOTE(michiel): Random add food
        if ((evolveState->noms.foodCount < evolveState->noms.maxFoodCount) && 
            (random_unilateral(&evolveState->randomizer) < 0.05f))
        {
            Food *food = evolveState->noms.foodies + evolveState->noms.foodCount++;
            init_food(food, random_in_bounds(&evolveState->randomizer, minBound, maxBound));
        }
        
        // NOTE(michiel): Random add poison
        if ((evolveState->poison.foodCount < evolveState->poison.maxFoodCount) && 
            (random_unilateral(&evolveState->randomizer) < 0.01f))
        {
            Food *food = evolveState->poison.foodies + evolveState->poison.foodCount++;
            init_food(food, random_in_bounds(&evolveState->randomizer, minBound, maxBound));
        }
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    for (u32 vehicleIndex = 0; vehicleIndex < evolveState->vehicleCount; ++vehicleIndex)
    {
         EvolveVehicle *vehicle = evolveState->vehicles + vehicleIndex;
        
        v2 dir = vehicle->mover.velocity;
        dir = normalize(dir);
        
        v2 front = V2(10, 0);
        v2 backUp = V2(-8, 5);
        v2 backDo = V2(-8, -5);
        v2 good = front * vehicle->dna[0];
        v2 bad  = front * vehicle->dna[1];
        
        if (length_squared(dir))
        {
            front = rotate(front, dir);
            backUp = rotate(backUp, dir);
            backDo = rotate(backDo, dir);
            good = rotate(good, dir);
            bad = rotate(bad, dir);
        }
        
        front += vehicle->mover.position;
         backUp += vehicle->mover.position;
         backDo += vehicle->mover.position;
         good += vehicle->mover.position;
         bad += vehicle->mover.position;
        
        draw_line(image, front.x + 1, front.y + 1, good.x, good.y, V4(0, 1, 0, 1));
        draw_line(image, front.x - 1, front.y - 1, bad.x, bad.y, V4(0.7f, 0, 0.6f, 1));
        fill_circle(image, vehicle->mover.position.x, vehicle->mover.position.y,
                    vehicle->dna[2], V4(0, 1, 0, 0.4f));
        fill_circle(image, vehicle->mover.position.x, vehicle->mover.position.y,
                    vehicle->dna[3], V4(1, 0, 0, 0.4f));
        
        v4 healthy = V4(1, 1, 0, 1);
        v4 dead = V4(0.2f, 0, 0, 1);
        v4 colour = lerp(vehicle->health, dead, healthy);
        fill_triangle(image, front, backUp, backDo, colour);
        
    }
    
    for (u32 foodIndex = 0; foodIndex < evolveState->noms.foodCount; ++foodIndex)
    {
        Food *food = evolveState->noms.foodies + foodIndex;
        fill_rectangle(image, food->position.x - 5, food->position.y - 5, 10, 10,
                       V4(0, 1, 0, 1));
    }
    
    for (u32 foodIndex = 0; foodIndex < evolveState->poison.foodCount; ++foodIndex)
    {
        Food *food = evolveState->poison.foodies + foodIndex;
        fill_rectangle(image, food->position.x - 5, food->position.y - 5, 10, 10,
                       V4(0.7f, 0, 0.6f, 1));
    }
    
    evolveState->seconds += dt;
    ++evolveState->ticks;
    if (evolveState->seconds >= 1.0f)
    {
        fprintf(stdout, "Frames: %d, %5.2fms\n", evolveState->ticks,
                (evolveState->seconds * 1000.0f) / (f32)evolveState->ticks);
        evolveState->seconds = 0.0f;
        evolveState->ticks = 0;
    }
}
