#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "drawing.cpp"

struct HexCell
{
    v2s v;
};

struct FractionalHexCell
{
    f32 q;
    f32 r;
    f32 s;
};

internal HexCell
hex_cell(s32 q, s32 r)
{
    HexCell result = {};
    result.v = V2S(q, r);
    return result;
}

internal HexCell
hex_cell(s32 q, s32 r, s32 s)
{
    i_expect(q + r + s == 0);
    return hex_cell(q, r);
}

internal s32
get_s(HexCell c)
{
    return -c.v.x - c.v.y;
}

internal inline b32
operator ==(HexCell a, HexCell b)
{
    b32 result = false;
    if ((a.v.x == b.v.x) &&
        (a.v.y == b.v.y))
    {
        result = true;
    }
    return result;
}

internal inline b32
operator !=(HexCell a, HexCell b)
{
    return !(a == b);
}

internal inline HexCell
operator +(HexCell a, HexCell b)
{
    HexCell result = {};
    result.v = a.v + b.v;
    return result;
}

internal inline HexCell
operator -(HexCell a, HexCell b)
{
    HexCell result = {};
    result.v = a.v - b.v;
    return result;
}

internal inline HexCell
operator *(HexCell a, s32 b)
{
    HexCell result = {};
    result.v = a.v * b;
    return result;
}

internal inline s32
hex_length(HexCell c)
{
    s32 result = 0;
    result = (absolute(c.v.x) + absolute(c.v.y) + absolute(get_s(c))) / 2;
    return result;
}

internal inline s32
hex_distance(HexCell a, HexCell b)
{
    s32 result = 0;
    result = hex_length(a - b);
    return result;
}

global HexCell hexDirections[6] = 
{
    hex_cell( 1, 0, -1), hex_cell( 1, -1, 0), hex_cell(0, -1,  1),
    hex_cell(-1, 0,  1), hex_cell(-1,  1, 0), hex_cell(0,  1, -1),
};

internal inline HexCell
hex_direction(s32 direction)
{
    i_expect(-6 < direction);
    i_expect(direction < 6);
    
    return hexDirections[((6 + direction) % 6)];
}

internal inline HexCell
hex_neighbour(HexCell c, s32 direction)
{
    return c + hex_direction(direction);
}

struct Orientation
{
    f32 forward[4];  // NOTE(michiel): Forward conversion to screen coords
    f32 backward[4]; // NOTE(michiel): Backward conversion to hex coords
    f32 startAngle;  // NOTE(michiel): In multiples of TAU32 / 6.0f
};

internal inline Orientation
init_orientation(f32 f0, f32 f1, f32 f2, f32 f3,
            f32 b0, f32 b1, f32 b2, f32 b3, 
            f32 angle)
{
    Orientation result = {};
    result.forward[0] = f0;
    result.forward[1] = f1;
    result.forward[2] = f2;
    result.forward[3] = f3;
    result.backward[0] = b0;
    result.backward[1] = b1;
    result.backward[2] = b2;
    result.backward[3] = b3;
    result.startAngle = angle;
    return result;
}

global Orientation pointyLayout = init_orientation(sqrt(3.0f), sqrt(3.0f) / 2.0f, 
                                                   0.0f, 3.0f / 2.0f,
                                              sqrt(3.0f) / 3.0f, -1.0f / 3.0f, 
                                                   0.0f, 2.0f / 3.0f,
                                              0.5f);

global Orientation flatLayout = init_orientation(3.0f / 2.0f, 0.0f, 
                                                 sqrt(3.0f) / 2.0f, sqrt(3.0f),
                                            2.0f / 3.0f, 0.0f,
                                                 -1.0f / 3.0f, sqrt(3.0f) / 3.0f,
                                              0.0f);

struct Layout
{
    Orientation orientation;
    v2 size;
    v2 origin;
};

internal inline Layout
layout(Orientation orientation, v2 size, v2 origin)
{
    Layout result = {};
    result.orientation = orientation;
    result.size = size;
    result.origin = origin;
    return result;
}

internal v2
hex_to_pixel(Layout *layout, HexCell c)
{
    v2 result = {};
    
    Orientation *m = &layout->orientation;
    result.x = (m->forward[0] * c.v.x + m->forward[1] * c.v.y) * layout->size.x;
    result.y = (m->forward[2] * c.v.x + m->forward[3] * c.v.y) * layout->size.y;
    result += layout->origin;
    return result;
}

internal FractionalHexCell
pixel_to_hex(Layout *layout, v2 p)
{
    FractionalHexCell result = {};
    
    Orientation *m = &layout->orientation;
    v2 pt = p - layout->origin;
    pt.x /= layout->size.x;
    pt.y /= layout->size.y;
    
      result.q = m->backward[0] * pt.x + m->backward[1] * pt.y;
    result.r = m->backward[2] * pt.x + m->backward[3] * pt.y;
    result.s = -result.q - result.r;
    
    return result;
}

internal HexCell
hex_round(FractionalHexCell f)
{
    s32 q = round(f.q);
    s32 r = round(f.r);
    s32 s = round(f.s);
    f32 dq = absolute((f32)q - f.q);
    f32 dr = absolute((f32)r - f.r);
    f32 ds = absolute((f32)s - f.s);
    if ((dq > dr) && (dq > ds))
    {
        q = -r - s;
    }
    else if (dr > ds)
    {
        r = -q - s;
    }
    else
    {
        s = -q - r;
    }
    return hex_cell(q, r, s);
}

internal v2
hex_corner_offset(Layout *layout, s32 corner)
{
    v2 result = layout->size;
    f32 angle = F32_TAU * (layout->orientation.startAngle + (f32)corner) / 6.0f;
    
    result.x *= cos(angle);
    result.y *= sin(angle);
    
    return result;
}

internal void
get_polygon_corners(Layout *layout, HexCell c, v2 *corners)
{
    v2 center = hex_to_pixel(layout, c);
    for (s32 i = 0; i < 6; ++i)
    {
        v2 offset = hex_corner_offset(layout, i);
        corners[i] = center + offset;
    }
}

// TODO(michiel): Rewrite lerp
internal FractionalHexCell
hex_lerp(f32 t, HexCell a, HexCell b)
{
    FractionalHexCell result = {};
    result.q = lerp((f32)a.v.x, t, (f32)b.v.x);
    result.r = lerp((f32)a.v.y, t, (f32)b.v.y);
    result.s = lerp((f32)get_s(a), t, (f32)get_s(b));
    return result;
}

internal FractionalHexCell
hex_lerp(f32 t, FractionalHexCell a, FractionalHexCell b)
{
    FractionalHexCell result = {};
    result.q = lerp(a.q, t, b.q);
    result.r = lerp(a.r, t, b.r);
    result.s = lerp(a.s, t, b.s);
    return result;
}

internal s32
get_hex_line(HexCell a, HexCell b, u32 maxLen, HexCell *line)
{
    s32 N = hex_distance(a, b);
    i_expect(N < maxLen);
    
    FractionalHexCell aNudge;
    aNudge.q = a.v.x + 1e-6f;
    aNudge.r = a.v.y + 1e-6f;
    aNudge.s = get_s(a) - 2e-6f;
    
    FractionalHexCell bNudge;
    bNudge.q = b.v.x + 1e-6f;
    bNudge.r = b.v.y + 1e-6f;
    bNudge.s = get_s(b) - 2e-6f;
    
    f32 step = 1.0f / maximum((f32)N, 1.0f);
    for (s32 i = 0; i <= N; ++i)
    {
        line[i] = hex_round(hex_lerp(step * (f32)i, aNudge, bNudge));
    }
    
    return N + 1;
}

internal u32
create_hexagrid(u32 maxCount, HexCell *cells, s32 radius)
{
    //  * *
    // * * *
    //  * *
    
    u32 count = 0;
    for (s32 s = -radius + 1; s < radius; ++s)
    {
        for (s32 r = -radius + 1; r < radius; ++r)
        {
            for (s32 q = -radius + 1; q < radius; ++q)
            {
                if (s == (-q - r))
                {
                    i_expect(count < maxCount);
                    cells[count++] = hex_cell(q, r, s);
                }
            }
        }
    }
    return count;
}

internal u32
create_trianglegrid(u32 maxCount, HexCell *cells, s32 size, b32 upsideDown = false)
{
    //    *   | * * *
    //   * *  |  * *
    //  * * * |   *
    s32 halfSize = (size + 1) / 2;
    
    u32 count = 0;
        for (s32 r = 0; r <= size; ++r)
        {
        if (upsideDown)
        {
            for (s32 q = 0; q < size - r; ++q)
            {
                    i_expect(count < maxCount);
                cells[count++] = hex_cell(q - halfSize / 2, r - halfSize / 2);
            }
        }
        else
        {
            for (s32 q = size - r; q < size; ++q)
            {
                i_expect(count < maxCount);
                cells[count++] = hex_cell(q - halfSize, r - halfSize - 1);
            }
        }
    }
    
    return count;
}

internal u32
create_rectangulargrid(u32 maxCount, HexCell *cells, s32 width, s32 height)
{
    //  * * *
    //   * * *
    //  * * *
    
    u32 count = 0;
    for (s32 r = 0; r < height; ++r)
    {
        s32 rOffset = trunc(r / 2);
            for (s32 q = -rOffset; q < width - rOffset; ++q)
            {
                i_expect(count < maxCount);
                cells[count++] = hex_cell(q, r, -q-r);
            }
        }
    
    return count;
}

struct HexState
{
    Arena memory;
    
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    u32 maxCellCount;
    u32 hexCellCount;
    HexCell *cells;
    
     HexCell startCell;
    HexCell *line;
    
    RandomList randList;
    
    Layout layout;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(HexState) <= state->memorySize);
    
    v2 size = V2((f32)image->width, (f32)image->height);
    
    HexState *hexer = (HexState *)state->memory;
    if (!state->initialized)
    {
        // hexer->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        hexer->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        hexer->layout.orientation = pointyLayout;
        hexer->layout.size = V2(20.0f, 20.0f);
        hexer->layout.origin = 0.1f * size;
        
        s32 roundSize = 15;
        hexer->maxCellCount = roundSize * roundSize;
        hexer->cells = arena_allocate_array(&hexer->memory, HexCell, hexer->maxCellCount);
        hexer->line = arena_allocate_array(&hexer->memory, HexCell, hexer->maxCellCount);
        //hexer->hexCellCount = create_hexagrid(hexer->maxCellCount, hexer->cells, roundSize);
        //hexer->hexCellCount = create_trianglegrid(hexer->maxCellCount, hexer->cells, roundSize);
        hexer->hexCellCount = create_rectangulargrid(hexer->maxCellCount, hexer->cells, roundSize, roundSize);
        
        // TODO(michiel): Arena variant
        hexer->randList = allocate_rand_list(hexer->maxCellCount);
        //hexer->randList.series = &hexer->randomizer;
        
        v2 mouses[3] = 
        {
            V2(309.0f, 140.0f),
            V2(320.0f, 148.0f),
            V2(369.0f, 114.0f),
        };
        for (u32 mousy = 0; mousy < array_count(mouses); ++mousy)
        {
            v2 m = mouses[mousy];
            FractionalHexCell fract = pixel_to_hex(&hexer->layout, m);
            HexCell rounded = hex_round(fract);
            fprintf(stdout, "Mouse: (%f, %f) | Fract: (%f, %f, %f) | Tile: (%d, %d, %d)\n",
                    m.x, m.y,
                    fract.q, fract.r, fract.s,
                    rounded.v.x, rounded.v.y, get_s(rounded));
        }
        /* 
 Mouse: (309.000000, 140.000000) | Tile: (-2, -4, 6)
Mouse: (320.000000, 148.000000) | Tile: (-2, -4, 6)
Mouse: (369.000000, 114.000000) | Tile: (-2, -4, 6)
*/
        
        TempMemory temp = temporary_memory(&hexer->memory);
        f32 *randWeights = arena_allocate_array(&hexer->memory, f32, hexer->hexCellCount);
        for (u32 hexIndex = 0; hexIndex < hexer->hexCellCount; ++hexIndex)
        {
            randWeights[hexIndex] = random_unilateral(&hexer->randomizer);
        }
        init_rand_list(&hexer->randomizer, &hexer->randList, randWeights, hexer->cells);
        
        destroy_temporary(temp);
        
        state->initialized = true;
    }
    
    HexCell mouseHover = hex_round(pixel_to_hex(&hexer->layout, V2(mouse.pixelPosition)));
    
    if ((mouse.mouseDowns & Mouse_Right) &&
        !(hexer->prevMouseDown & Mouse_Right))
    {
        fprintf(stdout, "Mouse: (%f, %f) | Tile: (%d, %d, %d)\n",
                (f32)mouse.pixelPosition.x, (f32)mouse.pixelPosition.y,
                mouseHover.v.x, mouseHover.v.y, get_s(mouseHover));
    }
    
    if ((mouse.mouseDowns & Mouse_Left) &&
        !(hexer->prevMouseDown & Mouse_Left))
        {
        for (u32 cellIdx = 0; cellIdx < hexer->hexCellCount; ++cellIdx)
        {
            if (mouseHover == hexer->cells[cellIdx])
            {
                hexer->startCell = mouseHover;
                break;
            }
        }
        }
    
    if ((mouse.mouseDowns & Mouse_Extended2) &&
        !(hexer->prevMouseDown & Mouse_Extended2))
    {
        RandomListEntry choice = random_entry(&hexer->randList);
        if (choice.data)
        {
        HexCell *cell = (HexCell *)choice.data;
        hexer->startCell = *cell;
        }
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    for (u32 hexIndex = 0; hexIndex < hexer->hexCellCount; ++hexIndex)
    {
        HexCell *cell = hexer->cells + hexIndex;
        v2 corners[6];
        v4 colour = V4(1, 1, 1, 1);
        
        if (*cell == hexer->startCell)
        {
            colour = V4(0, 1, 0, 1);
        }
        
        if (mouse.mouseDowns & Mouse_Extended1)
        {
            get_polygon_corners(&hexer->layout, *cell, corners);
for (u32 i = 0; i < 6; ++i)
        {
            u32 n = (i + 1) % 6;
            
            v2 a = corners[i];
            v2 b = corners[n];
            draw_line(image, round(a.x), round(a.y), round(b.x), round(b.y),
                      colour);
        }
        }
        else
        {
            v2 center = hex_to_pixel(&hexer->layout, *cell);
            for (u32 i = 0; i < 6; ++i)
            {
                v2 offset = hex_corner_offset(&hexer->layout, i);
                corners[i] = center + offset * 0.9f;
            }
            
        fill_triangle(image, 
                      V2S(round(corners[0].x), round(corners[0].y)),
                      V2S(round(corners[1].x), round(corners[1].y)),
                      V2S(round(corners[2].x), round(corners[2].y)),
                      colour);
fill_triangle(image, 
                      V2S(round(corners[0].x), round(corners[0].y)),
                      V2S(round(corners[2].x), round(corners[2].y)),
                          V2S(round(corners[3].x), round(corners[3].y)),
                          colour);
        fill_triangle(image, 
                      V2S(round(corners[0].x), round(corners[0].y)),
                      V2S(round(corners[3].x), round(corners[3].y)),
                          V2S(round(corners[5].x), round(corners[5].y)),
                          colour);
        fill_triangle(image, 
                      V2S(round(corners[3].x), round(corners[3].y)),
                      V2S(round(corners[4].x), round(corners[4].y)),
                          V2S(round(corners[5].x), round(corners[5].y)),
                          colour);
        }
    }
    
    s32 lineCount = get_hex_line(mouseHover, hexer->startCell, hexer->maxCellCount, hexer->line);
    for (u32 lineHex = 0; lineHex < lineCount; ++lineHex)
    {
        HexCell cell = hexer->line[lineHex];
        v2 corners[6];
        v4 colour = V4(0.5f, 0.5f, 0.9f, 1);
        
        v2 center = hex_to_pixel(&hexer->layout, cell);
        for (u32 i = 0; i < 6; ++i)
        {
            v2 offset = hex_corner_offset(&hexer->layout, i);
            corners[i] = center + offset * 0.5f;
        }
        
        fill_triangle(image, 
                      V2S(round(corners[0].x), round(corners[0].y)),
                      V2S(round(corners[1].x), round(corners[1].y)),
                      V2S(round(corners[2].x), round(corners[2].y)),
                      colour);
        fill_triangle(image, 
                      V2S(round(corners[0].x), round(corners[0].y)),
                      V2S(round(corners[2].x), round(corners[2].y)),
                      V2S(round(corners[3].x), round(corners[3].y)),
                      colour);
        fill_triangle(image, 
                      V2S(round(corners[0].x), round(corners[0].y)),
                      V2S(round(corners[3].x), round(corners[3].y)),
                      V2S(round(corners[5].x), round(corners[5].y)),
                      colour);
        fill_triangle(image, 
                      V2S(round(corners[3].x), round(corners[3].y)),
                      V2S(round(corners[4].x), round(corners[4].y)),
                      V2S(round(corners[5].x), round(corners[5].y)),
                      colour);
    }
    
    
    hexer->prevMouseDown = mouse.mouseDowns;
    hexer->seconds += dt;
    ++hexer->ticks;
    if (hexer->seconds > 1.0f)
    {
        hexer->seconds -= 1.0f;
        fprintf(stderr, "Ticks: %4u | Time: %fms\r", hexer->ticks,
                1000.0f / (f32)hexer->ticks);
        hexer->ticks = 0;
    }
}
