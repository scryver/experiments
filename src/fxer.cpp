#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "../libberdip/std_file.c"
#include "main.cpp"

#include "drawing.cpp"
#include "ui.h"

struct FXState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    b32 check1;
    b32 check2;
    
    u32 slide1;
    s32 slide2;
    f32 slide3;
    
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
        
        init_ui(&fxer->ui, image, 64, "data/output.font");
        
        state->initialized = true;
    }
    
    fxer->ui.mouse = hadamard(mouse.relativePosition, size);
    // NOTE(michiel): UI buttons only return true if the mouse click is released
    //fxer->ui.clicked = (!mouse.mouseDowns && fxer->prevMouseDown);
    // NOTE(michiel): UI buttons return true if the mouse is down
    fxer->ui.clicked = mouse.mouseDowns;
    
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
    
    UILayout *bottomLayout = ui_layout(&fxer->ui, layout, Layout_Horizontal, 5);
    if (ui_checkbox_imm(&fxer->ui, bottomLayout, fxer->check1))
    {
        fill_rectangle(image, 0, 50, 100, 100, V4(0.2f, 0.2f, 0.2f, 1));
        fxer->check1 = !fxer->check1;
    }
    if (ui_checkbox_imm(&fxer->ui, bottomLayout, fxer->check2))
    {
        fill_rectangle(image, 100, 50, 100, 100, V4(0.2f, 0.2f, 0.2f, 1));
        fxer->check2 = !fxer->check2;
    }
    
    if (ui_button_imm(&fxer->ui, bottomLayout, "Dap"))
    {
        fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 1, 1));
    }
    
    UILayout *sliderLayout = ui_layout(&fxer->ui, layout, Layout_Vertical, 5);
    UISlider *slider1 = ui_slider(&fxer->ui, sliderLayout, fxer->slide1, 200U);
    UISlider *slider2 = ui_slider(&fxer->ui, sliderLayout, fxer->slide2, 100);
    UISlider *slider3 = ui_slider(&fxer->ui, sliderLayout, fxer->slide3, 1.0f);
    
    ui_end(&fxer->ui);

    fxer->slide1 = slider1->value.u;
    fxer->slide2 = slider2->value.i;
    fxer->slide3 = slider3->value.f;

    fxer->prevMouseDown = mouse.mouseDowns;
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
