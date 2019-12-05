#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "../libberdip/perlin.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "../libberdip/std_file.c"
#include "main.cpp"

#include "forces.h"
#include "vehicle.h"
#include "flowfield.h"
#include "../libberdip/drawing.cpp"

#include "ui.h"

struct PhaseState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    s32 prevMouseScroll;
    
    UIState ui;
    
    f32 slideR;
    f32 slideL;
    f32 slideC;
    f32 slideVin;
    
    v2 viewSize;
    
    f32 R;
    f32 L;
    f32 C;
    f32 Vin;
    
    FlowField field;
};

internal void
update_phase_state(PhaseState *state)
{
    f32 oneOverLC = 1.0f / (state->L * state->C);
    f32 oneOverRC = 1.0f / (state->R * state->C);
    f32 offset = state->Vin * oneOverRC;
    
    v2 *grid = state->field.grid;
    for (u32 row = 0; row < state->field.size.y; ++row)
    {
        for (u32 col = 0; col < state->field.size.x; ++col)
        {
            f32 V0 = ((f32)col / (f32)state->field.size.x) * state->viewSize.x - 0.5f * state->viewSize.x;
            f32 dV0 = ((f32)row / (f32)state->field.size.y) * state->viewSize.y - 0.5f * state->viewSize.y;
            V0 += 0.001f;
            dV0 += 0.001f;
            f32 ddV0 = offset * dV0 - V0 * oneOverLC - dV0 * oneOverRC;
            grid[row * state->field.size.x + col] = normalize(V2(dV0, ddV0));
        }
    }
}

#if 0
internal void
update_phase_state(PhaseState *state)
{
    f32 oneOverLC = 1.0f / (state->L * state->C);
    f32 RoverL = state->R / state->L;
    f32 offset = state->Vin * oneOverLC;
    
    v2 *grid = state->field.grid;
    for (u32 row = 0; row < state->field.size.y; ++row)
    {
        for (u32 col = 0; col < state->field.size.x; ++col)
        {
            f32 V0 = ((f32)col / (f32)state->field.size.x) * state->viewSize.x - 0.5f * state->viewSize.x;
            f32 dV0 = ((f32)row / (f32)state->field.size.y) * state->viewSize.y - 0.5f * state->viewSize.y;
            f32 ddV0 = offset - V0 * oneOverLC - dV0 * RoverL;
            grid[row * state->field.size.x + col] = normalize(V2(dV0, ddV0));
        }
    }
}
#endif

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(PhaseState) <= state->memorySize);
    PhaseState *phaseSpace = (PhaseState *)state->memory;
    if (!state->initialized)
    {
        phaseSpace->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        
        phaseSpace->R = 1.0f;
        phaseSpace->L = 1.0f;
        phaseSpace->C = 1.0f;
        phaseSpace->Vin = 0.0f;
        
        phaseSpace->slideR = (phaseSpace->R - 0.01f) * 0.5f;
        phaseSpace->slideL = (phaseSpace->L - 0.01f) * 0.5f;
        phaseSpace->slideC = (phaseSpace->C - 0.01f) * 0.5f;
        
        phaseSpace->viewSize = V2(14, 14);
        
        u32 tileSize = 40;
        phaseSpace->field.size = V2U(14, 14);
        //phaseSpace->field.size = V2U(image->width / tileSize, image->height / tileSize);
        phaseSpace->field.tileSize = tileSize;
        phaseSpace->field.grid = allocate_array(v2, phaseSpace->field.size.x * phaseSpace->field.size.y);
        
        init_ui(&phaseSpace->ui, image, 32, static_string("data/output.font"));
        
        state->initialized = true;
    }
    
    FlowField *field = &phaseSpace->field;
    v2 startP = V2(20, ((f32)image->height - (f32)field->tileSize * field->size.y) * 0.5f);
    
    if ((mouse.pixelPosition.x > startP.x) &&
        (mouse.pixelPosition.x < (startP.x + field->size.x * field->tileSize)) &&
        (mouse.pixelPosition.y > startP.y) &&
        (mouse.pixelPosition.y < (startP.y + field->size.y * field->tileSize)) &&
        (mouse.scroll != phaseSpace->prevMouseScroll))
    {
        s32 steps = mouse.scroll - phaseSpace->prevMouseScroll;
        b32 zoomOut = false;
        if (steps < 0)
        {
            zoomOut = true;
            steps = -steps;
        }
        
        for (s32 step = 0; step < steps; ++step)
        {
            if (zoomOut) {
                phaseSpace->viewSize.x *= 1.2f;
                //phaseSpace->viewSize.y *= 1.2f;
            } else {
                phaseSpace->viewSize.x *= 0.9f;
                //phaseSpace->viewSize.y *= 0.9f;
            }
        }
    }
    
    update_phase_state(phaseSpace);
    
    v2 size = V2((f32)image->width, (f32)image->height);
    phaseSpace->ui.mouse = hadamard(mouse.relativePosition, size);
    phaseSpace->ui.mouseScroll = mouse.scroll - phaseSpace->prevMouseScroll;
    phaseSpace->ui.clicked = mouse.mouseDowns;
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    // NOTE(michiel): Grid
    // x = 0
    draw_line(image, startP + V2(field->size.x * 0.5f + 0.5f, 0) * field->tileSize,
              startP + V2(field->size.x * 0.5f + 0.5f, field->size.y) * field->tileSize, V4(0.4f, 0.4f, 0.4f, 1.0f));
    // y = 0
    draw_line(image, startP + V2(0, field->size.y * 0.5f + 0.5f) * field->tileSize,
              startP + V2(field->size.x, field->size.y * 0.5f + 0.5f) * field->tileSize, V4(0.4f, 0.4f, 0.4f, 1.0f));
    
    for (u32 row = 0; row < field->size.y; ++row)
    {
        for (u32 col = 0; col < field->size.x; ++col)
        {
            v2 center = startP + V2(col * (f32)field->tileSize, row * (f32)field->tileSize) + 
                V2(0.5f * (f32)field->tileSize, 0.5f * (f32)field->tileSize);
            v2 target = field->grid[row * field->size.x + col] * 0.5f * (f32)field->tileSize + center;
            
#if 0            
            outline_rectangle(image, round(startP.x) + col * field->tileSize, round(startP.y) + row * field->tileSize,
                              field->tileSize, field->tileSize, V4(0.3f, 0.3f, 0.3f, 1.0f));
#endif
            
            draw_line(image, round(center.x), round(center.y), 
                      round(target.x), round(target.y), V4(0.7f,0.7f,0.7f,1.0f));
            fill_circle(image, round(target.x), round(target.y), 4, V4(0.7f, 0.7f, 0.7f, 1.0f));
        }
    }
    
    // NOTE(michiel): UI
    v2 uiP = startP + V2(field->size.x, 0) * field->tileSize + V2(20, 0);
    UILayout *layout = ui_begin(&phaseSpace->ui, Layout_Horizontal, round(uiP.x), round(uiP.y),
                                image->width - round(uiP.x), image->height - round(uiP.y), 0);
    if (ui_slider_imm(&phaseSpace->ui, layout, &phaseSpace->slideR, 1.0f))
    {
        phaseSpace->R = 2.0f * phaseSpace->slideR + 0.01f;
    }
    if (ui_slider_imm(&phaseSpace->ui, layout, &phaseSpace->slideL, 1.0f))
    {
        phaseSpace->L = 2.0f * phaseSpace->slideL + 0.01f;
    }
    if (ui_slider_imm(&phaseSpace->ui, layout, &phaseSpace->slideC, 1.0f))
    {
        phaseSpace->C = 2.0f * phaseSpace->slideC + 0.01f;
    }
    ui_end(&phaseSpace->ui);
    
    phaseSpace->prevMouseDown = mouse.mouseDowns;
    phaseSpace->prevMouseScroll = mouse.scroll;
    
    phaseSpace->seconds += dt;
    ++phaseSpace->ticks;
    if (phaseSpace->seconds > 1.0f)
    {
        phaseSpace->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", phaseSpace->ticks,
                1000.0f / (f32)phaseSpace->ticks);
        phaseSpace->ticks = 0;
    }
}
