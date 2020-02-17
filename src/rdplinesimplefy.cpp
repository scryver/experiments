#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/std_file.c"
#include "../libberdip/drawing.cpp"
#include "ui.h"

struct RDPState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    UIState ui;
    
    f32 epsilon;
    v2 origLine[512];
    u32 rdpLineCount;
    v2 rdpLine[512];
};

internal f32
line_distance(v2 point, v2 start, v2 end)
{
    v2 sp = point - start;
    v2 se = end - start;
    se = normalize(se);
    v2 normal = start + se * dot(sp, se);
    f32 result = length(point - normal);
    return result;
}

internal u32
find_furthest(RDPState *state, v2 *line, u32 startIdx, u32 endIdx)
{
    u32 result = U32_MAX;
    v2 start = line[startIdx];
    v2 end = line[endIdx];
    f32 furthestDist = -1.0f;
    
    for (u32 idx = startIdx + 1; idx < endIdx; ++idx)
    {
        v2 point = line[idx];
        f32 dist = line_distance(point, start, end);
        if (furthestDist < dist)
        {
            furthestDist = dist;
            result = idx;
        }
    }
    
    if (furthestDist < state->epsilon)
    {
        result = U32_MAX;
    }
    
    return result;
}

internal void
rdp(RDPState *state, v2 *line, v2 *rdpLine, u32 startIdx, u32 endIdx)
{
    u32 nextIdx = find_furthest(state, line, startIdx, endIdx);
    if ((nextIdx > startIdx) && (nextIdx < endIdx))
    {
        rdp(state, line, rdpLine, startIdx, nextIdx);
        rdpLine[state->rdpLineCount++] = line[nextIdx];
        rdp(state, line, rdpLine, nextIdx, endIdx);
    }
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(RDPState) <= state->memorySize);
    
    v2 size = V2((f32)image->width, (f32)image->height);
    
    RDPState *rdpState = (RDPState *)state->memory;
    if (!state->initialized)
    {
        // rdpState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        rdpState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        f32 scaleFactor = 1.0f / (f32)array_count(rdpState->origLine);
        for (u32 idx = 0; idx < array_count(rdpState->origLine); ++idx)
        {
            v2 *v = rdpState->origLine + idx;
            f32 x = (f32)idx * scaleFactor * 5.0f;
            f32 y = exp(-x) * cos(F32_TAU * x);
            v->x = (x / 5.0f) * size.width;
            v->y = (-y + 1.0f) * 0.5f * size.height;
        }
        
        init_ui(&rdpState->ui, image, 64, static_string("data/output.font"));
        
        rdpState->epsilon = 0.0f;
        
        state->initialized = true;
    }
    
#if 0    
    flowers->ui.mouse = hadamard(mouse.relativePosition, size);
    flowers->ui.mouseScroll = mouse.scroll - flowers->prevMouseScroll;
    flowers->ui.clicked = (!mouse.mouseDowns && flowers->prevMouseDown);
#endif
    
    rdpState->rdpLineCount = 0;
    rdpState->rdpLine[rdpState->rdpLineCount++] = rdpState->origLine[0];
    rdp(rdpState, rdpState->origLine, rdpState->rdpLine, 0, array_count(rdpState->origLine));
    rdpState->rdpLine[rdpState->rdpLineCount++] = rdpState->origLine[array_count(rdpState->origLine) - 1];
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    draw_lines(image, array_count(rdpState->origLine), rdpState->origLine, V4(0, 0, 1, 1));
    draw_lines(image, rdpState->rdpLineCount, rdpState->rdpLine, V4(1, 1, 0, 1));
    
    UILayout *layout = ui_begin(&rdpState->ui, Layout_Vertical, 
                                20, 20, 400, 40, 0);
    u8 stepBuffer[128];
    String stepStr = string_fmt(sizeof(stepBuffer), stepBuffer, "Epsilon: %7.2f",
                                rdpState->epsilon);
    ui_text_imm(&rdpState->ui, layout, stepStr);
    ui_end(&rdpState->ui);
    
    rdpState->epsilon += 0.1f;
    if (rdpState->epsilon > 20.0f)
    {
        rdpState->epsilon = 0.0f;
    }
    
    rdpState->prevMouseDown = mouse.mouseDowns;
    rdpState->seconds += dt;
    ++rdpState->ticks;
    if (rdpState->seconds > 1.0f)
    {
        rdpState->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", rdpState->ticks,
                1000.0f / (f32)rdpState->ticks);
        rdpState->ticks = 0;
    }
}

