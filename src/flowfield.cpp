#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "../libberdip/perlin.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "forces.h"
#include "vehicle.h"
#include "flowfield.h"
#include "../libberdip/drawing.cpp"

internal void
init_flow_field(FlowField *field, u32 width, u32 height, u32 tileSize)
{
    field->size = V2U(width / tileSize, height / tileSize);
    field->tileSize = tileSize;
    field->grid = allocate_array(v2, field->size.x * field->size.y);
}

internal void
init_flow_field(FlowField *field, u32 width, u32 height, u32 tileSize,
                RandomSeriesPCG *random)
{
    init_flow_field(field, width, height, tileSize);
    
    for (u32 row = 0; row < field->size.y; ++row)
    {
        for (u32 col = 0; col < field->size.x; ++col)
        {
            f32 theta = random_unilateral(random) * F32_TAU;
            field->grid[row * field->size.x + col] = polar_to_cartesian(1.0f, theta);
        }
    }
}

internal void
update_flow_field(FlowField *field, PerlinNoise *random, f32 z = 0.0f)
{
    for (u32 row = 0; row < field->size.y; ++row)
    {
        for (u32 col = 0; col < field->size.x; ++col)
        {
            f32 theta = perlin_noise(random, V3(col / (f32)field->size.x, 
                                                row / (f32)field->size.y,
                                                z)) * F32_TAU;
            field->grid[row * field->size.x + col] = polar_to_cartesian(1.0f, theta);
        }
    }
}

struct FlowState
{
    RandomSeriesPCG randomizer;
    PerlinNoise perlin;
    u32 ticks;
    
    f32 zMod;
    
    FlowField field;
    
    //Array vehicles;
    u32 vehicleCount;
    Vehicle *vehicles;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(FlowState) <= state->memorySize);
    FlowState *flow = (FlowState *)state->memory;
    if (!state->initialized)
    {
        flow->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        
        init_perlin_noise(&flow->perlin, &flow->randomizer);
        //init_flow_field(&flow->field, image->width, image->height, 20, &flow->randomizer);
        init_flow_field(&flow->field, image->width, image->height, 40);
        
        flow->vehicleCount = 64;
        flow->vehicles = allocate_array(Vehicle, flow->vehicleCount);
        
        for (u32 vehicleIndex = 0; vehicleIndex < flow->vehicleCount; ++vehicleIndex)
        {
            Vehicle *vehicle = flow->vehicles + vehicleIndex;
            init_vehicle(vehicle, V2(random_choice(&flow->randomizer, image->width), 
                                     random_choice(&flow->randomizer, image->height)));
        }
        
        state->initialized = true;
    }
    
    v2 mouseP = mouse.pixelPosition;
    
    update_flow_field(&flow->field, &flow->perlin, flow->zMod);
    
    for (u32 vehicleIndex = 0; vehicleIndex < flow->vehicleCount; ++vehicleIndex)
    {
        Vehicle *vehicle = flow->vehicles + vehicleIndex;
        f32 mouseDiff = length_squared(mouseP - vehicle->mover.position);
        if (mouseDiff < 5000.0f)
        {
            arrive(vehicle, mouseP);
        }
        else
        {
            follow(vehicle, &flow->field);
        }
        update(&vehicle->mover);
        
        
        if (vehicle->mover.position.x < 0)
        {
            vehicle->mover.position.x += image->width;
        }
        if (vehicle->mover.position.x > image->width)
        {
            vehicle->mover.position.x -= image->width;
        }
        if (vehicle->mover.position.y < 0)
        {
            vehicle->mover.position.y += image->height;
        }
        if (vehicle->mover.position.y > image->height)
        {
            vehicle->mover.position.y -= image->height;
        }
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    FlowField *field = &flow->field;
    
    for (u32 row = 0; row < field->size.y; ++row)
    {
        for (u32 col = 0; col < field->size.x; ++col)
        {
            v2 center = V2(col * (f32)field->tileSize, row * (f32)field->tileSize) + 
                V2(0.5f * (f32)field->tileSize, 0.5f * (f32)field->tileSize);
            v2 target = field->grid[row * field->size.x + col] * 0.5f * (f32)field->tileSize + center;
            
            draw_line(image, round(center.x), round(center.y), 
                      round(target.x), round(target.y), V4(0.7f,0.7f,0.7f,1.0f));
            fill_circle(image, round(target.x), round(target.y), 4, V4(0.7f, 0.7f, 0.7f, 1.0f));
        }
    }
    
    for (u32 vehicleIndex = 0; vehicleIndex < flow->vehicleCount; ++vehicleIndex)
    {
        Vehicle *vehicle = flow->vehicles + vehicleIndex;
        
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
    
    flow->zMod += dt * 0.1f;
    if (flow->zMod >= 256.0f)
    {
        flow->zMod -= 512.0f;
    }
    ++flow->ticks;
}
