#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct Edge
{
    v2 *a;
    v2 *b;
    v2 h1a;
    v2 h1b;
    v2 h2a;
    v2 h2b;
};

struct Polygon
{
    u32 sides;
    v2 *points;
    Edge *edges;
};

struct StarPatternState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    s32 prevMouseScroll;

    f32 angle;
    f32 delta;

    Polygon poly;
};

internal void
init_polygon(Polygon *poly)
{
    f32 oneOverSides = 1.0f / (f32)poly->sides;
    v2 *prevPoint = 0;
    for (u32 pointIndex = 0; pointIndex < poly->sides; ++pointIndex)
    {
        v2 *point = poly->points + pointIndex;
        point->x = cos_f32((f32)pointIndex * oneOverSides);
        point->y = sin_f32((f32)pointIndex * oneOverSides);
        if (prevPoint)
        {
            Edge *edge = poly->edges + pointIndex;
            edge->a = prevPoint;
            edge->b = point;
        }

        prevPoint = point;
    }
    poly->edges[0].a = prevPoint;
    poly->edges[0].b = poly->points + 0;
}

internal void
calculate_hankins(Polygon *poly, f32 angle, f32 delta)
{
    for (u32 edgeIndex = 0; edgeIndex < poly->sides; ++edgeIndex)
    {
        Edge *edge = poly->edges + edgeIndex;
        v2 mid = 0.5f * (*edge->a + *edge->b);

        v2 vec1 = *edge->a - mid;
        v2 vec2 = *edge->b - mid;

        f32 edgeLen = length(vec1) + delta;

        v2 offset1 = mid;
        v2 offset2 = mid;
        if (delta > 0.0f)
        {
            offset1 = mid + delta * vec2;
            offset2 = mid - delta * vec1;
        }

        vec1 = rotate(normalize(vec1), -angle);
        vec2 = rotate(normalize(vec2), angle);

        f32 interior = ((f32)poly->sides - 2.0f) * F32_PI / (f32)poly->sides;
        f32 alpha = 0.5f * interior;
        f32 beta = F32_PI - angle - alpha;
        f32 hlen = (edgeLen * sin_pi(alpha)) / sin_pi(beta);

        edge->h1a = offset1;
        edge->h1b = offset1 + hlen * vec1;
        edge->h2a = offset2;
        edge->h2b = offset2 + hlen * vec2;
    }
}

internal v3
barycentric(v2 p, v2 a, v2 b, v2 c)
{
    v3 result;
    f32 oneOverDen = 1.0f / ((b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y));
    result.x = ((b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y)) * oneOverDen;
    result.y = ((c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y)) * oneOverDen;
    result.z = 1.0f - result.x - result.y;
    return result;
}

internal v4
barycentric_edgeoffsets(v2 p, v2 a, v2 b, v2 c,
                        f32 normalizeFactor, f32 modA, f32 modB, f32 modC)
{
    // NOTE(michiel): Barycentric coordinates in xyz and alpha in w (or a)
    // This function will discard clockwise winding triangles
    // TODO(michiel): Move to drawing3d where it should matter
    v4 result;

    f32 cXsubbX = c.x - b.x;
    f32 bYsubcY = b.y - c.y;
    f32 aXsubcX = a.x - c.x;
    //f32 aYsubcY = a.y - c.y; // == -(c.y - a.y)
    f32 cYsubaY = c.y - a.y;
    f32 pXsubcX = p.x - c.x;
    f32 pYsubcY = p.y - c.y;

    f32 mulcapx = cYsubaY * pXsubcX;
    f32 mulacpy = aXsubcX * pYsubcY;

    result.x = (bYsubcY * pXsubcX + cXsubbX * pYsubcY) * normalizeFactor;
    result.y = (mulcapx + mulacpy) * normalizeFactor;
    result.z = 1.0f - result.x - result.y;

    f32 edgeX = (a.x - b.x) * (p.y - a.y) - (a.y - b.y) * (p.x - a.x);
    f32 edgeY = -cXsubbX * (p.y - b.y) - bYsubcY * (p.x - b.x);
    f32 edgeZ = -mulacpy - mulcapx;

    f32 minP = edgeX;
    f32 modP = modA;
    if (minP > edgeY)
    {
        minP = edgeY;
        modP = modB;
    }
    if (minP > edgeZ)
    {
        minP = edgeZ;
        modP = modC;
    }
    if (modP == 0.0f)
    {
        result.a = 0.0f;
    }
    else
    {
        result.a = clamp01(1.0f + minP / modP);
    }

    return result;
}

internal void
fill_triangle2(Image *image, v2 a, v2 b, v2 c, v4 colourA, v4 colourB, v4 colourC)
{
    f32 minX = minimum(a.x, minimum(b.x, c.x));
    f32 maxX = maximum(a.x, maximum(b.x, c.x));
    f32 minY = minimum(a.y, minimum(b.y, c.y));
    f32 maxY = maximum(a.y, maximum(b.y, c.y));

    f32 modA = maximum(absolute(a.x - b.x), absolute(a.y - b.y));
    f32 modB = maximum(absolute(b.x - c.x), absolute(b.y - c.y));
    f32 modC = maximum(absolute(c.x - a.x), absolute(c.y - a.y));
    f32 normalizer = 1.0f / ((b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y));

    s32 xMin = s32_from_f32_truncate(minX);
    s32 yMin = s32_from_f32_truncate(minY);
    s32 xMax = s32_from_f32_ceil(maxX);
    s32 yMax = s32_from_f32_ceil(maxY);

    xMin = clamp(0, xMin, (s32)image->width);
    yMin = clamp(0, yMin, (s32)image->height);
    xMax = clamp(0, xMax, (s32)image->width);
    yMax = clamp(0, yMax, (s32)image->height);

    for (s32 y = yMin; y < yMax; ++y)
    {
        for (s32 x = xMin; x < xMax; ++x)
        {
            v4 pixel;

            v2 point = V2(x, y);
            v4 baryEdge = barycentric_edgeoffsets(point, a, b, c, normalizer, modA, modB, modC);
            //v4 baryEdge = barycentric_alpha(point, a, b, c, normalizer, modA, modB, modC);

            pixel = baryEdge.x * colourA + baryEdge.y * colourB + baryEdge.z * colourC;
            pixel.a *= baryEdge.a;
            pixel.rgb *= pixel.a;
            pixel = clamp01(pixel);

            draw_pixel(image, x, y, pixel);
        }
    }
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(StarPatternState) <= state->memorySize);

    v2 size = V2((f32)image->width, (f32)image->height);

    StarPatternState *starState = (StarPatternState *)state->memory;
    if (!state->initialized)
    {
        // starState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        starState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);

        starState->poly.sides = 6;
        starState->poly.points = arena_allocate_array(gMemoryArena, v2, starState->poly.sides, default_memory_alloc());
        starState->poly.edges = arena_allocate_array(gMemoryArena, Edge, starState->poly.sides, default_memory_alloc());
        init_polygon(&starState->poly);

        starState->angle = 0.2f * F32_TAU;

        state->initialized = true;
    }

    if (starState->prevMouseScroll != mouse.scroll)
    {
        f32 max = 0.25f * F32_TAU;
        s32 diff = mouse.scroll - starState->prevMouseScroll;
        f32 step = 1.0f / 200.0f * F32_TAU;
        starState->angle += diff * step;
        if (starState->angle > max)
        {
            starState->angle = max;
        }
        if (starState->angle < 0.0f)
        {
            starState->angle = 0.0f;
        }
    }

    calculate_hankins(&starState->poly, starState->angle, starState->delta);

    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));

    if (starState->poly.sides == 4)
    {
        u32 stepSize = 100;
        //u32 x = 2 * stepSize;
        //u32 y = 2 * stepSize;
        for (u32 y = stepSize / 2; y < image->height; y += stepSize)
        {
            for (u32 x = stepSize / 2; x < image->width; x += stepSize)
            {
                for (u32 edgeIndex = 0; edgeIndex < starState->poly.sides; ++edgeIndex)
                {
                    Edge *edge = starState->poly.edges + edgeIndex;
                    v2 a = *edge->a;
                    v2 b = *edge->b;
                    v2 center = V2((f32)x, (f32)y);
                    a = (f32)stepSize * a + center;
                    b = (f32)stepSize * b + center;
                    //draw_line(image, a, b, V4(1, 1, 1, 1));
                    draw_line(image, (f32)stepSize * edge->h1a + center, (f32)stepSize * edge->h1b + center, V4(1, 0, 0, 1));
                    draw_line(image, (f32)stepSize * edge->h2a + center, (f32)stepSize * edge->h2b + center, V4(1, 1, 0, 1));
                }
            }
        }
    }
    else if (starState->poly.sides == 6)
    {
        u32 stepSize = 100;
        u32 ySteps = u32_from_f32_round(2.0f * (f32)stepSize * sin_pi((f32)deg2rad(60.0f)));
        for (u32 y = stepSize / 2; y < image->height; y += ySteps)
        {
            for (u32 x = stepSize / 2; x < image->width; x += 3 * stepSize)
            {
                for (u32 edgeIndex = 0; edgeIndex < starState->poly.sides; ++edgeIndex)
                {
                    Edge *edge = starState->poly.edges + edgeIndex;
                    v2 a = *edge->a;
                    v2 b = *edge->b;
                    v2 center = V2((f32)x, (f32)y);
                    a = (f32)stepSize * a + center;
                    b = (f32)stepSize * b + center;
                    //draw_line(image, a, b, V4(1, 1, 1, 1));
                    draw_line(image, (f32)stepSize * edge->h1a + center, (f32)stepSize * edge->h1b + center, V4(1, 0, 0, 1));
                    draw_line(image, (f32)stepSize * edge->h2a + center, (f32)stepSize * edge->h2b + center, V4(1, 1, 0, 1));
                }
            }
        }
        for (u32 y = (stepSize + ySteps) / 2; y < image->height; y += ySteps)
        {
            for (u32 x = 2 * stepSize; x < image->width; x += 3 * stepSize)
            {
                for (u32 edgeIndex = 0; edgeIndex < starState->poly.sides; ++edgeIndex)
                {
                    Edge *edge = starState->poly.edges + edgeIndex;
                    v2 a = *edge->a;
                    v2 b = *edge->b;
                    v2 center = V2((f32)x, (f32)y);
                    a = (f32)stepSize * a + center;
                    b = (f32)stepSize * b + center;
                    //draw_line(image, a, b, V4(1, 1, 1, 1));
                    draw_line(image, (f32)stepSize * edge->h1a + center, (f32)stepSize * edge->h1b + center, V4(1, 0, 0, 1));
                    draw_line(image, (f32)stepSize * edge->h2a + center, (f32)stepSize * edge->h2b + center, V4(1, 1, 0, 1));
                }
            }
        }
    }

#if 0
    // NOTE(michiel): Backward triangle
    fill_triangle2(image, V2(0.6f * size.x, 0.9f * size.y), V2(0, 0.7f * size.y), V2(0.3f * size.x, 0.1f * size.y), V4(1, 0, 0, 1), V4(0, 1, 0, 1), V4(0, 0, 1, 1));
#else
    fill_triangle2(image, V2(0, 0.7f * size.y), V2(0.6f * size.x, 0.9f * size.y), V2(0.3f * size.x, 0.1f * size.y), V4(1, 0, 0, 1), V4(0, 1, 0, 1), V4(0, 0, 1, 1));
#endif

    fill_triangle2(image, V2(0.9f * size.x, 0.7f * size.y), V2(0.01f+0.9f * size.x, 0.7f * size.y), V2(0.8f * size.x, 0.1f * size.y), V4(1, 0, 0, 1), V4(0, 1, 0, 1), V4(0, 0, 1, 1));
    fill_triangle2(image, V2(0.8f * size.x, 0.1f * size.y), V2(0.8f * size.x - 0.01f, 0.1f * size.y), V2(0.9f * size.x, 0.7f * size.y), V4(0, 0, 1, 1), V4(1, 1, 1, 1), V4(1, 0, 0, 1));

    starState->prevMouseScroll = mouse.scroll;
    starState->seconds += dt;
    ++starState->ticks;
    if (starState->seconds > 1.0f)
    {
        starState->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", starState->ticks,
                1000.0f / (f32)starState->ticks);
        starState->ticks = 0;
    }
}

