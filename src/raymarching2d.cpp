#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct BasicState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
};

internal f32
smoothstep(f32 min, f32 t, f32 max)
{
    f32 x = clamp01((t - min) / (max - min));
    f32 result = x * x * (3.0f - 2.0f * x);
    return result;
}

internal v2
world_from_screen(v2 screenP, v2 screenSize)
{
#if 0
    v2 result = 2.0f * V2(screenP.x / screenSize.x, screenP.y / screenSize.y) - 1.0f;
    result.x *= screenSize.x / screenSize.y;
#else
    f32 oneOverHeight = 1.0f / screenSize.y;
    v2 result = (2.0f * screenP - screenSize) * oneOverHeight;
#endif
    result.y = -result.y;
    return result;
}

internal v3
palette(f32 t, v3 a, v3 b, v3 c, v3 d)
{
    t = clamp01(t);
    
    v3 theta = t * c + d;
    v3 cosTheta = V3(cos_f32(theta.x), cos_f32(theta.y), cos_f32(theta.z));
    return a + hadamard(b, cosTheta);
}

internal v3
shade(f32 dist)
{
    f32 maxDist = 2.0f;
    v3 palCol = palette(clamp(-maxDist, 0.5f - 0.4f * dist, maxDist),
                        V3(0.3f, 0.3f, 0.0f), V3(0.8f, 0.8f, 0.1f), V3(0.9f, 0.7f, 0.0f), V3(0.3f, 0.9f, 0.8f));
    v3 colour = palCol;
    colour = lerp(colour, 0.4f, colour - exp(-10.0f * absolute(dist)));
    colour *= 0.8f + 0.2f * cos_pi(150.0f * dist);
    colour = lerp(colour, 1.0f - smoothstep(0.0f, absolute(dist), 0.01f), V3(1, 1, 1));
    return colour;
}

//
// NOTE(michiel): Geometry
//

internal f32
implicit_sphere(v2 p, v2 center, f32 radius)
{
    f32 result = length(p - center) - radius;
    return result;
}

internal f32
implicit_box(v2 p, v2 center, v2 size)
{
    v2 d = absolute(p - center) - size;
    f32 result = minimum(0.0f, maximum(d.x, d.y)) + length(V2(maximum(d.x, 0.0f), maximum(d.y, 0.0f)));
    return result;
}

//
// NOTE(michiel): Operators
//

internal f32
smooth_min_cubic(f32 a, f32 t, f32 b)
{
    f32 h = maximum(t - absolute(a - b), 0.0f);
    f32 result = minimum(a, b) - h * h * h / (6.0f * t * t);
    return result;
}

internal f32
op_union(f32 a, f32 b)
{
    return minimum(a, b);
}

internal f32
op_subtract(f32 a, f32 b)
{
    return maximum(a, -b);
}

internal f32
op_intersect(f32 a, f32 b)
{
    return maximum(a, b);
}

internal f32
op_blend(f32 a, f32 b)
{
    f32 result = smooth_min_cubic(a, 0.2f, b);
    return result;
}

internal f32
op_smooth_union(f32 a, f32 t, f32 b)
{
    f32 h = clamp01(0.5f + 0.5f * (b - a) / t);
    return lerp(b, h, a) - t * h * (1.0f - h);
}

internal f32
op_smooth_subtract(f32 a, f32 t, f32 b)
{
    f32 h = clamp01(0.5f - 0.5f * (b + a) / t);
    return lerp(a, h, -b) + t * h * (1.0f - h);
}

internal f32
op_smooth_intersect(f32 a, f32 t, f32 b)
{
    f32 h = clamp01(0.5f - 0.5f * (b - a) / t);
    return lerp(b, h, a) + t * h * (1.0f - h);
}

//
// NOTE(michiel): World
//
internal f32
world_surface(v2 p)
{
    f32 d = 1000.0f;
    d = op_union(d, implicit_sphere(p, V2(-0.1f, 0.4f), 0.4f));
    d = op_subtract(d, implicit_sphere(p, V2(-0.1f, 0.4f), 0.2f));
    d = op_smooth_union(d, 0.1f, implicit_box(p, V2( 0.5f, 0.1f), V2(0.35f, 0.5f)));
    return d;
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(BasicState) <= state->memorySize);
    
    v2 size = V2((f32)image->width, (f32)image->height);
    
    BasicState *basics = (BasicState *)state->memory;
    if (!state->initialized)
    {
        // basics->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        basics->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        state->initialized = true;
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    u32 *pixels = image->pixels;
    for (u32 y = 0; y < image->height; ++y)
    {
        u32 *pixelRow = pixels;
        for (u32 x = 0; x < image->width; ++x)
        {
            v2 at = V2((f32)x, (f32)y);
            
            f32 dist = world_surface(world_from_screen(at, size));
            v3 colour = shade(dist);
            draw_pixel(pixelRow++, V4(colour, 1.0f));
        }
        pixels += image->width;
    }
    
    basics->prevMouseDown = mouse.mouseDowns;
    basics->seconds += dt;
    ++basics->ticks;
    if (basics->seconds > 1.0f)
    {
        basics->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", basics->ticks,
                1000.0f / (f32)basics->ticks);
        basics->ticks = 0;
    }
}
