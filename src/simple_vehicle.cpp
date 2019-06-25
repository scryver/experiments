#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "forces.h"
#include "vehicle.h"
#include "../libberdip/drawing.cpp"

struct VehicleState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    u32 vehicleCount;
    Vehicle *vehicles;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(VehicleState) <= state->memorySize);
    VehicleState *vehicleState = (VehicleState *)state->memory;
    if (!state->initialized)
    {
        vehicleState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        
        vehicleState->vehicleCount = 64;
        vehicleState->vehicles = allocate_array(Vehicle, vehicleState->vehicleCount);
        
        for (u32 vehicleIndex = 0; vehicleIndex < vehicleState->vehicleCount; ++vehicleIndex)
        {
            Vehicle *vehicle = vehicleState->vehicles + vehicleIndex;
            init_vehicle(vehicle, V2(random_choice(&vehicleState->randomizer, image->width), 
                                     random_choice(&vehicleState->randomizer, image->height)));
        }
        
        state->initialized = true;
    }
    
    //v2 center = V2((f32)image->width * 0.5f, (f32)image->height * 0.5f);
    
    for (u32 vehicleIndex = 0; vehicleIndex < vehicleState->vehicleCount; ++vehicleIndex)
    {
        Vehicle *vehicle = vehicleState->vehicles + vehicleIndex;
        seek(vehicle, mouse.pixelPosition);
        //arrive(vehicle, V2(mouse.pixelPosition));
        update(&vehicle->mover);
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    for (u32 vehicleIndex = 0; vehicleIndex < vehicleState->vehicleCount; ++vehicleIndex)
    {
        Vehicle *vehicle = vehicleState->vehicles + vehicleIndex;
        
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
    fill_circle(image, (f32)mouse.pixelPosition.x, (f32)mouse.pixelPosition.y, 
                20, V4(1, 1, 1, 0.7f));
    
    ++vehicleState->ticks;
}
