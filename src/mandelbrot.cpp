#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct FractalState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    u32 paletteCount;
    v4 *palette;
    
    v2 minDraw;
    v2 maxDraw;
    
    Image mandelbrot;
    
    v2 mouseSelectStart;
    v2 mouseDragStart;
    
    u32 prevMouseDown;
};

internal inline v4
get_mandelbrot(Complex64 c0, u32 paletteCount, v4 *palette, u32 maxIteration = 100, f64 blowUpPoint = 2.0)
{
    Complex64 c = {};
    u32 iteration = 0;
    f64 blowUp = square(blowUpPoint);
    while ((abs(c) < blowUp) &&
           (iteration < maxIteration))
    {
        c = square(c) + c0;
        ++iteration;
    }
    v4 colour = {};
    colour.a = 1.0f;
    if (iteration != maxIteration)
    {
        colour = palette[iteration % paletteCount];
    }
    
    return colour;
}

internal inline v4
get_mandelbrot(Complex32 c0, u32 paletteCount, v4 *palette, u32 maxIteration = 100, f32 blowUpPoint = 2.0f)
{
    Complex32 c = {};
    u32 iteration = 0;
    f32 blowUp = square(blowUpPoint);
    while ((abs(c) < blowUp) &&
           (iteration < maxIteration))
    {
        c = square(c) + c0;
        ++iteration;
    }
    v4 colour = {};
    colour.a = 1.0f;
    if (iteration != maxIteration)
    {
        colour = palette[iteration % paletteCount];
    }
    
    return colour;
}

struct DrawMandelbrotWork
{
    Image *image;
    
    v2s minDraw;
    v2s maxDraw;
    
    v2 min;
    v2 max;
    
    u32 paletteCount;
    v4 *palette;
};

internal void
draw_mandelbrot(DrawMandelbrotWork *work)
{
    for (s32 y = work->minDraw.y; y < work->maxDraw.y; ++y)
    {
        for (s32 x = work->minDraw.x; x < work->maxDraw.x; ++x)
        {
#if 1
            Complex64 point;
            point.real = map((f64)x, (f64)work->minDraw.x, (f64)work->maxDraw.x,
                             (f64)work->min.x, (f64)work->max.x);
            point.imag = map((f64)y, (f64)work->minDraw.y, (f64)work->maxDraw.y,
                             (f64)work->min.y, (f64)work->max.y);
#else
            Complex32 point;
            point.real = map((f32)x, work->minDraw.x, work->maxDraw.x,
                             work->min.x, work->max.x);
            point.imag = map((f32)y, work->minDraw.y, work->maxDraw.y,
                             work->min.y, work->max.y);
#endif
            v4 colour = get_mandelbrot(point, work->paletteCount, work->palette, 
                                       work->paletteCount);
            draw_pixel(work->image, x, y, colour);
        }
    }
}

internal
PLATFORM_WORK_QUEUE_CALLBACK(draw_mandelbrot_work)
{
    DrawMandelbrotWork *work = (DrawMandelbrotWork *)data;
    
    draw_mandelbrot(work);
}

internal inline void
align_min_max(v2 minScreen, v2 maxScreen, v2 *min, v2 *max)
{
    v2 size = *max - *min;
    f32 width = (maxScreen.x - minScreen.x);
    f32 height = (maxScreen.y - minScreen.y);
    
    f32 mX = height * size.x;
    f32 mY = width * size.y;
    if (mX < mY)
    {
        f32 diff = mY / height - size.x;
        min->x -= diff * 0.5f;
        max->x += diff * 0.5f;
    }
    else if (mY < mX)
    {
        f32 diff = mX / width - size.y;
        min->y -= diff * 0.5f;
        max->y += diff * 0.5f;
    }
}

internal void
draw_mandelbrot(PlatformWorkQueue *queue, Image *image, 
                v2 min, v2 max, u32 paletteCount, v4 *palette)
{
#if 0
    DrawMandelbrotWork work_ = {};
    DrawMandelbrotWork *work = &work_;
    work->image = image;
    work->minDraw = V2S(0, 0);
    work->maxDraw = V2S(image->width, image->height);
    work->min = min;
    work->max = max;
    work->paletteCount = paletteCount;
    work->palette = palette;
    draw_mandelbrot(work);
#else
    Arena tempArena = {0};
    
    u32 tileSize = 50;
    u32 workerEntries = (image->width / tileSize) * (image->height / tileSize);
    i_expect(workerEntries <= MAX_WORK_QUEUE_ENTRIES);
    
    f32 xStep = (max.x - min.x) * (f32)tileSize / (f32)image->width;
    f32 yStep = (max.y - min.y) * (f32)tileSize / (f32)image->height;
    
    DrawMandelbrotWork *workers = arena_allocate_array(&tempArena, DrawMandelbrotWork, workerEntries);
    u32 workerIndex = 0;
    
    f32 yAxis = min.y;
    for (u32 y = 0; y < image->height; y += tileSize)
    {
        f32 xAxis = min.x;
        for (u32 x = 0; x < image->width; x += tileSize)
        {
            DrawMandelbrotWork *work = workers + workerIndex++;
            work->image = image;
            work->minDraw = V2S(x, y);
            work->maxDraw = V2S(x + tileSize, y + tileSize);
            work->min = V2(xAxis, yAxis);
            work->max = V2(xAxis + xStep, yAxis + yStep);
            work->paletteCount = paletteCount;
            work->palette = palette;
            
            platform_add_entry(queue, draw_mandelbrot_work, work);
            xAxis += xStep;
        }
        yAxis += yStep;
    }
    
    platform_complete_all_work(queue);
    
    arena_free(&tempArena);
#endif
}

#if 0
internal void
draw_mandelbrot(Image *image, v2 min, v2 max, u32 paletteCount, v4 *palette)
{
    f32 maxX = (f32)image->width;
    f32 maxY = (f32)image->height;
    f32 xOffset = 0.0f;
    f32 yOffset = 0.0f;
    
    v2 size = max - min;
    f32 mX = maxX * size.y;
    f32 mY = maxY * size.x;
    if (mX > mY)
    {
        f32 newMaxX = mY / size.y;
        xOffset = 0.5f * (maxX - newMaxX);
        maxX = newMaxX;
    }
    else if (mY > mX)
    {
        f32 newMaxY = mX / size.x;
        yOffset = 0.5f * (maxY - newMaxY);
        maxY = newMaxY;
    }
    
    for (s32 y = 0; y < image->height; ++y)
    {
        for (s32 x = 0; x < image->width; ++x)
        {
            Complex32 point;
            point.real = map((f32)x - xOffset, 0.0f, maxX, min.x, max.x);
            point.imag = map((f32)y - yOffset, 0.0f, maxY, min.y, max.y);
            
            v4 colour = get_mandelbrot(point, paletteCount, palette, paletteCount);
            draw_pixel(image, x, y, colour);
        }
    }
}

internal void
draw_mandelbrot(Image *image, f64 minX, f64 minY, f64 maxX, f64 maxY,
                u32 paletteCount, v4 *palette)
{
    f64 maxWidth = (f64)image->width;
    f64 maxHeight = (f64)image->height;
    f64 xOffset = 0.0;
    f64 yOffset = 0.0;
    
    f64 sizeX = maxX - minX;
    f64 sizeY = maxY - minY;
    f64 mX = maxWidth * sizeY;
    f64 mY = maxHeight * sizeX;
    if (mX > mY)
    {
        f64 newMaxX = mY / sizeY;
        xOffset = 0.5 * (maxWidth - newMaxX);
        maxWidth = newMaxX;
    }
    else if (mY > mX)
    {
        f64 newMaxY = mX / sizeX;
        yOffset = 0.5 * (maxHeight - newMaxY);
        maxHeight = newMaxY;
    }
    
    for (s32 y = 0; y < image->height; ++y)
    {
        for (s32 x = 0; x < image->width; ++x)
        {
            Complex64 point;
            point.real = map((f64)x - xOffset, 0.0, maxWidth, minX, maxX);
            point.imag = map((f64)y - yOffset, 0.0, maxHeight, minY, maxY);
            
            v4 colour = get_mandelbrot(point, paletteCount, palette, paletteCount);
            draw_pixel(image, x, y, colour);
        }
    }
}
#endif

internal inline void
draw_mandelbrot(PlatformWorkQueue *queue, FractalState *state)
{
    align_min_max(V2(0, 0), V2((f32)state->mandelbrot.width, (f32)state->mandelbrot.height), 
                  &state->minDraw, &state->maxDraw);
    draw_mandelbrot(queue, &state->mandelbrot, state->minDraw, state->maxDraw,
                    state->paletteCount, state->palette);
    
    f32 zoomLevel = (state->maxDraw.x - state->minDraw.x) / 3.5f;
    fprintf(stdout, "Zoom level: %8.6f | Magnify: %8.6f\n", zoomLevel, 1.0f / zoomLevel);
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(FractalState) <= state->memorySize);
    //v2 size = V2((f32)image->width, (f32)image->height);
    
    FractalState *fractalState = (FractalState *)state->memory;
    if (!state->initialized)
    {
        fractalState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        fractalState->paletteCount = 1000;
        fractalState->palette = allocate_array(v4, fractalState->paletteCount);
        
        fractalState->mandelbrot.width = image->width;
        fractalState->mandelbrot.height = image->height;
        fractalState->mandelbrot.pixels = allocate_array(u32, image->width * image->height);
        
        v4 lastColour = V4(0, 0, 0, 1);
        u32 redGreenBlue = 0;
        f32 colourStep = 0.2f; // 3.0f / (f32)fractalState->paletteCount;
        for (u32 col = 0; col < fractalState->paletteCount; ++col)
        {
            //f32 gray = (f32)col / (f32)array_count(palette);
            v4 *colour = fractalState->palette + col;
            *colour = lastColour;
            if (redGreenBlue == 0)
            {
                lastColour.r += colourStep;
                if (lastColour.r >= 1.0f)
                {
                    lastColour.r = 0.0f;
                    ++redGreenBlue;
                }
            }
            if (redGreenBlue == 1)
            {
                lastColour.g += colourStep;
                if (lastColour.g >= 1.0f)
                {
                    lastColour.g = 0.0f;
                    ++redGreenBlue;
                }
            }
            if (redGreenBlue == 2)
            {
                lastColour.b += colourStep;
                if (lastColour.b >= 1.0f)
                {
                    lastColour.b = 0.0f;
                    redGreenBlue = 0;
                }
            }
        }
        
        fractalState->minDraw = V2(-2.5f, -1.0f);
        fractalState->maxDraw = V2( 1.0f,  1.0f);
        
        draw_mandelbrot(state->workQueue, fractalState);
        
        state->initialized = true;
    }
    
    draw_image(image, 0, 0, &fractalState->mandelbrot);
    
    if ((mouse.mouseDowns & Mouse_Left) &&
        !(fractalState->prevMouseDown & Mouse_Left))
    {
        fractalState->mouseSelectStart = mouse.relativePosition;
    }
    
    if (mouse.mouseDowns & Mouse_Left)
    {
        v2 min = fractalState->mouseSelectStart;
        v2 max = fractalState->mouseSelectStart;
        if (min.x > (f32)mouse.relativePosition.x)
        {
            min.x = (f32)mouse.relativePosition.x;
        }
        if (max.x < (f32)mouse.relativePosition.x)
        {
            max.x = (f32)mouse.relativePosition.x;
        }
        
        if (min.y > (f32)mouse.relativePosition.y)
        {
            min.y = (f32)mouse.relativePosition.y;
        }
        if (max.y < (f32)mouse.relativePosition.y)
        {
            max.y = (f32)mouse.relativePosition.y;
        }
        
        min.x *= image->width;
        max.x *= image->width;
        min.y *= image->height;
        max.y *= image->height;
        
        fill_rectangle(image, min.x, min.y, max.x - min.x, max.y - min.y,
                       V4(0, 0, 0.5f, 0.7f));
    }
    
    if (!(mouse.mouseDowns & Mouse_Left) &&
        (fractalState->prevMouseDown & Mouse_Left))
    {
        v2 mouseMin = fractalState->mouseSelectStart;
        v2 mouseMax = fractalState->mouseSelectStart;
        if (mouseMin.x > (f32)mouse.relativePosition.x)
        {
            mouseMin.x = (f32)mouse.relativePosition.x;
        }
        if (mouseMax.x < (f32)mouse.relativePosition.x)
        {
            mouseMax.x = (f32)mouse.relativePosition.x;
        }
        
        if (mouseMin.y > (f32)mouse.relativePosition.y)
        {
            mouseMin.y = (f32)mouse.relativePosition.y;
        }
        if (mouseMax.y < (f32)mouse.relativePosition.y)
        {
            mouseMax.y = (f32)mouse.relativePosition.y;
        }
        
        // NOTE(michiel): 32 bit floats
        v2 min = map(mouseMin, V2(0, 0), V2(1, 1), fractalState->minDraw, fractalState->maxDraw);
        v2 max = map(mouseMax, V2(0, 0), V2(1, 1), fractalState->minDraw, fractalState->maxDraw);
        fractalState->minDraw = min;
        fractalState->maxDraw = max;
        
        draw_mandelbrot(state->workQueue, fractalState);
        
#if 0
        // NOTE(michiel): 64 bit floats, better zoom level
        f64 minX = map((f64)mouseMin.x, 0.0, (f64)image->width,  fractalState->minX, fractalState->maxX);
        f64 maxX = map((f64)mouseMax.x, 0.0, (f64)image->width,  fractalState->minX, fractalState->maxX);
        f64 minY = map((f64)mouseMin.y, 0.0, (f64)image->height, fractalState->minY, fractalState->maxY);
        f64 maxY = map((f64)mouseMax.y, 0.0, (f64)image->height, fractalState->minY, fractalState->maxY);
        
        fractalState->minX = minX;
        fractalState->maxX = maxX;
        fractalState->minY = minY;
        fractalState->maxY = maxY;
        
        align_min_max(V2(0, 0), size, &fractalState->minDraw, &fractalState->maxDraw);
        draw_mandelbrot(state->workQueue, &fractalState->mandelbrot,
                        fractalState->minX, fractalState->minY, 
                        fractalState->maxX, fractalState->maxY,
                        fractalState->paletteCount, fractalState->palette);
#endif
    }
    
    if ((mouse.mouseDowns & Mouse_Right) &&
        !(fractalState->prevMouseDown & Mouse_Right))
    {
        fractalState->mouseDragStart = mouse.relativePosition;
    }
    
    if (mouse.mouseDowns & Mouse_Right)
    {
        v2 mouseS = map(fractalState->mouseDragStart, V2(0, 0), V2(1, 1), fractalState->minDraw, fractalState->maxDraw);
        v2 mouseP = map(mouse.relativePosition, V2(0, 0), V2(1, 1), fractalState->minDraw, fractalState->maxDraw);
        v2 diff = mouseS - mouseP;
        
        fractalState->mouseDragStart = mouse.relativePosition;
        
        // NOTE(michiel): 32 bit floats
        fractalState->minDraw += diff;
        fractalState->maxDraw += diff;
        
        draw_mandelbrot(state->workQueue, fractalState);
        
#if 0
        // NOTE(michiel): 64 bit floats, better zoom level
        draw_mandelbrot(state->workQueue, &fractalState->mandelbrot,
                        fractalState->minX, fractalState->minY, 
                        fractalState->maxX, fractalState->maxY,
                        fractalState->paletteCount, fractalState->palette);
#endif
    }
    
    if ((mouse.mouseDowns & Mouse_Extended1) &&
        !(fractalState->prevMouseDown & Mouse_Extended1))
    {
        fractalState->minDraw = V2(-2.5f, -1.0f);
        fractalState->maxDraw = V2( 1.0f,  1.0f);
        
        draw_mandelbrot(state->workQueue, fractalState);
    }
    
    fractalState->prevMouseDown = mouse.mouseDowns;
    ++fractalState->ticks;
}
