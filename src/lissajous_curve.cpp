#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct Curve
{
    u32 maxPaths;
    u32 pathIndex;
    u32 pathCount;
    v2u *paths;
};

internal void
init_curve(Curve *curve)
{
    i_expect(!curve->paths);
    
    curve->maxPaths = 1024; // NOTE(michiel): Must be a 2^N value
    curve->pathIndex = 0;
    curve->pathCount = 0;
    curve->paths = allocate_array(v2u, curve->maxPaths);
}

internal void
add_point(Curve *curve, u32 x, u32 y)
{
    u32 nextIndex = (curve->pathIndex + 1) & (curve->maxPaths - 1);
    v2u *point = curve->paths + curve->pathIndex;
    point->x = x;
    point->y = y;
    
    if (curve->pathCount < curve->maxPaths)
    {
        ++curve->pathCount;
    }
    
    curve->pathIndex = nextIndex;
}

internal void
draw_curve(Image *image, Curve *curve, v4 colour)
{
    v2u *prevLine = 0;
    s32 linePoints = curve->pathCount;
    while (--linePoints)
    {
        v2u *line = curve->paths + ((curve->pathIndex + linePoints) & (curve->maxPaths - 1));
        if (!prevLine)
        {
            prevLine = line;
        }
        else
        {
            draw_line(image, prevLine->x, prevLine->y, line->x, line->y, colour);
        }
        prevLine = line;
    }
}

struct LissaJousState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    
    f32 angle;
    
    u32 tileW;
    u32 rows;
    u32 columns;
    
    u32 *currentXs;
    u32 *currentYs;
    
    u32 curveCount;
    Curve *curves;
    // u32 prevMouseDown;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(LissaJousState) <= state->memorySize);
    
    //v2 size = V2((f32)image->width, (f32)image->height);
    
    LissaJousState *lissaJous = (LissaJousState *)state->memory;
    if (!state->initialized)
    {
        // lissaJous->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        lissaJous->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        lissaJous->tileW = 80;
        lissaJous->rows = image->height / lissaJous->tileW - 1;
        lissaJous->columns = image->width / lissaJous->tileW - 1;
        
        lissaJous->currentXs = allocate_array(u32, lissaJous->columns);
        lissaJous->currentYs = allocate_array(u32, lissaJous->rows);
        
        lissaJous->curveCount = lissaJous->rows * lissaJous->columns;
        lissaJous->curves = allocate_array(Curve, lissaJous->curveCount);
        for (u32 curveIndex = 0; curveIndex < lissaJous->curveCount; ++curveIndex)
        {
            Curve *curve = lissaJous->curves + curveIndex;
            init_curve(curve);
        }
        
        state->initialized = true;
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    u32 d = lissaJous->tileW - 10;
    f32 r = (f32)d *0.5f;
    
    for (u32 col = 0; col < lissaJous->columns; ++col)
    {
        u32 cx = col * lissaJous->tileW + lissaJous->tileW + lissaJous->tileW / 2;
        u32 cy = lissaJous->tileW / 2;
        outline_circle(image, cx, cy, r, 2.0f, V4(1, 1, 1, 1));
        
        s32 x = s32_from_f32_round(r * cos((f32)(col + 1) * lissaJous->angle - F32_TAU * 0.25f));
        s32 y = s32_from_f32_round(r * sin((f32)(col + 1) * lissaJous->angle - F32_TAU * 0.25f));
        
        fill_circle(image, (s32)cx + x, (s32)cy + y, 4, V4(0, 1, 0, 1));
        
        draw_line(image, cx + x, 0, cx + x, image->height, V4(0, 1, 0, 0.4f));
        
        lissaJous->currentXs[col] = cx + x;
    }
    
    for (u32 row = 0; row < lissaJous->rows; ++row)
    {
        u32 cx = lissaJous->tileW / 2;
        u32 cy = row * lissaJous->tileW + lissaJous->tileW + lissaJous->tileW / 2;
        outline_circle(image, cx, cy, r, 2.0f, V4(1, 1, 1, 1));
        
        s32 x = s32_from_f32_round(r * cos((f32)(row + 1) * lissaJous->angle - F32_TAU * 0.25f));
        s32 y = s32_from_f32_round(r * sin((f32)(row + 1) * lissaJous->angle - F32_TAU * 0.25f));
        
        fill_circle(image, (s32)cx + x, (s32)cy + y, 4, V4(0, 0, 1, 1));
        
        draw_line(image, 0, cy + y, image->width, cy + y, V4(0, 0, 1, 0.4f));
        
        lissaJous->currentYs[row] = cy + y;
    }
    
    Curve *currentCurve = lissaJous->curves;
    for (u32 row = 0; row < lissaJous->rows; ++row)
    {
        u32 y = lissaJous->currentYs[row];
        for (u32 col = 0; col < lissaJous->columns; ++col)
        {
            u32 x = lissaJous->currentXs[col];
            add_point(currentCurve, x, y);
            ++currentCurve;
        }
    }
    
    for (u32 curveIndex = 0; curveIndex < lissaJous->curveCount; ++curveIndex)
    {
        Curve *curve = lissaJous->curves + curveIndex;
        draw_curve(image, curve, V4(0.5f, 0.2f, 0.2f, 1.0f));
    }
    
#if 1
    lissaJous->angle += 0.01f;
    if (lissaJous->angle > F32_TAU)
    {
        lissaJous->angle -= F32_TAU;
    }
#endif
    
    //lissaJous->prevMouseDown = mouse.mouseDowns;
    lissaJous->seconds += dt;
    ++lissaJous->ticks;
}
