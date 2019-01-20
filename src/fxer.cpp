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
    
    BitmapFont font;
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
        fxer->ui.screen = image;
        
        ApiFile fontFile = read_entire_file("data/output.font");
        i_expect(fontFile.content.size);
        unpack_font(fontFile.content.data, &fxer->font);
        
        state->initialized = true;
    }
    
    fxer->ui.mouse = hadamard(mouse.relativePosition, size);
    fxer->ui.clicked = (!mouse.mouseDowns && fxer->prevMouseDown);
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    UILayout layout = ui_begin_layout(&fxer->ui, 20, 20, image->width - 40, image->height - 40);
    if (ui_button(&fxer->ui, &layout, "Hallo"))
    {
        fill_rectangle(image, 0, 0, image->width, image->height, V4(1, 1, 1, 1));
    }
    if (ui_button(&fxer->ui, &layout, "Doop"))
    {
        fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 1, 0, 1));
    }
    if (ui_button(&fxer->ui, &layout, "Dap"))
    {
        fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 1, 1));
    }
    ui_end_layout(&fxer->ui, &layout);
    
    draw_text(&fxer->font, image, 40, 40, "<Testing 1- 2+>");
    
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
