#include "interface.h"
DRAW_IMAGE(draw_image);

#include "../libberdip/std_file.c"
#include "main.cpp"

#include "../libberdip/drawing.cpp"
#include "ui.h"

struct TextItem
{
    String text;
    b32 collapsed;
};

struct SelectState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    s32 prevMouseScroll;
    
    u32 selectedIndex;
    
    UIState ui;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(SelectState) <= state->memorySize);
    
    v2 size = V2((f32)image->width, (f32)image->height);
    
    SelectState *selector = (SelectState *)state->memory;
    if (!state->initialized)
    {
        // selector->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        selector->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        init_ui(&selector->ui, image, 64, static_string("data/output.font"));
        
        state->initialized = true;
    }
    
    selector->ui.mouse = hadamard(mouse.relativePosition, size);
    selector->ui.mouseScroll = mouse.scroll - selector->prevMouseScroll;
    // NOTE(michiel): UI buttons only return true if the mouse click is released
    //selector->ui.clicked = (!mouse.mouseDowns && selector->prevMouseDown);
    // NOTE(michiel): UI buttons return true if the mouse is down
    selector->ui.clicked = mouse.mouseDowns;
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    UILayout *layout = ui_begin(&selector->ui, Layout_Vertical, 
                                20, 20, image->width - 40, image->height - 40, 0);
    
    UILayout *listLayout = ui_layout(&selector->ui, layout, Layout_Vertical, 2);
    TextItem textList[] = {
        {static_string("Test 1"), false},
        {static_string("YyJjTtipq,`~'=+-_"), false},
        {static_string("Test 2"), false},
        {static_string("Test 3"), false},
        {static_string("Test 4"), false},
        {static_string("Test 5"), true},
        {static_string("Test 6"), false},
    };
    f32 fontAdvance = get_line_advance_for(&selector->ui.font.info) + 12.0f;
    listLayout->maxSize.y = 0;
    for (u32 idx = 0; idx < array_count(textList); ++idx)
    {
        TextItem *item = textList + idx;
        if (item->collapsed)
        {
            //if (ui_checkbox_imm(&selector->ui, listLayout, ">"))
            {
            }
            listLayout->maxSize.y += 10.0f;
        }
        else
        {
            if (ui_text_imm(&selector->ui, listLayout, item->text))
            {
                fprintf(stdout, "List: %.*s\n", STR_FMT(item->text));
            }
            listLayout->maxSize.y += fontAdvance;
        }
    }
    
#if 0    
    UILayout *innerLayout = ui_layout(&selector->ui, layout, Layout_Horizontal, 5);
    if (ui_button_imm(&selector->ui, innerLayout, "Hallo"))
    {
        fprintf(stdout, "Slide: 1 = %u, 2 = %d, 3 = %f\n", selector->slide1, selector->slide2,
                selector->slide3);
        fill_rectangle(image, 0, 0, image->width, image->height, V4(1, 1, 1, 1));
    }
    if (ui_button_imm(&selector->ui, innerLayout, "Doop"))
    {
        fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 1, 0, 1));
    }
    
    UILayout *sliderLayout = ui_layout(&selector->ui, layout, Layout_Vertical, 5);
    if (ui_slider_imm(&selector->ui, sliderLayout, &selector->slide1, 255U))
    {
        selector->setting1 = selector->slide1;
        fprintf(stdout, "Unsigned: %u\n", selector->slide1);
    }
    if (ui_slider_imm(&selector->ui, sliderLayout, &selector->slide2, 100))
    {
        selector->setting2 = selector->slide2;
        fprintf(stdout, "Integer : %d\n", selector->slide2);
    }
    if (ui_slider_imm(&selector->ui, sliderLayout, &selector->slide3, 1.0f))
    {
        selector->setting3 = selector->slide3;
        fprintf(stdout, "Floating: %f\n", selector->slide3);
    }
    
    UILayout *bottomLayout = ui_layout(&selector->ui, layout, Layout_Horizontal, 5);
    if (ui_checkbox_imm(&selector->ui, bottomLayout, selector->check1))
    {
        selector->check1 = !selector->check1;
    }
    if (ui_checkbox_imm(&selector->ui, bottomLayout, selector->check2))
    {
        selector->check2 = !selector->check2;
    }
    
    if (ui_button_imm(&selector->ui, bottomLayout, "Dap \xCE\xA3\n"))
    {
        fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 1, 1));
    }
#endif
    
    ui_end(&selector->ui);
    
#if 0    
    if (keyboard->keys[Key_A].isDown)
    {
        if (selector->check1)
        {
            fill_rectangle(image, 0, 50, 100, 100, V4(0.2f, 0.2f, 0.2f, 1));
        }
        if (selector->check2)
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
    
    if (selector->check1)
    {
        v2u offset = V2U(image->width / 2 + selector->setting2, 0);
        u32 colour = ((        0xFF << 24) |
                      (selector->setting1 << 16) |
                      (selector->setting1 <<  8) |
                      (selector->setting1 <<  0));
        v2u dim = V2U(100, (u32)(150.0f * selector->setting3 + 50.0f));
        fill_rectangle(image, offset.x, offset.y, dim.x, dim.y, colour);
    }
    else
    {
        v2u offset = V2U(image->width / 2 + selector->slide2, 0);
        u32 colour = ((        0xFF << 24) |
                      (selector->slide1 << 16) |
                      (selector->slide1 <<  8) |
                      (selector->slide1 <<  0));
        v2u dim = V2U(100, (u32)(150.0f * selector->slide3 + 50.0f));
        fill_rectangle(image, offset.x, offset.y, dim.x, dim.y, colour);
    }
#endif
    
    selector->prevMouseDown = mouse.mouseDowns;
    selector->prevMouseScroll = mouse.scroll;
    if (keyboard->lastInput.size)
    {
        fprintf(stdout, "Last in: %.*s\n", STR_FMT(keyboard->lastInput));
    }
    selector->seconds += dt;
    ++selector->ticks;
    if (selector->seconds > 1.0f)
    {
        selector->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", selector->ticks,
                1000.0f / (f32)selector->ticks);
        selector->ticks = 0;
    }
}

