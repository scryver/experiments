struct Vehicle
{
    Mover mover;
    
    f32 maxForce;
    f32 maxSpeed;
    u32 radius;
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
steer(Vehicle *vehicle, v2 desired)
{
    v2 result = desired - vehicle->mover.velocity;
    if (length_squared(result) > (vehicle->maxForce * vehicle->maxForce))
    {
         result = set_length(result, vehicle->maxForce);
    }
    
    apply_force(&vehicle->mover, result);
}

internal void
seek(Vehicle *vehicle, v2 target)
{
    v2 desired = target - vehicle->mover.position;
    desired = set_length(desired, vehicle->maxSpeed);
    
    steer(vehicle, desired);
}

internal void
arrive(Vehicle *vehicle, v2 target, f32 catchDistance = 100.0f)
{
    v2 desired = target - vehicle->mover.position;
    f32 d2 = length_squared(desired);
    
    if (d2 < (catchDistance * catchDistance))
    {
        float t = sqrt(d2) / catchDistance; // TODO(michiel): Optimize
        desired = set_length(desired, vehicle->maxSpeed * t);
    }
    else
    {
        desired = set_length(desired, vehicle->maxSpeed);
    }
    
    steer(vehicle, desired);
}
