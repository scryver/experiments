struct Vehicle
{
    Mover mover;
    
    f32 maxForce;
    f32 maxSpeed;
    u32 radius;
};

internal void
init_vehicle(Vehicle *vehicle, v2 pos, f32 maxSpeed = 4.0f, f32 maxForce = 0.1f)
{
    vehicle->mover = create_mover(pos);
    vehicle->mover.velocity = V2(0, -2);
    
    vehicle->maxForce = maxForce;
    vehicle->maxSpeed = maxSpeed;
    vehicle->radius = 6;
}

internal void
steer(Vehicle *vehicle, v2 desired)
{
    v2 force = steering_force(&vehicle->mover, desired, vehicle->maxForce);
    apply_force(&vehicle->mover, force);
}

internal void
seek(Vehicle *vehicle, v2 target)
{
    v2 force = seeking_force(&vehicle->mover, target, vehicle->maxSpeed, vehicle->maxForce);
    apply_force(&vehicle->mover, force);
}

internal void
arrive(Vehicle *vehicle, v2 target, f32 catchDistance = 100.0f)
{
    v2 force = arriving_force(&vehicle->mover, target, catchDistance,
                              vehicle->maxSpeed, vehicle->maxForce);
    apply_force(&vehicle->mover, force);
}
