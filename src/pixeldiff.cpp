#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct BasicState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    Image a;
    Image b;
    
    u32 rectangleCount;
    Rectangle2u rectangles[1024];
    
    f32 tickAt;
    u32 atY;
};

internal u32
find_diff_rectangles(Image *a, Image *b, u32 maxRectCount, Rectangle2u *rectangles, u32 pixelBorder = 1)
{
    // NOTE(michiel): This function will return the amount of difference rectangles found.
    // The rectangles array will be filled accordingly (up till maxRectCount).
    
    // NOTE(michiel): We first cut the row in diff lines and at the end of the row merge the overlapping line pieces
    // The vector is just keeping the min and max value together, x == minX, y == maxX.
    i_expect(a->width <= 480);
    i_expect(a->width == b->width);
    i_expect(a->height == b->height);
    
    u32 diffRowCount = 0;
    v2u diffRowLines[480]; // NOTE(michiel): Can't ever need 480 lines, that would mean every pixel needs a rect.
    
    u32 *aPixels = a->pixels;
    u32 *bPixels = b->pixels;
    
    u32 result = 0;
    
    for (u32 y = 0; y < a->height; ++y)
    {
        u32 *aRow = aPixels;
        u32 *bRow = bPixels;
        
        diffRowCount = 0;
        v2u *diffRange = 0;
        for (u32 x = 0; x < a->width; ++x)
        {
            if (*aRow++ != *bRow++)
            {
                if (!diffRange)
                {
                    i_expect(diffRowCount < array_count(diffRowLines));
                    diffRange = diffRowLines + diffRowCount++;
                    diffRange->x = x;
                }
                diffRange->y = x;
            }
            else
            {
                // NOTE(michiel): End of a diff range (if needed)
                diffRange = 0;
            }
        }
        
        aPixels += a->rowStride;
        bPixels += a->rowStride;
        
        // NOTE(michiel): Try to find a previous rectangle that touches a found line diff range.
        for (u32 lineIdx = 0; lineIdx < diffRowCount; ++lineIdx)
        {
            v2u line = diffRowLines[lineIdx];
            
            b32 mergedLine = false;
            for (u32 rectIdx = 0; rectIdx < result; ++rectIdx)
            {
                Rectangle2u *rect = rectangles + rectIdx;
                if ((rect->max.y + pixelBorder) >= y)
                {
                    if ((((line.x + pixelBorder) >= rect->min.x) && (line.x <= (rect->max.x + pixelBorder))) ||
                        (((line.y + pixelBorder) >= rect->min.x) && (line.y <= (rect->max.x + pixelBorder))) ||
                        ((line.x <= (rect->min.x + pixelBorder)) && ((line.y + pixelBorder) >= rect->max.x)))
                    {
                        rect->min.x = minimum(line.x, rect->min.x);
                        rect->max.x = maximum(line.y, rect->max.x);
                        rect->max.y = y;
                        mergedLine = true;
                        break;
                    }
                }
            }
            
            if (!mergedLine)
            {
                // TODO(michiel): Make it so that the result can be greater than maxRectCount??
                i_expect(result < maxRectCount);
                Rectangle2u *rect = rectangles + result++;
                rect->min.x = line.x;
                rect->max.x = line.y;
                rect->min.y = rect->max.y = y;
            }
        }
    }
    
    // NOTE(michiel): Merge rectangles
    for (s32 rectIdx = result - 1; rectIdx >= 0; --rectIdx)
    {
        Rectangle2u *rect = rectangles + rectIdx;
        for (u32 testIdx = 0; testIdx < rectIdx; ++testIdx)
        {
            Rectangle2u *testRect = rectangles + testIdx;
            // NOTE(michiel): The pixelBorder makes the rectangles merges more neighbouring pixels.
            if (!((rect->min.x > (testRect->max.x + pixelBorder)) ||
                  ((rect->max.x + pixelBorder) < testRect->min.x) ||
                  (rect->min.y > (testRect->max.y + pixelBorder)) ||
                  ((rect->max.y + pixelBorder) < testRect->min.y)))
            {
                testRect->min.x = minimum(rect->min.x, testRect->min.x);
                testRect->min.y = minimum(rect->min.y, testRect->min.y);
                testRect->max.x = maximum(rect->max.x, testRect->max.x);
                testRect->max.y = maximum(rect->max.y, testRect->max.y);
                *rect = rectangles[--result];
                break;
            }
        }
    }
    
    return result;
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(BasicState) <= state->memorySize);
    
    v2 size = V2((f32)image->width, (f32)image->height);
    
    BasicState *basics = (BasicState *)state->memory;
    if (!state->initialized)
    {
        // basics->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        basics->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        basics->a.width = 480;
        basics->a.height = 320;
        basics->a.rowStride = basics->a.width;
        basics->b.width = 480;
        basics->b.height = 320;
        basics->b.rowStride = basics->b.width;
        u32 arrayCount = 320 * 480;
        basics->a.pixels = allocate_array(u32, arrayCount);
        basics->b.pixels = allocate_array(u32, arrayCount);
        
        fill_rectangle(&basics->a, 0, 0, basics->a.width, basics->a.height, V4(0, 0, 0, 1));
        fill_rectangle(&basics->b, 0, 0, basics->a.width, basics->a.height, V4(0, 0, 0, 1));
        
        fill_circle(&basics->b, 20.0f, 20.0f, 15.0f, V4(1, 0, 0, 1));
        fill_circle(&basics->b, 40.0f, 30.0f, 20.0f, V4(1, 0, 0, 1));
        fill_circle(&basics->b, 220.0f, 20.0f, 15.0f, V4(1, 0, 0, 1));
        fill_circle(&basics->b, 20.0f, 220.0f, 15.0f, V4(1, 0, 0, 1));
        fill_circle(&basics->b, 220.0f, 220.0f, 15.0f, V4(1, 0, 0, 1));
        
        state->initialized = true;
    }
    
    u32 diffRowCount = 0;
    v2u diffRowLines[480]; // NOTE(michiel): Can't ever need 480 lines, that would mean every pixel needs a rect.
    
    u32 *aPixels = basics->a.pixels + basics->atY * basics->a.rowStride;
    u32 *bPixels = basics->b.pixels + basics->atY * basics->b.rowStride;
    
    //basics->rectangleCount = 0;
    v2u *diffAt = 0;
    for (u32 x = 0; x < basics->a.width; ++x)
    {
        if (*aPixels++ != *bPixels++)
        {
            if (!diffAt)
            {
                i_expect(diffRowCount < array_count(diffRowLines));
                diffAt = diffRowLines + diffRowCount++;
                diffAt->x = x;
            }
            diffAt->y = x + 1;
        }
        else
        {
            diffAt = 0;
        }
    }
    
    u32 pixelBorder = 3;
    for (u32 lineIdx = 0; lineIdx < diffRowCount; ++lineIdx)
    {
        v2u line = diffRowLines[lineIdx];
        
        b32 mergedLine = false;
        for (u32 rectIdx = 0; rectIdx < basics->rectangleCount; ++rectIdx)
        {
            Rectangle2u *rect = basics->rectangles + rectIdx;
            if ((rect->max.y + pixelBorder) > basics->atY)
            {
                if ((((line.x + pixelBorder) >= rect->min.x) && (line.x < (rect->max.x + pixelBorder))) ||
                    (((line.y + pixelBorder) >= rect->min.x) && (line.y < (rect->max.x + pixelBorder))) ||
                    ((line.x <= (rect->min.x + pixelBorder)) && ((line.y + pixelBorder) > rect->max.x)))
                {
                    rect->min.x = minimum(line.x, rect->min.x);
                    rect->max.x = maximum(line.y, rect->max.x);
                    rect->max.y = basics->atY + 1;
                    mergedLine = true;
                    break;
                }
            }
        }
        
        if (!mergedLine)
        {
            i_expect(basics->rectangleCount < array_count(basics->rectangles));
            Rectangle2u *rect = basics->rectangles + basics->rectangleCount++;
            rect->min.x = line.x;
            rect->max.x = line.y;
            rect->min.y = rect->max.y = basics->atY;
            rect->max.y += 1;
        }
    }
    
    // NOTE(michiel): Merge rectangles
    for (s32 rectIdx = basics->rectangleCount - 1; rectIdx >= 0; --rectIdx)
    {
        Rectangle2u *rect = basics->rectangles + rectIdx;
        for (u32 testIdx = 0; testIdx < rectIdx; ++testIdx)
        {
            Rectangle2u *testRect = basics->rectangles + testIdx;
            // NOTE(michiel): The pixelBorder makes the rectangles merges more neighbouring pixels.
            if (in_rectangle(*testRect, rect->min) ||
                in_rectangle(*testRect, rect->max))
            {
                *testRect = rect_union(*testRect, *rect);
                *rect = basics->rectangles[--basics->rectangleCount];
                break;
            }
        }
    }
    
    clear(&basics->a);
    for (u32 rectIdx = 0; rectIdx < basics->rectangleCount; ++rectIdx)
    {
        Rectangle2u rect = basics->rectangles[rectIdx];
        //rect.max += V2U(1, 1);
        outline_rectangle(&basics->a, rect, 0xFFFFFFFF);
    }
    for (u32 lineIdx = 0; lineIdx < diffRowCount; ++lineIdx)
    {
        v2u *line = diffRowLines + lineIdx;
        draw_line(&basics->a, (s32)line->x, (s32)basics->atY, (s32)line->y, (s32)basics->atY, V4(1, 1, 0, 1));
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    draw_image(image, 0, 0, &basics->b);
    draw_image(image, 0, 0, &basics->a);
    
    fill_rectangle(&basics->a, 0, 0, basics->a.width, basics->a.height, V4(0, 0, 0, 1));
    
    basics->prevMouseDown = mouse.mouseDowns;
    basics->seconds += dt;
    ++basics->ticks;
    if (basics->seconds > 1.0f)
    {
        basics->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", basics->ticks,
                1000.0f / (f32)basics->ticks);
        basics->ticks = 0;
    }
    
    f32 maxTick = 0.25f;
    basics->tickAt += dt;
    if (basics->tickAt > maxTick)
    {
        basics->tickAt -= maxTick;
        ++basics->atY;
        if (basics->atY >= basics->a.height)
        {
            basics->atY = 0;
            basics->rectangleCount = 0;
        }
    }
}

