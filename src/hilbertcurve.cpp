#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"
#include "../libberdip/drawing_simd.cpp"

struct HilbertState
{
    MemoryArena arena;

    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    s32 prevScroll;

    u32 count;

    u32 maxOrder;
    u32 order;

    b32 arrayOfStructs;

    u32 maxPointCount;
    u32 pointCount;
    v2 *hilbertPoints;
    v4 *hilbertColours;
    ColouredPoint *points;
};

internal v2
hilbert_position(u32 order, u32 index)
{
    persist v2 points[4] = {
        {0, 0},
        {0, 1},
        {1, 1},
        {1, 0},
    };

    v2 result = points[index & 0x3];


    for (u32 j = 1; j < order; ++j)
    {
        f32 length = 1 << j;
        index >>= 2;
        switch (index & 0x3)
        {
            case 0: {
                result = V2(result.y, result.x);
            } break;

            case 1: {
                result.y += length;
            } break;

            case 2: {
                result.x += length;
                result.y += length;
            } break;

            case 3: {
                result = V2(2.0f * length - 1.0f - result.y, length - 1.0f - result.x);
            } break;

            default: break;
        }
    }

    return result;
}

internal f32
hsv_helper_func(f32 hue, f32 saturation, f32 value, f32 n)
{
    f32 k = modulus(n + hue / 60.0f, 6.0f);
    return value - value * saturation * clamp01(minimum(k, 4.0f - k));
}

internal v4
colour_from_hsv(f32 hue, f32 saturation, f32 value)
{
    v4 result = {0, 0, 0, 1};
#if 0
    f32 chroma = value * saturation;

    f32 h = hue / 60.0f;
    f32 x = chroma * (1.0f - abs(modulus(h, 2) - 1.0f));

    if (h < 0.0f)
    {
        // NOTE(michiel): Do nothing, invalid input
    }
    else if (h <= 1.0f)
    {
        result.rgb = V3(chroma, x, 0);
    }
    else if (h <= 2.0f)
    {
        result.rgb = V3(x, chroma, 0);
    }
    else if (h <= 3.0f)
    {
        result.rgb = V3(0, chroma, x);
    }
    else if (h <= 4.0f)
    {
        result.rgb = V3(0, x, chroma);
    }
    else if (h <= 5.0f)
    {
        result.rgb = V3(x, 0, chroma);
    }
    else if (h <= 6.0f)
    {
        result.rgb = V3(chroma, 0, x);
    }
#else
    result.r = hsv_helper_func(hue, saturation, value, 5.0f);
    result.g = hsv_helper_func(hue, saturation, value, 3.0f);
    result.b = hsv_helper_func(hue, saturation, value, 1.0f);
#endif

    return result;
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(HilbertState) <= state->memorySize);

    v2 size = V2((f32)image->width, (f32)image->height);

    HilbertState *hilbert = (HilbertState *)state->memory;
    if (!state->initialized)
    {
        // hilbert->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        hilbert->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);

        hilbert->maxOrder = 12;
        hilbert->order = 9;

        hilbert->maxPointCount  = (1 << hilbert->maxOrder) * (1 << hilbert->maxOrder);
        hilbert->hilbertPoints  = arena_allocate_array(&hilbert->arena, v2, hilbert->maxPointCount, default_memory_alloc());
        hilbert->hilbertColours = arena_allocate_array(&hilbert->arena, v4, hilbert->maxPointCount, default_memory_alloc());
        hilbert->points = arena_allocate_array(&hilbert->arena, ColouredPoint, hilbert->maxPointCount, default_memory_alloc());

        u32 N = 1 << hilbert->order;
        u32 totalPoints = N * N;

        hilbert->pointCount = 0;
        for (u32 idx = 0; idx < totalPoints; ++idx)
        {
            hilbert->hilbertColours[hilbert->pointCount] = colour_from_hsv(360.0f * idx / (totalPoints - 1.0f), 1.0f, 1.0f);
            hilbert->hilbertPoints[hilbert->pointCount++] = hilbert_position(hilbert->order, idx);

            hilbert->points[idx] = {
                hilbert_position(hilbert->order, idx),
                colour_from_hsv(360.0f * idx / (totalPoints - 1.0f), 1.0f, 1.0f),
            };
        }
        hilbert->pointCount = totalPoints;
        fprintf(stdout, "Pseudo-Hilbert Curve, order %u\n", hilbert->order);

        state->initialized = true;
    }

    fill_rect_simd(image, V2(0, 0), size, V4(0, 0, 0, 1));

    u32 N = 1 << hilbert->order;
    if (hilbert->prevScroll != mouse.scroll)
    {
        hilbert->count = 0;
        hilbert->order = clamp(1, hilbert->order + (mouse.scroll - hilbert->prevScroll), hilbert->maxOrder);
        hilbert->prevScroll = mouse.scroll;

        N = 1 << hilbert->order;
        u32 totalPoints = N * N;

        hilbert->pointCount = 0;
        for (u32 idx = 0; idx < totalPoints; ++idx)
        {
            hilbert->hilbertColours[hilbert->pointCount] = colour_from_hsv(360.0f * idx / (totalPoints - 1.0f), 1.0f, 1.0f);
            hilbert->hilbertPoints[hilbert->pointCount++] = hilbert_position(hilbert->order, idx);

            hilbert->points[idx] = {
                hilbert_position(hilbert->order, idx),
                colour_from_hsv(360.0f * idx / (totalPoints - 1.0f), 1.0f, 1.0f),
            };
        }
        hilbert->pointCount = totalPoints;
        fprintf(stdout, "Pseudo-Hilbert Curve, order %u\n", hilbert->order);
    }

    if (is_pressed(&mouse, Mouse_Left))
    {
        hilbert->arrayOfStructs = !hilbert->arrayOfStructs;
        fprintf(stdout, "Pseudo-Hilbert Curve, %s\n", hilbert->arrayOfStructs ? "array of structs" : "struct of arrays");
    }

    if (hilbert->count > hilbert->pointCount)
    {
        hilbert->count = hilbert->pointCount;
    }

    f32 minSize = minimum(size.x, size.y) / (f32)N;

    v2 offset = {minSize * 0.5f, minSize * 0.5f};
    v2 scale  = V2(minSize, minSize);
    if (hilbert->count)
    {
        if (hilbert->arrayOfStructs)
        {
            draw_lines_simd(image, hilbert->count, hilbert->points, offset + V2(0.5f, 0.5f), scale);
            //draw_lines(image, hilbert->count, hilbert->points, offset + V2(0.5f, 0.5f), scale);
        }
        else
        {
            draw_lines_simd(image, hilbert->count, hilbert->hilbertPoints, hilbert->hilbertColours, offset + V2(0.5f, 0.5f), scale);
            //draw_lines(image, hilbert->count, hilbert->hilbertPoints, hilbert->hilbertColours, offset + V2(0.5f, 0.5f), scale);
        }
    }

    hilbert->count += (1 << (hilbert->order - 1));

    hilbert->seconds += dt;
    ++hilbert->ticks;
    if (hilbert->seconds > 1.0f)
    {
        hilbert->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", hilbert->ticks, 1000.0f / (f32)hilbert->ticks);
        hilbert->ticks = 0;
    }
}

