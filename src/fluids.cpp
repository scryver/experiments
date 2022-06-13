#include "interface.h"
DRAW_IMAGE(draw_image);

// NOTE(michiel): See: https://mikeash.com/pyblog/fluid-simulation-for-dummies.html

#include "main.cpp"

#include "../libberdip/drawing.cpp"

#define IXP(x, y)    ((x) + (y) * N)
#define IXC(x, y, z) ((x) + (y) * N + (z) * N * N)

struct FluidPlane
{
    s32 size;
    f32 diffusion;
    f32 viscosity;

    f32 *s;
    f32 *density;

    f32 *velX;
    f32 *velY;
    f32 *velX0;
    f32 *velY0;
};

struct FluidCube
{
    s32 size;
    f32 diffusion;
    f32 viscosity;

    f32 *s;
    f32 *density;

    f32 *velX;
    f32 *velY;
    f32 *velZ;
    f32 *velX0;
    f32 *velY0;
    f32 *velZ0;
};

struct FluidBase
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    v2  prevMousePos;

    f32 t;
    PerlinNoise noise;

    FluidPlane plane;
    FluidCube  cube;
};

internal void
set_bound_plane(s32 bound, f32 *x, s32 N)
{
    for (s32 i = 1; i < N - 1; ++i) {
        x[IXP(i,     0)] = (bound == 2) ? -x[IXP(i,     1)] : x[IXP(i,     1)];
        x[IXP(i, N - 1)] = (bound == 2) ? -x[IXP(i, N - 2)] : x[IXP(i, N - 2)];
    }

    for (s32 j = 1; j < N - 1; ++j) {
        x[IXP(    0, j)] = (bound == 1) ? -x[IXP(    1, j)] : x[IXP(    1, j)];
        x[IXP(N - 1, j)] = (bound == 1) ? -x[IXP(N - 2, j)] : x[IXP(N - 2, j)];
    }

    // NOTE(michiel): Handle corners
    x[IXP(  0,   0)] = 0.33f * (x[IXP(  1,   0)] + x[IXP(  0,   1)]);
    x[IXP(  0, N-1)] = 0.33f * (x[IXP(  1, N-1)] + x[IXP(  0, N-2)]);
    x[IXP(N-1,   0)] = 0.33f * (x[IXP(N-2,   0)] + x[IXP(N-1,   1)]);
    x[IXP(N-1, N-1)] = 0.33f * (x[IXP(N-2, N-1)] + x[IXP(N-1, N-2)]);
}

internal void
linear_solve_plane(s32 bound, f32 *x, f32 *x0, f32 mult, f32 div, s32 iter, s32 N)
{
    f32 c = 1.0f / div;
    for (s32 k = 0; k < iter; ++k) {
        for (s32 j = 1; j < N - 1; ++j) {
            for (s32 i = 1; i < N - 1; ++i) {
                x[IXP(i, j)] = (x0[IXP(i, j)] + mult * (x[IXP(i+1, j  )] +
                                                        x[IXP(i-1, j  )] +
                                                        x[IXP(i  , j+1)] +
                                                        x[IXP(i  , j-1)])) * c;
            }
        }
        set_bound_plane(bound, x, N);
    }
}

internal void
diffuse_plane(s32 bound, f32 *x, f32 *x0, f32 amount, f32 dt, s32 iter, s32 N)
{
    f32 a = dt * amount * (f32)(N - 2) * (f32)(N - 2);
    linear_solve_plane(bound, x, x0, a, 1 + 6 * a, iter, N);
}

internal void
project_plane(f32 *velX, f32 *velY, f32 *p, f32 *div, s32 iter, s32 N)
{
    f32 oneOverN = 1.0f / (f32)N;
    for (s32 j = 1; j < N - 1; ++j) {
        for (s32 i = 1; i < N - 1; ++i) {
            div[IXP(i, j)] = -0.5f * (velX[IXP(i+1, j  )] -
                                      velX[IXP(i-1, j  )] +
                                      velY[IXP(i  , j+1)] -
                                      velY[IXP(i  , j-1)]) * oneOverN;
            p[IXP(i, j)] = 0.0f;
        }
    }

    set_bound_plane(0, div, N);
    set_bound_plane(0, p, N);
    linear_solve_plane(0, p, div, 1, 6, iter, N);

    for (s32 j = 1; j < N - 1; ++j) {
        for (s32 i = 1; i < N - 1; ++i) {
            velX[IXP(i, j)] -= 0.5f * (p[IXP(i+1, j  )] -
                                       p[IXP(i-1, j  )]) * N;
            velY[IXP(i, j)] -= 0.5f * (p[IXP(i  , j+1)] -
                                       p[IXP(i  , j-1)]) * N;
        }
    }
    set_bound_plane(1, velX, N);
    set_bound_plane(2, velY, N);
}

internal void
advect_plane(s32 bound, f32 *d, f32 *d0, f32 *velX, f32 *velY, f32 dt, s32 N)
{
    f32 dtx = dt * (N - 2);
    f32 dty = dt * (N - 2);

    f32 NFloat = (f32)N;
    f32 iFloat, jFloat;
    s32 i, j;

    for (j = 1, jFloat = 1.0f; j < N - 1; ++j, ++jFloat) {
        for (i = 1, iFloat = 1.0f; i < N - 1; ++i, ++iFloat) {
            s32 index = IXP(i, j);
            f32 x = iFloat - dtx * velX[index];
            f32 y = jFloat - dty * velY[index];

            if (x < 0.5f) {
                x = 0.5f;
            }
            if (x > NFloat + 0.5f) {
                x = NFloat + 0.5f;
            }
            if (y < 0.5f) {
                y = 0.5f;
            }
            if (y > NFloat + 0.5f) {
                y = NFloat + 0.5f;
            }

            f32 i0 = floor32(x);
            f32 i1 = i0 + 1.0f;
            f32 j0 = floor32(y);
            f32 j1 = j0 + 1.0f;

            f32 s1 = x - i0;
            f32 s0 = 1.0f - s1;
            f32 t1 = y - j0;
            f32 t0 = 1.0f - t1;

            s32 i0i = clamp(0, s32_from_f32_truncate(i0), N);
            s32 i1i = clamp(0, s32_from_f32_truncate(i1), N);
            s32 j0i = clamp(0, s32_from_f32_truncate(j0), N);
            s32 j1i = clamp(0, s32_from_f32_truncate(j1), N);

            d[IXP(i, j)] = (s0 * (t0 * d0[IXP(i0i, j0i)] +
                                  t1 * d0[IXP(i0i, j1i)]) +
                            s1 * (t0 * d0[IXP(i1i, j0i)] +
                                  t1 * d0[IXP(i1i, j1i)]));
        }
    }
    set_bound_plane(bound, d, N);
}

internal void
set_bound_cube(s32 bound, f32 *x, s32 N)
{
    for (s32 j = 1; j < N - 1; ++j) {
        for (s32 i = 1; i < N - 1; ++i) {
            x[IXC(i, j,     0)] = (bound == 3) ? -x[IXC(i, j,     1)] : x[IXC(i, j,     1)];
            x[IXC(i, j, N - 1)] = (bound == 3) ? -x[IXC(i, j, N - 2)] : x[IXC(i, j, N - 2)];
        }
    }
    for (s32 k = 1; k < N - 1; ++k) {
        for (s32 i = 1; i < N - 1; ++i) {
            x[IXC(i,     0, k)] = (bound == 2) ? -x[IXC(i,     1, k)] : x[IXC(i,     1, k)];
            x[IXC(i, N - 1, k)] = (bound == 2) ? -x[IXC(i, N - 2, k)] : x[IXC(i, N - 2, k)];
        }
    }
    for (s32 k = 1; k < N - 1; ++k) {
        for (s32 j = 1; j < N - 1; ++j) {
            x[IXC(    0, j, k)] = (bound == 1) ? -x[IXC(    1, j, k)] : x[IXC(    1, j, k)];
            x[IXC(N - 1, j, k)] = (bound == 1) ? -x[IXC(N - 2, j, k)] : x[IXC(N - 2, j, k)];
        }
    }

    // NOTE(michiel): Handle corners
    x[IXC(  0,   0,   0)] = 0.33f * (x[IXC(  1,   0,   0)] + x[IXC(  0,   1,   0)] + x[IXC(  0,   0,   1)]);
    x[IXC(  0, N-1,   0)] = 0.33f * (x[IXC(  1, N-1,   0)] + x[IXC(  0, N-2,   0)] + x[IXC(  0, N-1,   1)]);
    x[IXC(  0,   0, N-1)] = 0.33f * (x[IXC(  1,   0, N-1)] + x[IXC(  0,   1, N-1)] + x[IXC(  0,   0, N-2)]);
    x[IXC(  0, N-1, N-1)] = 0.33f * (x[IXC(  1, N-1, N-1)] + x[IXC(  0, N-2, N-1)] + x[IXC(  0, N-1, N-2)]);
    x[IXC(N-1,   0,   0)] = 0.33f * (x[IXC(N-2,   0,   0)] + x[IXC(N-1,   1,   0)] + x[IXC(N-1,   0,   1)]);
    x[IXC(N-1, N-1,   0)] = 0.33f * (x[IXC(N-2, N-1,   0)] + x[IXC(N-1, N-2,   0)] + x[IXC(N-1, N-1,   1)]);
    x[IXC(N-1,   0, N-1)] = 0.33f * (x[IXC(N-2,   0, N-1)] + x[IXC(N-1,   1, N-1)] + x[IXC(N-1,   0, N-2)]);
    x[IXC(N-1, N-1, N-1)] = 0.33f * (x[IXC(N-2, N-1, N-1)] + x[IXC(N-1, N-2, N-1)] + x[IXC(N-1, N-1, N-2)]);
}

internal void
linear_solve_cube(s32 bound, f32 *x, f32 *x0, f32 mult, f32 div, s32 iter, s32 N)
{
    f32 c = 1.0f / div;
    for (s32 k = 0; k < iter; ++k) {
        for (s32 m = 1; m < N - 1; ++m) {
            for (s32 j = 1; j < N - 1; ++j) {
                for (s32 i = 1; i < N - 1; ++i) {
                    x[IXC(i, j, m)] = (x0[IXC(i, j, m)] + mult * (
                                                                  x[IXC(i+1, j  , m  )] +
                                                                  x[IXC(i-1, j  , m  )] +
                                                                  x[IXC(i  , j+1, m  )] +
                                                                  x[IXC(i  , j-1, m  )] +
                                                                  x[IXC(i  , j  , m+1)] +
                                                                  x[IXC(i  , j  , m-1)])) * c;
                }
            }
        }
        set_bound_cube(bound, x, N);
    }
}

internal void
diffuse_cube(s32 bound, f32 *x, f32 *x0, f32 amount, f32 dt, s32 iter, s32 N)
{
    f32 a = dt * amount * (f32)(N - 2) * (f32)(N - 2);
    linear_solve_cube(bound, x, x0, a, 1 + 6 * a, iter, N);
}

internal void
project_cube(f32 *velX, f32 *velY, f32 *velZ, f32 *p, f32 *div, s32 iter, s32 N)
{
    f32 oneOverN = 1.0f / (f32)N;
    for (s32 k = 1; k < N - 1; ++k) {
        for (s32 j = 1; j < N - 1; ++j) {
            for (s32 i = 1; i < N - 1; ++i) {
                div[IXC(i, j, k)] = -0.5f * (velX[IXC(i+1, j  , k  )] -
                                             velX[IXC(i-1, j  , k  )] +
                                             velY[IXC(i  , j+1, k  )] -
                                             velY[IXC(i  , j-1, k  )] +
                                             velZ[IXC(i  , j  , k+1)] -
                                             velZ[IXC(i  , j  , k-1)]) * oneOverN;
                p[IXC(i, j, k)] = 0.0f;
            }
        }
    }
    set_bound_cube(0, div, N);
    set_bound_cube(0, p, N);
    linear_solve_cube(0, p, div, 1, 6, iter, N);

    for (s32 k = 1; k < N - 1; ++k) {
        for (s32 j = 1; j < N - 1; ++j) {
            for (s32 i = 1; i < N - 1; ++i) {
                velX[IXC(i, j, k)] -= 0.5f * (p[IXC(i+1, j  , k  )] -
                                              p[IXC(i-1, j  , k  )]) * N;
                velY[IXC(i, j, k)] -= 0.5f * (p[IXC(i  , j+1, k  )] -
                                              p[IXC(i  , j-1, k  )]) * N;
                velZ[IXC(i, j, k)] -= 0.5f * (p[IXC(i  , j  , k+1)] -
                                              p[IXC(i  , j  , k-1)]) * N;
            }
        }
    }
    set_bound_cube(1, velX, N);
    set_bound_cube(2, velY, N);
    set_bound_cube(3, velZ, N);
}

internal void
advect_cube(s32 bound, f32 *d, f32 *d0, f32 *velX, f32 *velY, f32 *velZ, f32 dt, s32 N)
{
    f32 dtx = dt * (N - 2);
    f32 dty = dt * (N - 2);
    f32 dtz = dt * (N - 2);

    f32 NFloat = (f32)N;
    f32 iFloat, jFloat, kFloat;
    s32 i, j, k;

    for (k = 1, kFloat = 1.0f; k < N - 1; ++k, ++kFloat) {
        for (j = 1, jFloat = 1.0f; j < N - 1; ++j, ++jFloat) {
            for (i = 1, iFloat = 1.0f; i < N - 1; ++i, ++iFloat) {
                s32 index = IXC(i, j, k);
                f32 x = iFloat - dtx * velX[index];
                f32 y = jFloat - dty * velY[index];
                f32 z = kFloat - dtz * velZ[index];

                if (x < 0.5f) {
                    x = 0.5f;
                }
                if (x > NFloat + 0.5f) {
                    x = NFloat + 0.5f;
                }
                if (y < 0.5f) {
                    y = 0.5f;
                }
                if (y > NFloat + 0.5f) {
                    y = NFloat + 0.5f;
                }
                if (z < 0.5f) {
                    z = 0.5f;
                }
                if (z > NFloat + 0.5f) {
                    z = NFloat + 0.5f;
                }

                f32 i0 = floor32(x);
                f32 i1 = i0 + 1.0f;
                f32 j0 = floor32(y);
                f32 j1 = j0 + 1.0f;
                f32 k0 = floor32(z);
                f32 k1 = k0 + 1.0f;

                f32 s1 = x - i0;
                f32 s0 = 1.0f - s1;
                f32 t1 = y - j0;
                f32 t0 = 1.0f - t1;
                f32 u1 = z - k0;
                f32 u0 = 1.0f - u1;

                s32 i0i = clamp(0, s32_from_f32_truncate(i0), N);
                s32 i1i = clamp(0, s32_from_f32_truncate(i1), N);
                s32 j0i = clamp(0, s32_from_f32_truncate(j0), N);
                s32 j1i = clamp(0, s32_from_f32_truncate(j1), N);
                s32 k0i = clamp(0, s32_from_f32_truncate(k0), N);
                s32 k1i = clamp(0, s32_from_f32_truncate(k1), N);

                d[IXC(i, j, k)] = (s0 * (t0 * (u0 * d0[IXC(i0i, j0i, k0i)] +
                                               u1 * d0[IXC(i0i, j0i, k1i)]) +
                                         t1 * (u0 * d0[IXC(i0i, j1i, k0i)] +
                                               u1 * d0[IXC(i0i, j1i, k1i)])) +
                                   s1 * (t0 * (u0 * d0[IXC(i1i, j0i, k0i)] +
                                               u1 * d0[IXC(i1i, j0i, k1i)]) +
                                         t1 * (u0 * d0[IXC(i1i, j1i, k0i)] +
                                               u1 * d0[IXC(i1i, j1i, k1i)])));
            }
        }
    }
    set_bound_cube(bound, d, N);
}

internal void
fluid_init(FluidPlane *plane, s32 N, f32 diffusion, f32 viscosity)
{
    plane->size = N;
    plane->diffusion = diffusion;
    plane->viscosity = viscosity;

    plane->s = arena_allocate_array(gMemoryArena, f32, N * N, default_memory_alloc());
    plane->density = arena_allocate_array(gMemoryArena, f32, N * N, default_memory_alloc());

    plane->velX = arena_allocate_array(gMemoryArena, f32, N * N, default_memory_alloc());
    plane->velY = arena_allocate_array(gMemoryArena, f32, N * N, default_memory_alloc());
    plane->velX0 = arena_allocate_array(gMemoryArena, f32, N * N, default_memory_alloc());
    plane->velY0 = arena_allocate_array(gMemoryArena, f32, N * N, default_memory_alloc());
}

internal void
fluid_init(FluidCube *cube, s32 N, f32 diffusion, f32 viscosity)
{
    cube->size = N;
    cube->diffusion = diffusion;
    cube->viscosity = viscosity;

    cube->s = arena_allocate_array(gMemoryArena, f32, N * N * N, default_memory_alloc());
    cube->density = arena_allocate_array(gMemoryArena, f32, N * N * N, default_memory_alloc());

    cube->velX = arena_allocate_array(gMemoryArena, f32, N * N * N, default_memory_alloc());
    cube->velY = arena_allocate_array(gMemoryArena, f32, N * N * N, default_memory_alloc());
    cube->velZ = arena_allocate_array(gMemoryArena, f32, N * N * N, default_memory_alloc());
    cube->velX0 = arena_allocate_array(gMemoryArena, f32, N * N * N, default_memory_alloc());
    cube->velY0 = arena_allocate_array(gMemoryArena, f32, N * N * N, default_memory_alloc());
    cube->velZ0 = arena_allocate_array(gMemoryArena, f32, N * N * N, default_memory_alloc());
}

internal void
fluid_add_density(FluidPlane *plane, v2s pos, f32 amount)
{
    s32 N = plane->size;
    plane->density[IXP(pos.x, pos.y)] += amount;
}

internal void
fluid_add_density(FluidCube *cube, v3s pos, f32 amount)
{
    s32 N = cube->size;
    cube->density[IXC(pos.x, pos.y, pos.z)] += amount;
}

internal void
fluid_add_velocity(FluidPlane *plane, v2s pos, v2 amount)
{
    s32 N = plane->size;
    s32 index = IXP(pos.x, pos.y);

    plane->velX[index] += amount.x;
    plane->velY[index] += amount.y;
}

internal void
fluid_add_velocity(FluidCube *cube, v3s pos, v3 amount)
{
    s32 N = cube->size;
    s32 index = IXC(pos.x, pos.y, pos.z);

    cube->velX[index] += amount.x;
    cube->velY[index] += amount.y;
    cube->velZ[index] += amount.z;
}

internal void
fluid_step(FluidPlane *plane, f32 dt)
{
    s32 N        = plane->size;
    f32 visc     = plane->viscosity;
    f32 diff     = plane->diffusion;
    f32 *s       = plane->s;
    f32 *density = plane->density;
    f32 *vX      = plane->velX;
    f32 *vY      = plane->velY;
    f32 *vX0     = plane->velX0;
    f32 *vY0     = plane->velY0;

    diffuse_plane(1, vX0, vX, visc, dt, 4, N);
    diffuse_plane(2, vY0, vY, visc, dt, 4, N);

    project_plane(vX0, vY0, vX, vY, 4, N);

    advect_plane(1, vX, vX0, vX0, vY0, dt, N);
    advect_plane(2, vY, vY0, vX0, vY0, dt, N);

    project_plane(vX, vY, vX0, vY0, 4, N);

    diffuse_plane(0, s, density, diff, dt, 4, N);
    advect_plane(0, density, s, vX, vY, dt, N);
}

internal void
fluid_step(FluidCube *cube, f32 dt)
{
    s32 N        = cube->size;
    f32 visc     = cube->viscosity;
    f32 diff     = cube->diffusion;
    f32 *s       = cube->s;
    f32 *density = cube->density;
    f32 *vX      = cube->velX;
    f32 *vY      = cube->velY;
    f32 *vZ      = cube->velZ;
    f32 *vX0     = cube->velX0;
    f32 *vY0     = cube->velY0;
    f32 *vZ0     = cube->velZ0;

    diffuse_cube(1, vX0, vX, visc, dt, 4, N);
    diffuse_cube(2, vY0, vY, visc, dt, 4, N);
    diffuse_cube(3, vZ0, vZ, visc, dt, 4, N);

    project_cube(vX0, vY0, vZ0, vX, vY, 4, N);

    advect_cube(1, vX, vX0, vX0, vY0, vZ0, dt, N);
    advect_cube(2, vY, vY0, vX0, vY0, vZ0, dt, N);
    advect_cube(3, vZ, vZ0, vX0, vY0, vZ0, dt, N);

    project_cube(vX, vY, vZ, vX0, vY0, 4, N);

    diffuse_cube(0, s, density, diff, dt, 4, N);
    advect_cube(0, density, s, vX, vY, vZ, dt, N);
}

internal void
fluid_fade_density(FluidPlane *plane)
{
    s32 N = plane->size;
    for (s32 j = 0; j < N; ++j) {
        for (s32 i = 0; i < N; ++i) {
            plane->density[IXP(i, j)] *= 0.997f;
        }
    }
}

internal void
fluid_render(FluidPlane *plane, Image *image, s32 x, s32 y, s32 scale = 1)
{
    s32 N = plane->size;
    for (s32 j = 0; j < N; ++j) {
        for (s32 i = 0; i < N; ++i) {
            f32 r = plane->velX[IXP(i, j)] / 255.0f;
            f32 s = plane->s[IXP(i, j)] / 255.0f;
            f32 d = plane->density[IXP(i, j)] / 255.0f;
            fill_rectangle(image, x + i * scale, y + j * scale,
                           scale, scale, V4(r, s, d, 1.0f));
        }
    }
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(FluidBase) <= state->memorySize);

    v2 size = V2((f32)image->width, (f32)image->height);
    unused(size);

    FluidBase *fluidBase = (FluidBase *)state->memory;
    if (!state->initialized)
    {
        // fluidBase->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        fluidBase->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        init_perlin_noise(&fluidBase->noise, &fluidBase->randomizer);

        fluid_init(&fluidBase->plane, 512, 0.0f, 0.0f);
        //fluid_init(&fluidBase->cube, 64, 1.0f, 2.0f);

        state->initialized = true;
    }

    s32 scale = 1;
    FluidPlane *fluid = &fluidBase->plane;

    if ((10 <= mouse.pixelPosition.x) && (mouse.pixelPosition.x < 10 + fluid->size * scale) &&
        (10 <= mouse.pixelPosition.y) && (mouse.pixelPosition.y < 10 + fluid->size * scale))
    {
        if (is_down(&mouse, Mouse_Left))
        {
            fluid_add_density(fluid, V2S(mouse.pixelPosition.x - 10, mouse.pixelPosition.y - 10) / scale, 300.0f);
        }

    }

    v2s center = V2S(fluid->size / 2, fluid->size / 2);
    fluid_add_density(fluid, center,
                      random_unilateral(&fluidBase->randomizer) * 400.0f + 100.0f);

    f32 angle = perlin_noise(&fluidBase->noise, fluidBase->t) * F32_PI;
    v2 amount = polar_to_cartesian(10.0f, angle);
    fluid_add_velocity(fluid, center, amount);
    fluidBase->t += 0.05f;

    fluid_step(fluid, dt);
    fluid_fade_density(fluid);

    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    fluid_render(fluid, image, 10, 10, scale);

    fluidBase->prevMousePos = mouse.pixelPosition;
    fluidBase->seconds += dt;
    ++fluidBase->ticks;
    if (fluidBase->seconds > 1.0f)
    {
        fluidBase->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", fluidBase->ticks,
                1000.0f / (f32)fluidBase->ticks);
        fluidBase->ticks = 0;
    }
}
