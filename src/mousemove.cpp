#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct MouseState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    v2 pixelCoordsMin;
    v2 pixelCoordsMax;
    v2 virtualCoordsMin;
    v2 virtualCoordsMax;
    
    u32 prevMouseDown;
    v2 mouseSelectStart;
    v2 mouseDragStart;
};

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
        f32 diff = (width * size.y) / (maxScreen.y - minScreen.y) - size.x;
        min->x -= diff * 0.5f;
        max->x += diff * 0.5f;
    }
    else if (mY < mX)
    {
        f32 diff = (height * size.x) / (maxScreen.x - minScreen.x) - size.y;
        min->y -= diff * 0.5f;
        max->y += diff * 0.5f;
    }
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(MouseState) <= state->memorySize);
    v2 size = V2((f32)image->width, (f32)image->height);
    
    MouseState *mouseState = (MouseState *)state->memory;
    if (!state->initialized)
    {
        mouseState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        
        mouseState->pixelCoordsMin = V2(0, 0);
        mouseState->pixelCoordsMax = V2((f32)image->width, (f32)image->height);
        
        mouseState->virtualCoordsMin = V2(-1, -1);
        mouseState->virtualCoordsMax = V2(1, 1);
        
        v2 min = V2(-0.5, -0.5);
        v2 max = V2(0.5, 0.5);
        
        v2 minInt = map(min, mouseState->virtualCoordsMin, mouseState->virtualCoordsMax,
                        mouseState->pixelCoordsMin, mouseState->pixelCoordsMax);
        v2 maxInt = map(max, mouseState->virtualCoordsMin, mouseState->virtualCoordsMax,
                        mouseState->pixelCoordsMin, mouseState->pixelCoordsMax);
        f32 width = maxInt.x - minInt.x;
        f32 height = maxInt.y - minInt.y;
        fprintf(stderr, "Mapped:\n\t%f -> %f\n\t%f -> %f\n\t%f -> %f\n\t%f -> %f\n",
                min.x, minInt.x, min.y, minInt.y, max.x, maxInt.x, max.y, maxInt.y);
        fprintf(stderr, "Width: %f, Height: %f\n", width, height);
        
        state->initialized = true;
    }
    
    align_min_max(V2(0, 0), size, &mouseState->virtualCoordsMin, &mouseState->virtualCoordsMax);
    
    v2 minRect = V2(-0.5, -0.5);
    v2 maxRect = V2(0.5, 0.5);
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    v2 minRectInt = map(minRect, mouseState->virtualCoordsMin, mouseState->virtualCoordsMax,
                        mouseState->pixelCoordsMin, mouseState->pixelCoordsMax);
    v2 maxRectInt = map(maxRect, mouseState->virtualCoordsMin, mouseState->virtualCoordsMax,
                        mouseState->pixelCoordsMin, mouseState->pixelCoordsMax);
    f32 width = maxRectInt.x - minRectInt.x;
    f32 height = maxRectInt.y - minRectInt.y;
    
    fill_rectangle(image, minRectInt.x, minRectInt.y, width, height, V4(1, 1, 1, 1));
    
    if ((mouse.mouseDowns & Mouse_Left) &&
        !(mouseState->prevMouseDown & Mouse_Left))
    {
        mouseState->mouseSelectStart = mouse.pixelPosition;
    }
    
    if (mouse.mouseDowns & Mouse_Left)
    {
        v2 min = mouseState->mouseSelectStart;
        v2 max = mouseState->mouseSelectStart;
        if (min.x > (f32)mouse.pixelPosition.x)
        {
            min.x = (f32)mouse.pixelPosition.x;
        }
        if (max.x < (f32)mouse.pixelPosition.x)
        {
            max.x = (f32)mouse.pixelPosition.x;
        }
        if (min.y > (f32)mouse.pixelPosition.y)
        {
            min.y = (f32)mouse.pixelPosition.y;
        }
        if (max.y < (f32)mouse.pixelPosition.y)
        {
            max.y = (f32)mouse.pixelPosition.y;
        }
        
        fill_rectangle(image, min.x, min.y, max.x - min.x, max.y - min.y,
                       V4(0, 0, 0.5f, 0.7f));
    }
    
    if (!(mouse.mouseDowns & Mouse_Left) &&
        (mouseState->prevMouseDown & Mouse_Left))
    {
        v2 mouseMin = mouseState->mouseSelectStart;
        v2 mouseMax = mouseState->mouseSelectStart;
        if (mouseMin.x > (f32)mouse.pixelPosition.x)
        {
            mouseMin.x = (f32)mouse.pixelPosition.x;
        }
        if (mouseMax.x < (f32)mouse.pixelPosition.x)
        {
            mouseMax.x = (f32)mouse.pixelPosition.x;
        }
        if (mouseMin.y > (f32)mouse.pixelPosition.y)
        {
            mouseMin.y = (f32)mouse.pixelPosition.y;
        }
        if (mouseMax.y < (f32)mouse.pixelPosition.y)
        {
            mouseMax.y = (f32)mouse.pixelPosition.y;
        }
        
        // NOTE(michiel): 32 bit floats
        v2 min = map(mouseMin, V2(0, 0), size,
                     mouseState->virtualCoordsMin, mouseState->virtualCoordsMax);
        v2 max = map(mouseMax, V2(0, 0), size,
                     mouseState->virtualCoordsMin, mouseState->virtualCoordsMax);
        mouseState->virtualCoordsMin = min;
        mouseState->virtualCoordsMax = max;
    }
    
    if ((mouse.mouseDowns & Mouse_Right) &&
        !(mouseState->prevMouseDown & Mouse_Right))
    {
        mouseState->mouseDragStart = mouse.pixelPosition;
    }
    
    if (mouse.mouseDowns & Mouse_Right)
    {
        v2 mouseS = map(mouseState->mouseDragStart, 
                        V2(0, 0), size, 
                        mouseState->virtualCoordsMin, mouseState->virtualCoordsMax);
        v2 mouseP = map(mouse.pixelPosition,
                        V2(0, 0), size, 
                        mouseState->virtualCoordsMin, mouseState->virtualCoordsMax);
        v2 diff = mouseS - mouseP;
        
        mouseState->mouseDragStart = mouse.pixelPosition;
        
        mouseState->virtualCoordsMin += diff;
        mouseState->virtualCoordsMax += diff;
    }
    
    if ((mouse.mouseDowns & Mouse_Extended1) &&
        !(mouseState->prevMouseDown & Mouse_Extended1))
    {
        mouseState->virtualCoordsMin = V2(-1.0f, -1.0f);
        mouseState->virtualCoordsMax = V2( 1.0f,  1.0f);
    }
    
    
    mouseState->prevMouseDown = mouse.mouseDowns;
    ++mouseState->ticks;
}
