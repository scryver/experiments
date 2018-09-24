//
 // NOTE(michiel): V2U
//

struct v2u
{
    u32 x;
    u32 y;
};

internal inline v2u
V2U(u32 x, u32 y)
{
    v2u result;
    
    result.x = x;
    result.y = y;
    
    return result;
}

//
// NOTE(michiel): V2
//

struct v2
{
    f32 x;
    f32 y;
};

internal inline v2
V2(f32 x, f32 y)
{
    v2 result;
    
    result.x = x;
    result.y = y;
    
    return result;
}

internal inline v2
operator -(v2 a)
{
    v2 result;
    result.x = -a.x;
    result.y = -a.y;
    return result;
}

internal inline v2 &
operator +=(v2 &a, v2 b)
{
    a.x += b.x;
    a.y += b.y;
    return a;
}

internal inline v2
operator +(v2 a, v2 b)
{
    v2 result = a;
    result += b;
    return result;
}

internal inline v2 &
operator -=(v2 &a, v2 b)
{
    a.x -= b.x;
    a.y -= b.y;
    return a;
}

internal inline v2
operator -(v2 a, v2 b)
{
    v2 result = a;
    result -= b;
    return result;
}

internal inline v2 &
operator *=(v2 &a, f32 b)
{
    a.x *= b;
    a.y *= b;
    return a;
}

internal inline v2
operator *(v2 a, f32 b)
{
    v2 result = a;
    result *= b;
    return result;
}

internal inline v2
operator *(f32 a, v2 b)
{
    return b * a;
}

internal inline v2 &
operator /=(v2 &a, f32 b)
{
    a *= 1.0f / b;
    return a;
}

internal inline v2
operator /(v2 a, f32 b)
{
    v2 result = a;
    result /= b;
    return result;
}

internal inline f32
length_squared(v2 a)
{
    f32 result;
    result = a.x * a.x + a.y * a.y;
    return result;
}

internal inline f32
length(v2 a)
{
    f32 result = length_squared(a);
    result = sqrtf(result);
    return result;
}

internal inline v2
normalize(v2 a)
{
    v2 result = {};
    f32 len = length(a);
    if (len != 0.0f)
    {
        result = a / len;
    }
    return result;
}

internal inline v2
set_length(v2 a, f32 length)
{
    v2 result = normalize(a);
    result *= length;
    return result;
}

internal inline v2
direction(v2 from, v2 to)
{
    v2 result = to - from;
    return result;
}

internal inline v2
direction_unit(v2 from, v2 to)
{
    v2 result = normalize(to - from);
    return result;
}

internal inline v2
rotate(v2 a, v2 rotation)
{
    v2 result = {};
    
    result.x = a.x * rotation.x - a.y * rotation.y;
    result.y = a.x * rotation.y + a.y * rotation.x;
    
    return result;
}

//
// NOTE(michiel): V4
//

union v4
{
    struct {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };
    struct {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
    };
};

internal inline v4
V4(f32 x, f32 y, f32 z, f32 w)
{
    v4 result;
    
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    
    return result;
}
