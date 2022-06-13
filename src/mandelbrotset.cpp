#include "interface.h"
DRAW_IMAGE(draw_image);

#include "../libberdip/std_file.c"
#include "main.cpp"

#include "../libberdip/drawing.cpp"
#include "ui.h"

struct MandelState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    s32 prevMouseScroll;

    UIState ui;

    b32 grabbedPos;
    v2 pos;

    b32 grabbedConst;
    v2 constant;

    u32 steps;

    b32 showMandel;
    b32 rotateBaseUnit;
    u32 paletteCount;
    v4 *palette;
    v2 minDraw;
    v2 maxDraw;
    Image mandelbrot;
};

struct DrawMandelbrotWork
{
    Image *image;

    Complex32 base;

    v2s minDraw;
    v2s maxDraw;

    v2 min;
    v2 max;

    u32 paletteCount;
    v4 *palette;
};

internal Complex32
complex32_from_v2(v2 v)
{
    Complex32 result;
    result.real = v.x;
    result.imag = v.y;
    return result;
}

internal v2
v2screen_from_complex(Complex32 c, v2 center, f32 scale)
{
    v2 result;
    result.x = c.real;
    result.y = -c.imag;
    result = result * scale + center;
    return result;
}

internal v4
get_mandelbrot(Complex32 base, Complex32 c0, u32 paletteCount, v4 *palette,
               u32 maxIteration = 100, f32 blowUpPoint = 3.0f)
{
    Complex32 c = base;
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
            Complex32 point;
            point.real = map((f32)x, work->minDraw.x, work->maxDraw.x,
                             work->min.x, work->max.x);
            point.imag = map((f32)y, work->minDraw.y, work->maxDraw.y,
                             work->min.y, work->max.y);
            v4 colour = get_mandelbrot(work->base, point, work->paletteCount, work->palette,
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

internal void
draw_mandelbrot(PlatformWorkQueue *queue, Image *image, Complex32 base,
                v2 min, v2 max, u32 paletteCount, v4 *palette)
{
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
            work->base = base;
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
}

#define SCALE     100
#define SCALING   (1.0f / (f32)SCALE)

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(MandelState) <= state->memorySize);

    v2 size = V2((f32)image->width, (f32)image->height);
    v2 center = size * 0.5f;

    MandelState *mandelState = (MandelState *)state->memory;
    if (!state->initialized)
    {
        // basics->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        mandelState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);

        mandelState->paletteCount = 1000;
        mandelState->palette = arena_allocate_array(gMemoryArena, v4, mandelState->paletteCount, default_memory_alloc());

        mandelState->mandelbrot.width = image->width;
        mandelState->mandelbrot.height = image->height;
        mandelState->mandelbrot.rowStride = image->width;
        mandelState->mandelbrot.pixels = arena_allocate_array(gMemoryArena, u32, image->width * image->height, default_memory_alloc());

        v4 lastColour = V4(0, 0, 0, 1);
        u32 redGreenBlue = 0;
        f32 colourStep = 0.2f; // 3.0f / (f32)fractalState->paletteCount;
        for (u32 col = 0; col < mandelState->paletteCount; ++col)
        {
            //f32 gray = (f32)col / (f32)array_count(palette);
            v4 *colour = mandelState->palette + col;
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

        v2 minDraw = -center * SCALING;
        minDraw.y = -minDraw.y;

        v2 maxDraw = (size - center) * SCALING;
        maxDraw.y = -maxDraw.y;

        mandelState->minDraw = minDraw;
        mandelState->maxDraw = maxDraw;
        Complex32 base = {0, 0};
        draw_mandelbrot(state->workQueue, &mandelState->mandelbrot, base,
                        mandelState->minDraw, mandelState->maxDraw,
                        mandelState->paletteCount, mandelState->palette);

        init_ui(&mandelState->ui, image, 32, static_string("data/output.font"));
        mandelState->steps = 1;
        mandelState->constant = V2(-1, 0);
        state->initialized = true;
    }

    mandelState->ui.mouse = hadamard(mouse.relativePosition, size);
    mandelState->ui.mouseScroll = mouse.scroll - mandelState->prevMouseScroll;
    mandelState->ui.clicked = is_down(&mouse, Mouse_Left);

    //
    // NOTE(michiel): Input processing
    //

    v2 at = mouse.pixelPosition;
    at = (at - center) * SCALING;
    at.y = -at.y;

    if (!mandelState->grabbedConst &&
        ((mandelState->constant.x - 5.0f * SCALING) <= at.x) &&
        ((mandelState->constant.x + 5.0f * SCALING) >= at.x) &&
        ((mandelState->constant.y - 5.0f * SCALING) <= at.y) &&
        ((mandelState->constant.y + 5.0f * SCALING) >= at.y))
    {
        if (is_pressed(&mouse, Mouse_Left))
        {
            mandelState->grabbedConst = true;
        }
    }
    else if (!mandelState->grabbedPos &&
             ((mandelState->pos.x - 5.0f * SCALING) <= at.x) &&
             ((mandelState->pos.x + 5.0f * SCALING) >= at.x) &&
             ((mandelState->pos.y - 5.0f * SCALING) <= at.y) &&
             ((mandelState->pos.y + 5.0f * SCALING) >= at.y))
    {
        if (is_pressed(&mouse, Mouse_Left))
        {
            mandelState->grabbedPos = true;
            mandelState->rotateBaseUnit = false;
        }
    }

    if (mandelState->grabbedConst)
    {
        mandelState->constant = at;

        if (!is_down(&mouse, Mouse_Left))
        {
            mandelState->grabbedConst = false;
        }
    }
    else if (mandelState->grabbedPos)
    {
        mandelState->pos = at;

        if (!is_down(&mouse, Mouse_Left))
        {
            mandelState->grabbedPos = false;
        }
    }

    if (mandelState->rotateBaseUnit)
    {
        mandelState->pos = rotate(mandelState->pos, polar_to_cartesian(1.0f, 0.01f));
    }

    if (mandelState->grabbedPos || mandelState->rotateBaseUnit)
    {
        draw_mandelbrot(state->workQueue, &mandelState->mandelbrot, complex32_from_v2(mandelState->pos),
                        mandelState->minDraw, mandelState->maxDraw,
                        mandelState->paletteCount, mandelState->palette);

    }

    //
    // NOTE(michiel): Background
    //
    if (mandelState->showMandel)
    {
        draw_image(image, 0, 0, &mandelState->mandelbrot);
    }
    else
    {
        fill_rectangle(image, 0, 0, image->width, image->height, V4(0.85f, 0.89f, 0.7f, 1));
    }

    //
    // NOTE(michiel): UI Layout
    //
    UILayout *layout = ui_begin(&mandelState->ui, Layout_Horizontal,
                                10, 5, 780, 75, 0);

    UILayout *positions = ui_layout(&mandelState->ui, layout, Layout_Vertical, 0, 2.0f);
    u8 baseBuffer[128];
    String baseStr = string_fmt(sizeof(baseBuffer), baseBuffer, "Base: (%.2f, %.2f)",
                                mandelState->pos.x, mandelState->pos.y);
    ui_text_imm(&mandelState->ui, positions, baseStr);

    u8 constBuffer[128];
    String constStr = string_fmt(sizeof(constBuffer), constBuffer, "Constant: (%.2f, %.2f)",
                                 mandelState->constant.x, mandelState->constant.y);
    ui_text_imm(&mandelState->ui, positions, constStr);

    u8 stepBuffer[128];
    String stepStr = string_fmt(sizeof(stepBuffer), stepBuffer, "Steps: %u",
                                mandelState->steps);
    ui_text_imm(&mandelState->ui, layout, stepStr, 1.3f);

    UILayout *buttons = ui_layout(&mandelState->ui, layout, Layout_Vertical, 0, 0.2f);
    if (ui_button_imm(&mandelState->ui, buttons, "+"))
    {
        if (mandelState->steps < 100) {
            ++mandelState->steps;
        }
    }
    if (ui_button_imm(&mandelState->ui, buttons, "-"))
    {
        if (mandelState->steps > 1) {
            --mandelState->steps;
        }
    }

    UILayout *checks = ui_layout(&mandelState->ui, layout, Layout_Vertical, 0, 0.2f);
    if (ui_checkbox_imm(&mandelState->ui, checks, mandelState->showMandel))
    {
        mandelState->showMandel = !mandelState->showMandel;
    }
    if (ui_checkbox_imm(&mandelState->ui, checks, mandelState->rotateBaseUnit))
    {
        mandelState->rotateBaseUnit = !mandelState->rotateBaseUnit;
        if (mandelState->rotateBaseUnit) {
            mandelState->pos.x = 1.0f;
            mandelState->pos.y = 0.0f;
        }
    }
    ui_end(&mandelState->ui);

    //
    // NOTE(michiel): Axis and Unit circle + text
    //
    draw_line(image, 0, s32_from_f32_round(center.y), image->width, s32_from_f32_round(center.y),
              V4(0, 0, 0, 1));

    draw_line(image, s32_from_f32_round(center.x), 0, s32_from_f32_round(center.x), image->height,
              V4(0, 0, 0, 1));

    outline_circle(image, s32_from_f32_round(center.x), s32_from_f32_round(center.y), SCALE, 2.0f,
                   V4(0, 0, 0, 1));

    draw_text(&mandelState->ui.font, image, s32_from_f32_round(center.x), s32_from_f32_round(center.y),
              static_string("0"), V4(0, 0, 0, 1));
    draw_text(&mandelState->ui.font, image, s32_from_f32_round(center.x - SCALE), s32_from_f32_round(center.y),
              static_string("-1"), V4(0, 0, 0, 1));
    draw_text(&mandelState->ui.font, image, s32_from_f32_round(center.x + SCALE), s32_from_f32_round(center.y),
              static_string("1"), V4(0, 0, 0, 1));
    draw_text(&mandelState->ui.font, image, s32_from_f32_round(center.x), s32_from_f32_round(center.y + SCALE),
              static_string("-1"), V4(0, 0, 0, 1));
    draw_text(&mandelState->ui.font, image, s32_from_f32_round(center.x), s32_from_f32_round(center.y - SCALE),
              static_string("1"), V4(0, 0, 0, 1));

    //
    // NOTE(michiel): Iteration showcase
    //
    Complex32 addComplex = complex32_from_v2(mandelState->constant);
    Complex32 atComplex = complex32_from_v2(mandelState->pos);

    v2 mappedAt = v2screen_from_complex(atComplex, center, SCALE);
    fill_rectangle(image, s32_from_f32_round(mappedAt.x - 5.0f), s32_from_f32_round(mappedAt.y - 5.0f),
                   10, 10, V4(0.2f, 0.2f, 0.2f, 1.0f));

    v2 mappedC = v2screen_from_complex(addComplex, center, SCALE);
    fill_rectangle(image, s32_from_f32_round(mappedC.x - 5.0f), s32_from_f32_round(mappedC.y - 5.0f),
                   10, 10, V4(0.2f, 0.2f, 0.5f, 1.0f));

    v2 prevMapped = mappedAt;
    Complex32 prevCompl = atComplex;
    for (u32 step = 0; step < mandelState->steps; ++step)
    {
        Complex32 sqAtCompl = square(prevCompl);
        sqAtCompl += addComplex;

        v2 mappedSq = v2screen_from_complex(sqAtCompl, center, SCALE);
        fill_rectangle(image, s32_from_f32_round(mappedSq.x - 3.0f), s32_from_f32_round(mappedSq.y - 3.0f),
                       6, 6, V4(0.3f, 0.3f, 0.3f, 1.0f));
        draw_line(image, prevMapped, mappedSq, V4(0.1f, 0.1f, 0.1f, 1.0f));

        prevMapped = mappedSq;
        prevCompl = sqAtCompl;
    }

    //
    // NOTE(michiel): Book keeping
    //
    mandelState->seconds += dt;
    ++mandelState->ticks;
    if (mandelState->seconds > 1.0f)
    {
        mandelState->seconds -= 1.0f;
#if 0
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", mandelState->ticks,
                1000.0f / (f32)mandelState->ticks);
#endif
        mandelState->ticks = 0;
    }
}
