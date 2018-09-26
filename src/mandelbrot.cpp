#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "random.h"
#include "drawing.cpp"

struct FractalState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    u32 paletteCount;
    v4 *palette;
    
    v2 minDraw;
    v2 maxDraw;
    
    f64 minX;
    f64 minY;
    f64 maxX;
    f64 maxY;
    
    Image mandelbrot;
    
    b32 zoomSelect;
    v2 mouseSelectStart;
    v2 mouseDragStart;
    
    u32 prevMouseDown;
};

internal inline v4
get_mandelbrot(Complex64 c0, u32 paletteCount, v4 *palette, u32 maxIteration = 100)
{
    Complex64 c = {};
    u32 iteration = 0;
    while ((abs(c) < (2.0 * 2.0)) &&
           (iteration < maxIteration))
    {
        c = square(c) + c0;
        ++iteration;
    }
    v4 colour = palette[iteration % paletteCount];
    return colour;
}

internal inline v4
get_mandelbrot(Complex32 c0, u32 paletteCount, v4 *palette, u32 maxIteration = 100)
{
    Complex32 c = {};
    u32 iteration = 0;
    while ((abs(c) < (2.0f * 2.0f)) &&
           (iteration < maxIteration))
    {
        c = square(c) + c0;
        ++iteration;
    }
    v4 colour = palette[iteration % paletteCount];
    return colour;
}

internal inline v4
get_mandelbrot(s32 xInt, s32 yInt, u32 paletteCount, v4 *palette,
               v2 scale = V2(1, 1), u32 maxIteration = 100)
{
    Complex32 c0;
    c0.real = ((f32)xInt / scale.x) * 3.5f - 2.5f;
    c0.imag = ((f32)yInt / scale.y) * 2.0f - 1.0f;
    return get_mandelbrot(c0, paletteCount, palette, maxIteration);
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
            Complex32 point;
            point.real = map((f32)x, work->minDraw.x, work->maxDraw.x,
                             work->min.x, work->max.x);
            point.imag = map((f32)y, work->minDraw.y, work->maxDraw.y,
                             work->min.y, work->max.y);
            
            v4 colour = get_mandelbrot(point, work->paletteCount, work->palette, work->paletteCount);
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
    f32 mX = (maxScreen.x - minScreen.x) * size.y;
    f32 mY = (maxScreen.y - minScreen.y) * size.x;
    if (mX > mY)
    {
        f32 diff = mY / (maxScreen.x - minScreen.x) - size.x;
        min->x -= diff * 0.5f;
        max->x += diff * 0.5f;
    }
    else if (mY > mX)
    {
        f32 diff = mX / (maxScreen.y - minScreen.y) - size.y;
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
    
    u32 tileSize = 50;
    u32 workerEntries = (image->width / tileSize) * (image->height / tileSize);
    i_expect(workerEntries <= MAX_WORK_QUEUE_ENTRIES);
    
    f32 xStep = (max.x - min.x) * (f32)tileSize / (f32)image->width;
    f32 yStep = (max.y - min.y) * (f32)tileSize / (f32)image->height;
    
    DrawMandelbrotWork *workers = allocate_array(DrawMandelbrotWork, workerEntries);
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
    
    deallocate(workers);
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
#endif

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

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(FractalState) <= state->memorySize);
    
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
        f32 colourStep = 0.1f;
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
        fractalState->minX = -2.5;
        fractalState->maxX = 1.0;
        fractalState->minY = -1.0;
        fractalState->maxX = 1.0;
        
        align_min_max(V2(0, 0), V2((f32)image->width, (f32)image->height), &fractalState->minDraw, &fractalState->maxDraw);
        draw_mandelbrot(state->workQueue, &fractalState->mandelbrot, fractalState->minDraw, fractalState->maxDraw,
                        fractalState->paletteCount, fractalState->palette);
        
        state->initialized = true;
        }
    
    draw_image(image, 0, 0, &fractalState->mandelbrot);

    if ((mouse.mouseDowns & Mouse_Left) &&
        !(fractalState->prevMouseDown & Mouse_Left))
    {
        fractalState->zoomSelect = true;
        fractalState->mouseSelectStart = mouse.pixelPosition;
    }
    
    if (mouse.mouseDowns & Mouse_Left)
    {
        v2 min = fractalState->mouseSelectStart;
        v2 max = fractalState->mouseSelectStart;
        if (min.x > mouse.pixelPosition.x)
        {
            min.x = mouse.pixelPosition.x;
        }
        if (max.x < mouse.pixelPosition.x)
        {
            max.x = mouse.pixelPosition.x;
        }
        
        if (min.y > mouse.pixelPosition.y)
        {
            min.y = mouse.pixelPosition.y;
        }
        if (max.y < mouse.pixelPosition.y)
        {
            max.y = mouse.pixelPosition.y;
        }
        
        fill_rectangle(image, min.x, min.y, max.x - min.x, max.y - min.y,
                       V4(0, 0, 0.5f, 0.7f));
    }
    
    if (!(mouse.mouseDowns & Mouse_Left) &&
        (fractalState->prevMouseDown & Mouse_Left))
    {
        i_expect(fractalState->zoomSelect); // TODO(michiel): Remove this
        fractalState->zoomSelect = false;
        
        v2 mouseMin = fractalState->mouseSelectStart;
        v2 mouseMax = fractalState->mouseSelectStart;
        if (mouseMin.x > mouse.pixelPosition.x)
        {
            mouseMin.x = mouse.pixelPosition.x;
        }
        if (mouseMax.x < mouse.pixelPosition.x)
        {
            mouseMax.x = mouse.pixelPosition.x;
        }
        
        if (mouseMin.y > mouse.pixelPosition.y)
        {
            mouseMin.y = mouse.pixelPosition.y;
        }
        if (mouseMax.y < mouse.pixelPosition.y)
        {
            mouseMax.y = mouse.pixelPosition.y;
        }
        
#if 1
        // NOTE(michiel): 32 bit floats
        v2 min = map(mouseMin, V2(0, 0), V2((f32)image->width, (f32)image->height),
                     fractalState->minDraw, fractalState->maxDraw);
        v2 max = map(mouseMax, V2(0, 0), V2((f32)image->width, (f32)image->height),
                     fractalState->minDraw, fractalState->maxDraw);
        align_min_max(V2(0, 0), V2((f32)image->width, (f32)image->height), &min, &max);
        fractalState->minDraw = min;
        fractalState->maxDraw = max;
        
        draw_mandelbrot(state->workQueue, &fractalState->mandelbrot, fractalState->minDraw, fractalState->maxDraw,
                        fractalState->paletteCount, fractalState->palette);
#else
        // NOTE(michiel): 64 bit floats, better zoom level
        f64 minX = map((f64)mouseMin.x, 0.0, (f64)image->width,  fractalState->minX, fractalState->maxX);
        f64 maxX = map((f64)mouseMax.x, 0.0, (f64)image->width,  fractalState->minX, fractalState->maxX);
        f64 minY = map((f64)mouseMin.y, 0.0, (f64)image->height, fractalState->minY, fractalState->maxY);
        f64 maxY = map((f64)mouseMax.y, 0.0, (f64)image->height, fractalState->minY, fractalState->maxY);
        
        fractalState->minX = minX;
        fractalState->maxX = maxX;
        fractalState->minY = minY;
        fractalState->maxY = maxY;
        
        align_min_max(V2(0, 0), V2((f32)image->width, (f32)image->height), &fractalState->minDraw, &fractalState->maxDraw);
        draw_mandelbrot(state->workQueue, &fractalState->mandelbrot,
                        fractalState->minX, fractalState->minY, 
                        fractalState->maxX, fractalState->maxY,
                        fractalState->paletteCount, fractalState->palette);
        #endif
    }
    
    if ((mouse.mouseDowns & Mouse_Right) &&
        !(fractalState->prevMouseDown & Mouse_Right))
    {
        fractalState->mouseDragStart = mouse.pixelPosition;
    }
    
    if (mouse.mouseDowns & Mouse_Right)
    {
        v2 diff = fractalState->mouseDragStart - mouse.pixelPosition;
        fractalState->mouseDragStart = mouse.pixelPosition;
        
        f64 mX = map((f64)diff.x, -(f64)image->width, (f64)image->width,
                     -(fractalState->maxX - fractalState->minX), 
                     (fractalState->maxX - fractalState->minX));
        f64 mY = map((f64)diff.y, -(f64)image->height, (f64)image->height, 
                     -(fractalState->maxY - fractalState->minY), 
                     (fractalState->maxY - fractalState->minY));
        #if 1
        // NOTE(michiel): 32 bit floats
        fractalState->minDraw += V2(mX, mY);
        fractalState->maxDraw += V2(mX, mY);
        
        align_min_max(V2(0, 0), V2((f32)image->width, (f32)image->height), &fractalState->minDraw, &fractalState->maxDraw);
        draw_mandelbrot(state->workQueue, &fractalState->mandelbrot, fractalState->minDraw, fractalState->maxDraw,
                        fractalState->paletteCount, fractalState->palette);
#else
        // NOTE(michiel): 64 bit floats, better zoom level
        fractalState->minX += mX;
        fractalState->maxX += mX;
        fractalState->minY += mY;
        fractalState->maxY += mY;
        
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
        
        fractalState->minX = -2.5;
        fractalState->maxX = 1.0;
        fractalState->minY = -1.0;
        fractalState->maxY = 1.0;
        
        align_min_max(V2(0, 0), V2((f32)image->width, (f32)image->height), &fractalState->minDraw, &fractalState->maxDraw);
        draw_mandelbrot(state->workQueue, &fractalState->mandelbrot, fractalState->minDraw, fractalState->maxDraw,
                        fractalState->paletteCount, fractalState->palette);
    }
    
    fractalState->prevMouseDown = mouse.mouseDowns;
    ++fractalState->ticks;
}
    