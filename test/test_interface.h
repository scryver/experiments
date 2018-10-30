internal inline b32
almost_equal(f32 a, f32 b, f32 epsilon = 1e-6f)
{
    b32 result = false;
    
    if ((a >= (b - epsilon)) &&
        (a <= (b + epsilon)))
    {
        result = true;
    }
    
    return result;
}