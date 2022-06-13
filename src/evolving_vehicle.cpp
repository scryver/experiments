#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "forces.h"
#include "vehicle.h"
#include "../libberdip/drawing.cpp"

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
    f32 dna[4];
};

struct EvolveSettings
{
    f32 border;

    f32 mutationRate;     //   0.01f  percentage
    f32 mutationFactor;   //   0.10f  +/- dna chance

    f32 foodWeight;       //   5.00f
    f32 foodHealth;       //   0.20f  per eating
    f32 foodPerception;   // 100.00f  pixels

    f32 poisonWeight;     //   5.00f
    f32 poisonHealth;     //  -0.50f  per eating
    f32 poisonPerception; // 100.00f  pixels

    f32 healthDrop;       //   0.1f   per second
    f32 cloneRate;        //   0.005f percentage
    f32 foodSpawnRate;    //   0.05f  percentage
    f32 poisonSpawnRate;  //   0.01f  percentage
};

struct EvolveState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;

    EvolveSettings settings;

    u32 maxVehicleCount;
    u32 vehicleCount;
    EvolveVehicle *currentVehicles;
    EvolveVehicle *nextGeneration;

    WorldFood noms;
    WorldFood poison;

    b32 debug;
    u32 prevMouseDown;
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
    vehicle->dna[0] = random_unilateral(random); // NOTE(michiel): Food weight
    vehicle->dna[1] = random_unilateral(random); // NOTE(michiel): Poison weight
    vehicle->dna[2] = random_unilateral(random); // NOTE(michiel): Food perception
    vehicle->dna[3] = random_unilateral(random); // NOTE(michiel): Poison perception
}

internal inline f32
food_weight(EvolveSettings *settings, EvolveVehicle *vehicle)
{
    return vehicle->dna[0] * 2.0f * settings->foodWeight - settings->foodWeight;
}

internal inline f32
poison_weight(EvolveSettings *settings, EvolveVehicle *vehicle)
{
    return vehicle->dna[1] * 2.0f * settings->poisonWeight - settings->poisonWeight;
}

internal inline f32
food_perception(EvolveSettings *settings, EvolveVehicle *vehicle)
{
    return vehicle->dna[2] * settings->foodPerception;
}

internal inline f32
poison_perception(EvolveSettings *settings, EvolveVehicle *vehicle)
{
    return vehicle->dna[3] * settings->poisonPerception;
}

internal void
clone_vehicle(RandomSeriesPCG *random, EvolveSettings *settings,
              EvolveVehicle *source, EvolveVehicle *dest)
{
    dest->mover = source->mover;
    dest->maxForce = source->maxForce;
    dest->maxSpeed = source->maxSpeed;
    dest->health = 1.0f;
    dest->dnaCount = source->dnaCount;
    for (u32 dnaIndex = 0; dnaIndex < source->dnaCount; ++dnaIndex)
    {
        dest->dna[dnaIndex] = source->dna[dnaIndex];
        if (random_unilateral(random) < settings->mutationRate)
        {
            dest->dna[dnaIndex] += random_bilateral(random) * settings->mutationFactor;
        }
    }
}

internal v2
seperate(u32 vehicleIndex, EvolveVehicle *vehicle,
         u32 neighbourCount, EvolveVehicle *neighbours)
{
    v2 seperationForce = {};

    f32 oneOverNeighbours = 1.0f / (f32)neighbourCount;
    for (u32 neighbourIndex = 0; neighbourIndex < neighbourCount; ++neighbourIndex)
    {
        if (neighbourIndex != vehicleIndex)
        {
            EvolveVehicle *neighbour = neighbours + neighbourIndex;
            v2 diff = vehicle->mover.position - neighbour->mover.position;
            if (length_squared(diff) < square(40.0f))
            {
                v2 force = normalize(diff);
                force *= oneOverNeighbours;
                seperationForce += force;
            }
        }
    }

    return seperationForce;
}

internal v2
eat(EvolveVehicle *vehicle, WorldFood *food, f32 healthMod = 0.0f, f32 perceptionRadius = 100.0f, f32 dt = 1.0f)
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
        if (dirDistSqr < square(4.0f))
        {
            *current = food->foodies[--food->foodCount];
            vehicle->health += healthMod;
            if (vehicle->health > 1.0f)
            {
                vehicle->health = 1.0f;
            }
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
behaviour(EvolveSettings *settings, EvolveVehicle *vehicle,
          WorldFood *food, WorldFood *poison, f32 dt)
{
    v2 foodSteer = eat(vehicle, food, settings->foodHealth,
                       food_perception(settings, vehicle), dt);
    v2 poisonSteer = eat(vehicle, poison, settings->poisonHealth,
                         poison_perception(settings, vehicle), dt);

    foodSteer *= food_weight(settings, vehicle);
    poisonSteer *= poison_weight(settings, vehicle);

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

        evolveState->settings.border = border;
        evolveState->settings.mutationRate = 0.01f;
        evolveState->settings.mutationFactor = 0.1f;
        evolveState->settings.foodWeight = 5.0f;
        evolveState->settings.foodHealth = 0.1f;
        evolveState->settings.foodPerception = 100.0f;
        evolveState->settings.poisonWeight = 5.0f;
        evolveState->settings.poisonHealth = -0.75f;
        evolveState->settings.poisonPerception = 100.0f;
        evolveState->settings.healthDrop = 0.1f;
        evolveState->settings.cloneRate = 0.005f;
        evolveState->settings.foodSpawnRate = 0.05f;
        evolveState->settings.poisonSpawnRate = 0.01f;

        evolveState->vehicleCount = evolveState->maxVehicleCount = 50;
        evolveState->currentVehicles = arena_allocate_array(gMemoryArena, EvolveVehicle, evolveState->vehicleCount, default_memory_alloc());
        //evolveState->nextGeneration = allocate_array(EvolveVehicle, evolveState->vehicleCount);

        for (u32 vehicleIndex = 0; vehicleIndex < evolveState->vehicleCount; ++vehicleIndex)
        {
            EvolveVehicle *vehicle = evolveState->currentVehicles + vehicleIndex;
            init_vehicle(&evolveState->randomizer, vehicle,
                         random_in_bounds(&evolveState->randomizer, minBound + 2, maxBound - 2),
                         200.0f, 100.0f);
        }

        evolveState->noms.maxFoodCount = evolveState->noms.foodCount = 200;
        evolveState->noms.foodies = arena_allocate_array(gMemoryArena, Food, evolveState->noms.maxFoodCount, default_memory_alloc());
        evolveState->noms.foodCount = 40;

        for (u32 foodIndex = 0; foodIndex < evolveState->noms.foodCount; ++foodIndex)
        {
            Food *food = evolveState->noms.foodies + foodIndex;
            init_food(food, random_in_bounds(&evolveState->randomizer, minBound, maxBound));
        }

        evolveState->poison.maxFoodCount = 200;
        evolveState->poison.foodies = arena_allocate_array(gMemoryArena, Food, evolveState->poison.maxFoodCount, default_memory_alloc());
        evolveState->poison.foodCount = 10;

        for (u32 foodIndex = 0; foodIndex < evolveState->poison.foodCount; ++foodIndex)
        {
            Food *food = evolveState->poison.foodies + foodIndex;
            init_food(food, random_in_bounds(&evolveState->randomizer, minBound, maxBound));
        }

        state->initialized = true;
    }

    EvolveSettings *settings = &evolveState->settings;

    //v2 center = V2((f32)image->width * 0.5f, (f32)image->height * 0.5f);

    for (s32 vehicleIndex = evolveState->vehicleCount - 1;
         vehicleIndex >= 0;
         --vehicleIndex)
    {
        EvolveVehicle *vehicle = evolveState->currentVehicles + vehicleIndex;
        v2 boundaryForce = within_boundary_force(&vehicle->mover, minBound, maxBound,
                                                 vehicle->maxSpeed, vehicle->maxForce);
        apply_force(&vehicle->mover, 20.0f * boundaryForce);
        seperate(vehicleIndex, vehicle, evolveState->vehicleCount,
                 evolveState->currentVehicles);
        behaviour(settings, vehicle, &evolveState->noms, &evolveState->poison, dt);
        update(&vehicle->mover, dt, vehicle->maxSpeed);

        // NOTE(michiel): Random clone
        if ((evolveState->vehicleCount < evolveState->maxVehicleCount) &&
            (random_unilateral(&evolveState->randomizer) < settings->cloneRate))
        {
            EvolveVehicle *cloneVehicle = evolveState->currentVehicles + evolveState->vehicleCount++;
            clone_vehicle(&evolveState->randomizer, settings, vehicle, cloneVehicle);
        }

        vehicle->health -= settings->healthDrop * dt;
        if (vehicle->health <= 0.0f)
        {
            if (evolveState->noms.foodCount < evolveState->noms.maxFoodCount)
            {
                Food *food = evolveState->noms.foodies + evolveState->noms.foodCount++;
                init_food(food, vehicle->mover.position);
            }
            *vehicle = evolveState->currentVehicles[--evolveState->vehicleCount];
        }
    }

    {
        // NOTE(michiel): Random add food
        if ((evolveState->noms.foodCount < evolveState->noms.maxFoodCount) &&
            (random_unilateral(&evolveState->randomizer) < settings->foodSpawnRate))
        {
            Food *food = evolveState->noms.foodies + evolveState->noms.foodCount++;
            init_food(food, random_in_bounds(&evolveState->randomizer, minBound, maxBound));
        }

        // NOTE(michiel): Random add poison
        if ((evolveState->poison.foodCount < evolveState->poison.maxFoodCount) &&
            (random_unilateral(&evolveState->randomizer) < settings->poisonSpawnRate))
        {
            Food *food = evolveState->poison.foodies + evolveState->poison.foodCount++;
            init_food(food, random_in_bounds(&evolveState->randomizer, minBound, maxBound));
        }
    }

    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));

    v4 foodColour = V4(0, 1, 0, 1);
    v4 poisonColour = V4(0.6f, 0, 0.5f, 1);

    for (u32 vehicleIndex = 0; vehicleIndex < evolveState->vehicleCount; ++vehicleIndex)
    {
        EvolveVehicle *vehicle = evolveState->currentVehicles + vehicleIndex;

        v2 dir = vehicle->mover.velocity;
        dir = normalize(dir);

        v2 front = V2(10, 0);
        v2 backUp = V2(-8, 5);
        v2 backDo = V2(-8, -5);
        v2 good = front * food_weight(settings, vehicle);
        v2 bad  = front * poison_weight(settings, vehicle);

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

        if (evolveState->debug)
        {
            draw_line(image, vehicle->mover.position.x + 1, vehicle->mover.position.y + 1,
                      good.x, good.y, foodColour);
            draw_line(image, vehicle->mover.position.x - 1, vehicle->mover.position.y - 1,
                      bad.x, bad.y, poisonColour);
            outline_circle(image, vehicle->mover.position.x, vehicle->mover.position.y,
                           food_perception(settings, vehicle), 2.0f, foodColour);
            outline_circle(image, vehicle->mover.position.x, vehicle->mover.position.y,
                           poison_perception(settings, vehicle), 2.0f, poisonColour);
        }

        v4 healthy = V4(1, 1, 0, 1);
        v4 dead = V4(0.2f, 0, 0, 1);
        v4 colour = lerp(dead, vehicle->health, healthy);
        fill_triangle(image, front, backUp, backDo, colour);

    }

    for (u32 foodIndex = 0; foodIndex < evolveState->noms.foodCount; ++foodIndex)
    {
        Food *food = evolveState->noms.foodies + foodIndex;
        fill_rectangle(image, food->position.x - 2, food->position.y - 2, 4, 4,
                       foodColour);
    }

    for (u32 foodIndex = 0; foodIndex < evolveState->poison.foodCount; ++foodIndex)
    {
        Food *food = evolveState->poison.foodies + foodIndex;
        fill_rectangle(image, food->position.x - 2, food->position.y - 2, 4, 4,
                       poisonColour);
    }

    v2 boxAt = V2(10, 10);
    v2 boxSize = V2(10, 10);
    fill_rectangle(image, boxAt.x, boxAt.y, boxSize.x, boxSize.y, V4(1, 1, 1, 1));
    if (evolveState->debug)
    {
        fill_rectangle(image, boxAt.x + 2, boxAt.y + 2, boxSize.x - 4, boxSize.y - 4,
                       V4(0, 0, 0, 1));
    }
    if ((boxAt.x <= (f32)mouse.pixelPosition.x) && ((f32)mouse.pixelPosition.x < (boxAt.x + boxSize.x)) &&
        (boxAt.y <= (f32)mouse.pixelPosition.y) && ((f32)mouse.pixelPosition.y < (boxAt.y + boxSize.y)))
    {
        fill_rectangle(image, boxAt.x + 1, boxAt.y + 1, boxSize.x - 2, boxSize.y - 2,
                       V4(0, 0, 1, 0.5f));
        if (is_pressed(&mouse, Mouse_Left))
        {
            evolveState->debug = !evolveState->debug;
        }
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
