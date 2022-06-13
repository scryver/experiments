#include "interface.h"
DRAW_IMAGE(draw_image);

#include "../libberdip/std_file.c"
#include "main.cpp"

#include "../libberdip/drawing.cpp"
#include "ui.h"

struct FlowerState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    s32 prevMouseScroll;

    UIState ui;

    f32 stepAt;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(FlowerState) <= state->memorySize);

    v2 size = V2((f32)image->width, (f32)image->height);

    FlowerState *flowers = (FlowerState *)state->memory;
    if (!state->initialized)
    {
        // basics->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        flowers->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);

        init_ui(&flowers->ui, image, 64, static_string("data/output.font"));

        flowers->stepAt = 0.1f;

        state->initialized = true;
    }

    flowers->ui.mouse = hadamard(mouse.relativePosition, size);
    flowers->ui.mouseScroll = mouse.scroll - flowers->prevMouseScroll;
    flowers->ui.clicked = is_released(&mouse, Mouse_Left);

    f32 step = 0.00002f;
    //f32 step = 0.5f + square_root(5) * 0.5f;
    flowers->stepAt += step;
    if (flowers->stepAt > 1.0f) {
        flowers->stepAt = 0.1f;
    }

    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));

    UILayout *layout = ui_begin(&flowers->ui, Layout_Vertical,
                                20, 20, 400, 40, 0);
    u8 stepBuffer[128];
    String stepStr = string_fmt(sizeof(stepBuffer), stepBuffer, "Steps: %f",
                                flowers->stepAt);
    ui_text_imm(&flowers->ui, layout, stepStr);

    ui_end(&flowers->ui);

    v4 flowerColour = V4(1.0f, 0.4f, 0.1f, 1.0f);
    u32 flowerRadius = 4;
    v2 center = 0.5f * size;
    fill_circle(image, s32_from_f32_round(center.x), s32_from_f32_round(center.y),
                flowerRadius, flowerColour);


    f32 magnitude = 4.0f * (f32)flowerRadius;
    f32 angle = 0.0f;
    f32 angleStep = flowers->stepAt * F32_TAU;
    for (u32 i = 0; i < 1000; ++i)
    {
        v2 at = polar_to_cartesian(magnitude, angle);

        at += center;
        fill_circle(image, s32_from_f32_round(at.x), s32_from_f32_round(at.y),
                    flowerRadius, flowerColour);

        angle += angleStep;
        if (angle >= F32_TAU)
        {
            magnitude += (f32)flowerRadius * 2.0f;
            angle -= F32_TAU;
        }
    }

    flowers->seconds += dt;
    ++flowers->ticks;
    if (flowers->seconds > 1.0f)
    {
        flowers->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", flowers->ticks,
                1000.0f / (f32)flowers->ticks);
        flowers->ticks = 0;
    }
}
