#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "random.h"
#include "forces.h"
#include "drawing.cpp"

struct Vehicle
{
    Mover mover;
    
    f32 maxForce;
    f32 maxSpeed;
    u32 radius;
};

struct VehicleState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    Vehicle vehicle;
};

internal void
init_vehicle(Vehicle *vehicle, v2 pos)
{
    vehicle->mover = create_mover(pos);
    vehicle->mover.velocity = V2(0, -2);
    
    vehicle->maxForce = 0.1f;
    vehicle->maxSpeed = 4.0f;
    vehicle->radius = 6;
}

internal void
seek(Vehicle *vehicle, v2 target)
{
    v2 desired = target - vehicle->mover.position;
    desired = set_length(desired, vehicle->maxSpeed);
    
    v2 steer = desired - vehicle->mover.velocity;
    if (length_squared(steer) > (vehicle->maxForce * vehicle->maxForce))
    {
        steer = set_length(steer, vehicle->maxForce);
    }
    
    apply_force(&vehicle->mover, steer);
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(VehicleState) <= state->memorySize);
    VehicleState *vehicleState = (VehicleState *)state->memory;
    if (!state->initialized)
    {
        vehicleState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        
        init_vehicle(&vehicleState->vehicle, V2(100, 50));
        
        state->initialized = true;
    }
    
    //v2 center = V2((f32)image->width * 0.5f, (f32)image->height * 0.5f);
    
    Vehicle *vehicle = &vehicleState->vehicle;
    
    seek(vehicle, V2(mouse.x, mouse.y));
    update(&vehicle->mover);
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    v2 dir = vehicle->mover.velocity;
    dir = normalize(dir);
    
    v2 front = V2(10, 0);
    v2 backUp = V2(-8, 5);
    v2 backDo = V2(-8, -5);
    
    front = rotate(front, dir);
     backUp = rotate(backUp, dir);
     backDo = rotate(backDo, dir);
    
    fill_triangle(image, front + vehicle->mover.position, backUp + vehicle->mover.position,
                  backDo + vehicle->mover.position, V4(1, 1, 0, 1));
    fill_circle(image, mouse.x, mouse.y, 20, V4(1, 1, 1, 1));
    
    ++vehicleState->ticks;
}
