struct FlowField
{
    v2u size;
    u32 tileSize;
    v2 *grid;
};

internal v2
lookup(FlowField *field, v2 position)
{
    s32 col = maximum(0, minimum(field->size.x - 1, round32(position.x / (f32)field->tileSize)));
    s32 row = maximum(0, minimum(field->size.y - 1, round32(position.y / (f32)field->tileSize)));

    v2 result = field->grid[row * field->size.x + col];
    return result;
}

internal void
follow(Vehicle *vehicle, FlowField *field)
{
    v2 desired = lookup(field, vehicle->mover.position);
    desired *= vehicle->maxSpeed;

    steer(vehicle, desired);
}

