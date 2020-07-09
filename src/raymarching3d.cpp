#include "interface.h"
DRAW_IMAGE(draw_image);


#define MAIN_WINDOW_WIDTH  400
#define MAIN_WINDOW_HEIGHT 300
#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct ImplicitGeometry
{
    f32 dist;
    f32 mat;
};

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
normalized_screen(v2 screenP, v2 screenSize)
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

internal ImplicitGeometry
implicit_plane(v3 p, v4 n, f32 material = 0)
{
    ImplicitGeometry result;
    result.dist = dot(p, n.xyz) + n.w;
    result.mat = material;
    return result;
}

internal ImplicitGeometry
implicit_sphere(v3 p, v3 center, f32 radius, f32 material = 0)
{
    ImplicitGeometry result;
    result.dist = length(p - center) - radius;
    result.mat = material;
    return result;
}

internal ImplicitGeometry
implicit_box(v3 p, v3 center, v3 size, f32 material = 0)
{
    v3 d = absolute(p - center) - size;
    ImplicitGeometry result;
    result.dist = minimum(0.0f, maximum3(d.x, d.y, d.z)) + length(V3(maximum(d.x, 0.0f), maximum(d.y, 0.0f), maximum(d.z, 0.0f)));
    result.mat = material;
    return result;
}

internal ImplicitGeometry
implicit_capsule(v3 p, v3 center, v3 a, v3 b, f32 radius, f32 material = 0)
{
    v3 pa = (p - center) - a;
    v3 ba = b - a;
    f32 h = clamp01(dot(pa, ba) / dot(ba, ba));
    ImplicitGeometry result;
    result.dist = length(pa - h * ba) - radius;
    result.mat = material;
    return result;
}

//
// NOTE(michiel): Operators
//

internal ImplicitGeometry &
operator |=(ImplicitGeometry &a, ImplicitGeometry b)
{
    a = a.dist < b.dist ? a : b;
    return a;
}

internal ImplicitGeometry &
operator &=(ImplicitGeometry &a, ImplicitGeometry b)
{
    a = a.dist > b.dist ? a : b;
    return a;
}

internal ImplicitGeometry &
operator -=(ImplicitGeometry &a, ImplicitGeometry b)
{
    if (a.dist < -b.dist)
    {
        a.dist = -b.dist;
        a.mat = b.mat;
    }
    return a;
}

internal ImplicitGeometry
operator |(ImplicitGeometry a, ImplicitGeometry b)
{
    ImplicitGeometry result = a;
    result |= b;
    return result;
}

internal ImplicitGeometry
operator &(ImplicitGeometry a, ImplicitGeometry b)
{
    ImplicitGeometry result = a;
    result &= b;
    return result;
}

internal ImplicitGeometry
operator -(ImplicitGeometry a, ImplicitGeometry b)
{
    ImplicitGeometry result = a;
    result -= b;
    return result;
}

internal ImplicitGeometry
smooth_union(ImplicitGeometry a, f32 t, ImplicitGeometry b)
{
    ImplicitGeometry result;
    f32 h = clamp01(0.5f + 0.5f * (b.dist - a.dist) / t);
    result.dist = lerp(b.dist, h, a.dist) - t * h * (1.0f - h);
    result.mat = lerp(b.mat, h, a.mat) - t * h * (1.0f - h);
    return result;
}

internal ImplicitGeometry
smooth_subtract(ImplicitGeometry a, f32 t, ImplicitGeometry b)
{
    ImplicitGeometry result;
    f32 h = clamp01(0.5f - 0.5f * (b.dist + a.dist) / t);
    result.dist = lerp(a.dist, h, -b.dist) + t * h * (1.0f - h);
    result.mat = lerp(a.mat, h, b.mat) + t * h * (1.0f - h);
    return result;
}

internal ImplicitGeometry
smooth_intersect(ImplicitGeometry a, f32 t, ImplicitGeometry b)
{
    ImplicitGeometry result;
    f32 h = clamp01(0.5f - 0.5f * (b.dist - a.dist) / t);
    result.dist = lerp(b.dist, h, a.dist) + t * h * (1.0f - h);
    result.mat = lerp(b.mat, h, a.mat) + t * h * (1.0f - h);
    return result;
}

//
// NOTE(michiel): World
//

internal ImplicitGeometry
world_surface(v3 p)
{
    ImplicitGeometry result = implicit_sphere(p, V3(3.0f, -2.5f, 10.0f), 2.5f);
    result |= implicit_sphere(p, V3(-3.0f, -2.5f, 10.0f), 2.5f);
    result |= implicit_sphere(p, V3(0, 2.5f, 10.0f), 2.5f);
    result |= implicit_plane(p, V4(0, 1, 0, 5.5f), 1);
    return result;
}

//
// NOTE(michiel): Camera
//
internal v3
get_camera_ray_dir(v2 uv, f32 perspective, v3 cameraForward, v3 cameraRight, v3 cameraUp)
{
    v3 result = normalize_or_zero(uv.x * cameraRight + uv.y * cameraUp + perspective * cameraForward);
    return result;
}

#if 0
internal v3
get_camera_ray_dir(v2 uv, v3 cameraP, v3 cameraTarget)
{
    v3 cameraForward = normalize(cameraTarget - cameraP);
    v3 cameraRight = normalize(cross(V3(0, 1, 0), cameraForward));
    v3 cameraUp = normalize(cross(cameraForward, cameraRight));
    
    v3 result = get_camera_ray_dir(uv, cameraForward, cameraRight, cameraUp);
    return result;
}
#endif


internal f32
checkers_texture(v2 p)
{
    v2 f = V2(fraction(0.5f * p.x), fraction(0.5f * p.y)) - 0.5f;
    
    v2 s;
    s.x = sign_of(f.x);
    s.y = sign_of(f.y);
    
    f32 result = 0.5f - 0.5f * s.x * s.y;
    return result;
}

//
// NOTE(michiel): Ray marching
//
internal ImplicitGeometry
cast_ray(v3 origin, v3 direction)
{
    ImplicitGeometry result;
    result.dist = -F32_MAX;
    result.mat = -1.0f;
    
    f32 t = 0.0f;
    f32 tMax = 250.0f;
    
    for (u32 tryIdx = 0; tryIdx < 256; ++tryIdx)
    {
        ImplicitGeometry d = world_surface(origin + t * direction);
        if ((f64)d.dist < (0.001 * (f64)t))
        {
            result.dist = t;
            break;
        }
        else if (d.dist > tMax)
        {
            result.dist = -1.0f;
            result.mat = -1.0f;
            break;
        }
        t += d.dist;
        result.mat = d.mat;
    }
    
    if (result.dist == -F32_MAX)
    {
        result.dist = t;
    }
    
    return result;
}

internal v3
calculate_normal(v3 p)
{
    f32 eps = 0.001f;
    
    ImplicitGeometry geom = world_surface(p);
    f32 c = geom.dist;
    v3 result = normalize_or_zero(V3(world_surface(p + V3(eps, 0, 0)).dist,
                                     world_surface(p + V3(0, eps, 0)).dist,
                                     world_surface(p + V3(0, 0, eps)).dist) - c);
    return result;
}

internal v3
render(v3 origin, v3 direction, f32 timeAt)
{
    v3 result = V3(0.3f, 0.36f, 0.6f) - 0.4f * direction.y;
    
    ImplicitGeometry tRay = cast_ray(origin, direction);
    v3 l = normalize_or_zero(V3(sin_f32(0.2f * timeAt), cos_f32(0.1f*timeAt) + 0.5f, -0.5f));
    
    if (tRay.mat > -1.0f)
    {
        v3 pos = origin + tRay.dist * direction;
        
        if (tRay.mat > 0.0f)
        {
            f32 grid = tRay.mat * (checkers_texture(0.4f * V2(pos.x, pos.z)) * 0.03f + 0.1f);
            result = V3(grid, grid, grid);
        }
        else
        {
            v3 surfaceNormal = calculate_normal(pos);
            
            v3 surfaceColour = V3(0.4f, 0.8f, 0.1f);
            f32 NoL = maximum(dot(surfaceNormal, l), 0.0f);
            v3 lightDirect = V3(1.8f, 1.27f, 0.99f) * NoL;
            v3 lightAmbient = V3(0.03f, 0.04f, 0.1f);
            
            v3 diffuse = hadamard(surfaceColour, lightDirect + lightAmbient);
            result = diffuse;
            
            v3 shadowOrigin = pos + 0.01f * surfaceNormal;
            v3 shadowDir = l;
            ImplicitGeometry tShadow = cast_ray(shadowOrigin, shadowDir);
            result = (tShadow.dist >= -1.0f) ? 0.2f * result : result;
        }
    }
    
    return result;
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
    
    v3 cameraPos = V3(0, 0, -1);
    v3 cameraTarget = V3(0, 0, 0);
    f32 cameraPerspective = 2.0f;
    
    v3 cameraForward = normalize_or_zero(cameraTarget - cameraPos);
    v3 cameraRight = normalize_or_zero(cross(V3(0, 1, 0), cameraForward));
    v3 cameraUp = normalize_or_zero(cross(cameraForward, cameraRight));
    
    u32 *pixels = image->pixels;
    for (u32 y = 0; y < image->height; ++y)
    {
        u32 *pixelRow = pixels;
        for (u32 x = 0; x < image->width; ++x)
        {
            v2 at = V2((f32)x, (f32)y);
            
            v2 uv = normalized_screen(at, size);
            v3 rayDir = get_camera_ray_dir(uv, cameraPerspective, cameraForward, cameraRight, cameraUp);
            
            v3 colour = render(cameraPos, rayDir, basics->seconds);
            draw_pixel(pixelRow++, sRGB_from_linear(V4(colour, 1.0f)));
            //draw_pixel(pixelRow++, V4(colour, 1.0f));
        }
        pixels += image->rowStride;
    }
    
    basics->prevMouseDown = mouse.mouseDowns;
    basics->seconds += dt;
    ++basics->ticks;
    if (basics->seconds > 10.0f)
    {
        basics->seconds -= 10.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", basics->ticks,
                1000.0f / (f32)basics->ticks);
        basics->ticks = 0;
    }
}
