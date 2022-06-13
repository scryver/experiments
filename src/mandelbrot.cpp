#include "interface.h"
#include "fixedpoint.h"

DRAW_IMAGE(draw_image);

#define COMPLEX_32  1
#define COMPLEX_64  0
#define COMPLEX_UFP 0

#define MAIN_NUMBER_OF_THREADS 8
#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct ComplexUFP
{
    FixedPoint real;
    FixedPoint imag;
};

struct v2fp
{
    FixedPoint x;
    FixedPoint y;
};

struct DrawMandelbrotWork
{
    Image *image;

    v2s minDraw;
    v2s maxDraw;

#if COMPLEX_UFP
    v2fp min;
    v2fp max;
#elif COMPLEX_64
    v2d min;
    v2d max;
#elif COMPLEX_32
    v2 min;
    v2 max;
#else
#error "No known way to make the mandelbrot without a type"
#endif

    u32 paletteCount;
    v4 *palette;
};

struct FractalState
{
    RandomSeriesPCG randomizer;
    u32 ticks;

    u32 paletteCount;
    v4 *palette;

#if COMPLEX_UFP
    v2fp minDraw;
    v2fp maxDraw;
#elif COMPLEX_64
    v2d minDraw;
    v2d maxDraw;
#else
    v2 minDraw;
    v2 maxDraw;
#endif

    Image mandelbrot;

    v2 mouseSelectStart;
    v2 mouseDragStart;

    u32 prevMouseDown;
};

internal FixedPoint
absolute(ComplexUFP c)
{
    FixedPoint result = {};
    return result;
}

internal ComplexUFP
square(ComplexUFP c)
{
    ComplexUFP result = {};
    return result;
}

internal ComplexUFP
operator +(ComplexUFP a, ComplexUFP b)
{
    ComplexUFP result = {};
    return result;
}

internal inline v4
get_mandelbrot(ComplexUFP c0, u32 paletteCount, v4 *palette, u32 maxIteration = 100)
{
    ComplexUFP c = {};
    u32 iteration = 0;
    FixedPoint blowUp = square(fixed_point(2, 0, array_count(c.real.digits) * 32 - 3));
    while ((absolute(c) < blowUp) &&
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
get_mandelbrot(Complex64 c0, u32 paletteCount, v4 *palette, u32 maxIteration = 100, f64 blowUpPoint = 2.0)
{
    Complex64 c = {};
    u32 iteration = 0;
    f64 blowUp = square(blowUpPoint);
    while ((absolute(c) < blowUp) &&
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
    while ((absolute(c) < blowUp) &&
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

internal void
draw_mandelbrot(DrawMandelbrotWork *work)
{
    for (s32 y = work->minDraw.y; y < work->maxDraw.y; ++y)
    {
        for (s32 x = work->minDraw.x; x < work->maxDraw.x; ++x)
        {
#if COMPLEX_UFP
            ComplexUFP point;
            point.real = map((f64)x, (f64)work->minDraw.x, (f64)work->maxDraw.x,
                             (f64)work->min.x, (f64)work->max.x);
            point.imag = map((f64)y, (f64)work->minDraw.y, (f64)work->maxDraw.y,
                             (f64)work->min.y, (f64)work->max.y);
#elif COMPLEX_64
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
    MemoryArena tempArena = {0};

    u32 tileSize = 50;
    u32 workerEntries = (image->width / tileSize) * (image->height / tileSize);
    i_expect(workerEntries <= MAX_WORK_QUEUE_ENTRIES);

    f32 xStep = (max.x - min.x) * (f32)tileSize / (f32)image->width;
    f32 yStep = (max.y - min.y) * (f32)tileSize / (f32)image->height;

    DrawMandelbrotWork *workers = arena_allocate_array(&tempArena, DrawMandelbrotWork, workerEntries, default_memory_alloc());
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

    arena_deallocate_all(&tempArena);
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

    f64 zoomLevel = (state->maxDraw.x - state->minDraw.x) / 3.5;
    fprintf(stdout, "Zoom level: %10.9f | Magnify: %8.1f\n", zoomLevel, 1.0 / zoomLevel);
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
        fractalState->palette = arena_allocate_array(gMemoryArena, v4, fractalState->paletteCount, default_memory_alloc());

        fractalState->mandelbrot.width = image->width;
        fractalState->mandelbrot.height = image->height;
        fractalState->mandelbrot.rowStride = image->width;
        fractalState->mandelbrot.pixels = arena_allocate_array(gMemoryArena, u32, image->width * image->height, default_memory_alloc());

        v4 lastColour = V4(0, 0, 0, 1);
        u32 redGreenBlue = 0;
        f32 colourStep = 3.0f / (f32)fractalState->paletteCount;
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

    if (is_pressed(&mouse, Mouse_Left))
    {
        fractalState->mouseSelectStart = mouse.relativePosition;
    }

    if (is_down(&mouse, Mouse_Left))
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

    if (is_released(&mouse, Mouse_Left))
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

    if (is_pressed(&mouse, Mouse_Right))
    {
        fractalState->mouseDragStart = mouse.relativePosition;
    }

    if (is_down(&mouse, Mouse_Right))
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

    if (is_pressed(&mouse, Mouse_Extended1))
    {
        fractalState->minDraw = V2(-2.5f, -1.0f);
        fractalState->maxDraw = V2( 1.0f,  1.0f);

        draw_mandelbrot(state->workQueue, fractalState);
    }

    ++fractalState->ticks;
}
