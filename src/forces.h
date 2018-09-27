struct Mover
{
    v2 acceleration;
    v2 velocity;
    v2 position;
    
    f32 mass;
    f32 oneOverMass;
};

internal inline Mover
create_mover(v2 position, f32 mass = 1.0f)
{
    Mover result = {};
    result.position = position;
    result.mass = mass;
    result.oneOverMass = 1.0f / mass;
    return result;
}

internal inline void
apply_force(Mover *mover, v2 force)
{
    mover->acceleration += force * mover->oneOverMass;
}

internal inline void
apply_friction(Mover *mover, f32 c = 0.01f)
{
    f32 constant = -c;
    v2 friction = normalize(mover->velocity);
    friction *= constant;
    apply_force(mover, friction);
}

internal inline void
apply_drag(Mover *mover, f32 c = 0.001f)
{
    f32 constant = -c;
    v2 drag = mover->velocity;
    drag *= length(drag) * constant;
    apply_force(mover, drag);
}

internal inline void
apply_gravity(Mover *m1, Mover *m2, f32 G = 1.0f)
{
    v2 dir = direction(m1->position, m2->position);
    f32 d2 = length_squared(dir);
    dir = normalize(dir);
    dir *= (G * m1->mass * m2->mass) / d2;
    apply_force(m1, dir);
    apply_force(m2, -dir);
}

internal inline void
apply_spring(Mover *mover, v2 anchor, f32 restLength, f32 k = 0.01f)
{
    v2 spring = mover->position - anchor;
    f32 curLength = length(spring);
    spring = normalize(spring);
    
    f32 stretch = curLength - restLength;
    spring *= -k * stretch;
    apply_force(mover, spring);
}

internal inline v2
attraction_force(Mover *mover, Mover *attractor, f32 G = 1.0f)
{
    v2 result = direction(mover->position, attractor->position);
    f32 d2 = length_squared(result);
    d2 = minimum(25.0f, maximum(5.0f, d2));
    result = normalize(result);
    result *= (G * mover->mass * attractor->mass) / d2;
    return result;
}

internal inline v2
steering_force(Mover *mover, v2 targetVelocity, f32 maxForce = 0.1f)
{
    v2 result = targetVelocity - mover->velocity;
    if (length_squared(result) > (maxForce * maxForce))
    {
        result = set_length(result, maxForce);
    }
    return result;
}

internal inline v2
seeking_force(Mover *mover, v2 targetPosition, f32 maxSpeed = 4.0f, f32 maxForce = 0.1f)
{
    v2 desired = targetPosition - mover->position;
    desired = set_length(desired, maxSpeed);
    return steering_force(mover, desired, maxForce);
}

internal inline v2
arriving_force(Mover *mover, v2 targetPosition, f32 catchDistance = 100.0f, 
               f32 maxSpeed = 4.0f, f32 maxForce = 0.1f)
{
    v2 desired = targetPosition - mover->position;
    f32 d2 = length_squared(desired);
    
    if (d2 < (catchDistance * catchDistance))
    {
        float t = sqrt(d2) / catchDistance; // TODO(michiel): Optimize
        desired = set_length(desired, maxSpeed * t);
    }
    else
    {
        desired = set_length(desired, maxSpeed);
    }
    
    return steering_force(mover, desired, maxForce);
}

internal inline void
update(Mover *mover, f32 dt = 1.0f, f32 maxVelocity = 0.0f)
{
    mover->velocity += mover->acceleration * dt;
    mover->position += mover->velocity * dt;
    if (maxVelocity != 0.0f)
    { 
        f32 maxVelSqr = maxVelocity * maxVelocity;
        f32 velMagSqr = length_squared(mover->velocity);
        if (velMagSqr > maxVelSqr)
        {
            f32 scale = maxVelSqr / velMagSqr;
            mover->velocity *= scale;
        }
    }
    mover->acceleration.x = 0.0f;
    mover->acceleration.y = 0.0f;
}
