#include "interface.h"
DRAW_IMAGE(draw_image);

#include "../libberdip/std_file.c"
#include "main.cpp"

#include "../libberdip/drawing.cpp"
#include "ui.h"

struct FXState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    s32 prevMouseScroll;

    b32 check1;
    b32 check2;

    u32 slide1;
    s32 slide2;
    f32 slide3;

    u32 setting1;
    s32 setting2;
    f32 setting3;

    UIState ui;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(FXState) <= state->memorySize);

    v2 size = V2((f32)image->width, (f32)image->height);

    FXState *fxer = (FXState *)state->memory;
    if (!state->initialized)
    {
        // fxer->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        fxer->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);

        init_ui(&fxer->ui, image, 64, static_string("data/output.font"));

        state->initialized = true;
    }

    fxer->ui.mouse = hadamard(mouse.relativePosition, size);
    fxer->ui.mouseScroll = mouse.scroll - fxer->prevMouseScroll;
    // NOTE(michiel): UI buttons only return true if the mouse click is released
    //fxer->ui.clicked = (!mouse.mouseDowns && fxer->prevMouseDown);
    // NOTE(michiel): UI buttons return true if the mouse is down
    fxer->ui.clicked = is_down(&mouse, Mouse_Left);

    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));

    UILayout *layout = ui_begin(&fxer->ui, Layout_Vertical,
                                20, 20, image->width - 40, image->height - 40, 0);

    UILayout *innerLayout = ui_layout(&fxer->ui, layout, Layout_Horizontal, 5);
    if (ui_button_imm(&fxer->ui, innerLayout, "Hallo"))
    {
        fprintf(stdout, "Slide: 1 = %u, 2 = %d, 3 = %f\n", fxer->slide1, fxer->slide2,
                fxer->slide3);
        fill_rectangle(image, 0, 0, image->width, image->height, V4(1, 1, 1, 1));
    }
    if (ui_button_imm(&fxer->ui, innerLayout, "Doop"))
    {
        fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 1, 0, 1));
    }

    UILayout *sliderLayout = ui_layout(&fxer->ui, layout, Layout_Vertical, 5);
    if (ui_slider_imm(&fxer->ui, sliderLayout, &fxer->slide1, 255U))
    {
        fxer->setting1 = fxer->slide1;
        fprintf(stdout, "Unsigned: %u\n", fxer->slide1);
    }
    if (ui_slider_imm(&fxer->ui, sliderLayout, &fxer->slide2, 100))
    {
        fxer->setting2 = fxer->slide2;
        fprintf(stdout, "Integer : %d\n", fxer->slide2);
    }
    if (ui_slider_imm(&fxer->ui, sliderLayout, &fxer->slide3, 1.0f))
    {
        fxer->setting3 = fxer->slide3;
        fprintf(stdout, "Floating: %f\n", fxer->slide3);
    }

    UILayout *bottomLayout = ui_layout(&fxer->ui, layout, Layout_Horizontal, 5);
    if (ui_checkbox_imm(&fxer->ui, bottomLayout, fxer->check1))
    {
        fxer->check1 = !fxer->check1;
    }
    if (ui_checkbox_imm(&fxer->ui, bottomLayout, fxer->check2))
    {
        fxer->check2 = !fxer->check2;
    }

    if (ui_button_imm(&fxer->ui, bottomLayout, "Dap \xCE\xA3\n"))
    {
        fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 1, 1));
    }

    ui_end(&fxer->ui);

    if (keyboard->keys[Key_A].isDown)
    {
        if (fxer->check1)
        {
            fill_rectangle(image, 0, 50, 100, 100, V4(0.2f, 0.2f, 0.2f, 1));
        }
        if (fxer->check2)
        {
            fill_rectangle(image, 0, 150, 100, 100, V4(0.2f, 0.2f, 0.2f, 1));
        }
    }

    if (keyboard->keys[Key_S].isPressed)
    {
        fill_rectangle(image, 0, 250, 100, 100, V4(0.2f, 0.2f, 0.2f, 1));
    }
    else if (keyboard->keys[Key_S].isReleased)
    {
        fill_rectangle(image, 0, 350, 100, 100, V4(0.2f, 0.2f, 0.2f, 1));
    }

    if (fxer->check1)
    {
        v2u offset = V2U(image->width / 2 + fxer->setting2, 0);
        u32 colour = ((        0xFF << 24) |
                      (fxer->setting1 << 16) |
                      (fxer->setting1 <<  8) |
                      (fxer->setting1 <<  0));
        v2u dim = V2U(100, (u32)(150.0f * fxer->setting3 + 50.0f));
        fill_rectangle(image, offset.x, offset.y, dim.x, dim.y, colour);
    }
    else
    {
        v2u offset = V2U(image->width / 2 + fxer->slide2, 0);
        u32 colour = ((        0xFF << 24) |
                      (fxer->slide1 << 16) |
                      (fxer->slide1 <<  8) |
                      (fxer->slide1 <<  0));
        v2u dim = V2U(100, (u32)(150.0f * fxer->slide3 + 50.0f));
        fill_rectangle(image, offset.x, offset.y, dim.x, dim.y, colour);
    }

    fxer->prevMouseScroll = mouse.scroll;
    if (keyboard->lastInput.size)
    {
        fprintf(stdout, "Last in: %.*s\n", STR_FMT(keyboard->lastInput));
    }
    fxer->seconds += dt;
    ++fxer->ticks;
    if (fxer->seconds > 1.0f)
    {
        fxer->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", fxer->ticks,
                1000.0f / (f32)fxer->ticks);
        fxer->ticks = 0;
    }
}
