#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct Boundary
{
    v2 start;
    v2 end;
};

struct Ray
{
    v2 pos;
    v2 dir;
};

struct RayBundle
{
    v2 pos;
    u32 rayCount;
    Ray *rays;
};

internal Boundary
init_boundary(v2 start, v2 end)
{
    Boundary result = {};
    result.start = start;
    result.end = end;
    return result;
}

internal void
draw_boundary(Image *image, Boundary boundary, v4 colour = V4(1, 1, 1, 1))
{
    draw_line(image, round32(boundary.start.x), round32(boundary.start.y),
              round32(boundary.end.x), round32(boundary.end.y), colour);
}

internal Ray
init_ray(v2 pos)
{
    Ray result = {};
    result.pos = pos;
    result.dir = V2(1, 0);
    return result;
}

internal void
draw_ray(Image *image, Ray ray, v4 colour = V4(1, 1, 1, 1))
{
    draw_line(image, ray.pos, ray.pos + ray.dir * 20.0f, colour);
}

internal RayBundle
init_ray_bundle(v2 pos, u32 rayCount)
{
    RayBundle result = {};
    result.pos = pos;
    result.rayCount = rayCount;
    result.rays = arena_allocate_array(gMemoryArena, Ray, rayCount, default_memory_alloc());

    f32 angleStep = F32_TAU / (f32)rayCount;
    f32 angle = 0.0f;
    for (u32 rayIdx = 0; rayIdx < rayCount; ++rayIdx) {
        Ray *ray = result.rays + rayIdx;
        ray->pos = pos;
        ray->dir = polar_to_cartesian(1.0f, angle);
        angle += angleStep;
    }
    return result;
}

internal void
draw_ray_bundle(Image *image, RayBundle bundle, v4 colour = V4(1, 1, 1, 1))
{
    fill_circle(image, round32(bundle.pos.x), round32(bundle.pos.y), 3, colour);
    for (u32 rayIdx = 0; rayIdx < bundle.rayCount; ++rayIdx) {
        draw_ray(image, bundle.rays[rayIdx], colour);
    }
}

struct RayIntersection
{
    b32 intersects;
    v2 at;
};

internal RayIntersection
ray_intersects_wall(Ray ray, Boundary wall)
{
    RayIntersection result = {};

    f32 x1 = wall.start.x;
    f32 y1 = wall.start.y;
    f32 x2 = wall.end.x;
    f32 y2 = wall.end.y;

    f32 x3 = ray.pos.x;
    f32 y3 = ray.pos.y;
    f32 x4 = x3 + ray.dir.x;
    f32 y4 = y3 + ray.dir.y;

    f32 den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (den != 0.0f) {
        f32 oneOverDen = 1.0f / den;
        f32 t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) * oneOverDen;
        f32 u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) * oneOverDen;

        if ((0.0f < t) && (t < 1.0f) && (0.0f < u)) {
            result.intersects = true;
            result.at.x = lerp(x1, t, x2);
            result.at.y = lerp(y1, t, y2);
        }
    }

    return result;
}

internal void
ray_bundle_intersects_walls(Image *image, RayBundle bundle, u32 wallCount, Boundary *walls)
{
    for (u32 rayIdx = 0; rayIdx < bundle.rayCount; ++rayIdx) {
        Ray ray = bundle.rays[rayIdx];
        f32 closestLenSq = F32_MAX;
        v2 closestRay = {};
        for (u32 wallIdx = 0; wallIdx < wallCount; ++wallIdx) {
            RayIntersection castRay = ray_intersects_wall(ray, walls[wallIdx]);
            if (castRay.intersects) {
                f32 dist = length_squared(castRay.at - ray.pos);
                if (closestLenSq > dist) {
                    closestLenSq = dist;
                    closestRay = castRay.at;
                }
            }
        }
        if (closestLenSq != F32_MAX) {
            draw_line(image, round32(bundle.pos.x), round32(bundle.pos.y),
                      round32(closestRay.x), round32(closestRay.y), V4(1, 1, 1, 0.5f));
        }
    }
}

struct BasicState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;

    u32 wallCount;
    Boundary *walls;
    RayBundle bundle;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(BasicState) <= state->memorySize);

    v2 size = V2((f32)image->width, (f32)image->height);

    BasicState *basics = (BasicState *)state->memory;
    if (!state->initialized)
    {
        // basics->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        basics->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);

        basics->wallCount = 5 + 4;
        basics->walls = arena_allocate_array(gMemoryArena, Boundary, basics->wallCount, default_memory_alloc());

#if 0
        basics->walls[0] = init_boundary(V2(0, 0), V2(size.x, 0));
        basics->walls[1] = init_boundary(V2(size.x, 0), size);
        basics->walls[2] = init_boundary(size, V2(0, size.y));
        basics->walls[3] = init_boundary(V2(0, size.y), V2(0, 0));
#endif

        for (u32 wallIdx = 4; wallIdx < basics->wallCount; ++wallIdx) {
            Boundary *wall = basics->walls + wallIdx;
            wall->start.x = random_unilateral(&basics->randomizer) * size.x;
            wall->start.y = random_unilateral(&basics->randomizer) * size.y;
            wall->end.x = random_unilateral(&basics->randomizer) * size.x;
            wall->end.y = random_unilateral(&basics->randomizer) * size.y;
        }

        basics->bundle = init_ray_bundle(V2(200, 200), 256);

        state->initialized = true;
    }

    basics->bundle.pos = hadamard(mouse.relativePosition, size);
    for (u32 rayIdx = 0; rayIdx < basics->bundle.rayCount; ++rayIdx) {
        Ray *ray = basics->bundle.rays + rayIdx;
        ray->pos = basics->bundle.pos;
    }

    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));

    for (u32 wallIdx = 0; wallIdx < basics->wallCount; ++wallIdx) {
        draw_boundary(image, basics->walls[wallIdx]);
    }
    draw_ray_bundle(image, basics->bundle);

    ray_bundle_intersects_walls(image, basics->bundle, basics->wallCount, basics->walls);

#if 0
    RayIntersection castRay = ray_intersects_wall(basics->ray, basics->wall);
    if (castRay.intersects) {
        draw_ray(image, basics->ray, V4(0, 1, 0, 1));
        fill_circle(image, round(castRay.at.x), round(castRay.at.y), 8, V4(1, 1, 1, 1));
    } else {
        draw_ray(image, basics->ray, V4(1, 0, 0, 1));
    }

    //basics->ray.dir = rotate(basics->ray.dir, polar_to_cartesian(1.0, 0.2f * dt * F32_TAU));
    basics->ray.dir = direction_unit(basics->ray.pos, hadamard(mouse.relativePosition, size));
#endif

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
