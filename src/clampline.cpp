#include "../libberdip/platform.h"

enum RectPositionFlag
{
    Outside_Inside = 0x00,
    Outside_Left   = 0x01,
    Outside_Right  = 0x02,
    Outside_Bottom = 0x04,
    Outside_Top    = 0x08,
};

internal u32
calculate_point_outside_rect(v2 point, v2 rectMin, v2 rectMax)
{
    u32 result = Outside_Inside;

    if (point.x < rectMin.x) { result |= Outside_Left; }
    if (point.y < rectMin.y) { result |= Outside_Bottom; }
    if (point.x > rectMax.x) { result |= Outside_Right; }
    if (point.y > rectMax.y) { result |= Outside_Top; }

    return result;
}

internal v2
calculate_intersection(v2 start, v2 end, v2 rectMin, v2 rectMax, u32 flags)
{
    v2 result = {};

    v2 delta = end - start;

    f32 slopeX = delta.y / delta.x;
    f32 slopeY = delta.x / delta.y;

    if (flags & Outside_Top)
    {
        result.x = start.x + slopeY * (rectMax.y - start.y);
        result.y = rectMax.y;
    }
    else if (flags & Outside_Bottom)
    {
        result.x = start.x + slopeY * (rectMin.y - start.y);
        result.y = rectMin.y;
    }
    else if (flags & Outside_Right)
    {
        result.x = rectMax.x;
        result.y = start.y + slopeX * (rectMax.x - start.x);
    }
    else if (flags & Outside_Left)
    {
        result.x = rectMin.x;
        result.y = start.y + slopeX * (rectMin.x - start.x);
    }

    return result;
}

internal v4
clamp_line(v2 start, v2 end, v2 rectSize)
{
    v2 rectMin = V2(0, 0);
    v2 rectMax = rectSize - V2(1, 1);

    u32 startFlag = calculate_point_outside_rect(start, rectMin, rectMax);
    u32 endFlag = calculate_point_outside_rect(end, rectMin, rectMax);

    b32 found = false;

    while (1)
    {
        if ((startFlag | endFlag) == Outside_Inside)
        {
            // NOTE(michiel): Point is inside
            found = true;
            break;
        }

        if ((startFlag & endFlag) != 0)
        {
            // NOTE(michiel): Totally outside
            break;
        }

        if (startFlag != Outside_Inside)
        {
            v2 clipped = calculate_intersection(start, end, rectMin, rectMax, startFlag);
            start = clipped;
            startFlag = calculate_point_outside_rect(start, rectMin, rectMax);
        }
        else
        {
            v2 clipped = calculate_intersection(start, end, rectMin, rectMax, endFlag);
            end = clipped;
            endFlag = calculate_point_outside_rect(end, rectMin, rectMax);
        }
    }

    v4 result = {};
    if (found)
    {
        result.xy = start;
        result.zw = end;
    }
    return result;
}

#if 0
internal v4
clamp_line(v2 start, v2 end, v2 rectSize)
{
    v4 result = {};

    b32 startXless = start.x < 0.0f;
    b32 startYless = start.y < 0.0f;
    b32 startXgreater = start.x >= rectSize.x;
    b32 startYgreater = start.y >= rectSize.y;

    b32 endXless = end.x < 0.0f;
    b32 endYless = end.y < 0.0f;
    b32 endXgreater = end.x >= rectSize.x;
    b32 endYgreater = end.y >= rectSize.y;

    if ((startXless && endXless) ||
        (startXgreater && endXgreater) ||
        (startYless && endYless) ||
        (startYgreater && endYgreater))
    {
        // NOTE(michiel): Whole line is outside of the rectangle
    }
    else
    {
        v2 start0 = start;
        v2 end0 = end;

        if (startXless && startYless)
        {

        }
    }

    if (((start.x < 0.0f) && (end.x < 0.0f)) ||
        ((start.x >= rectSize.x) && (end.x >= rectSize.x)) ||
        ((start.y < 0.0f) && (end.y < 0.0f)) ||
        ((start.y >= rectSize.y) && (end.y >= rectSize.y)))
    {
        return {};
    }

    while ((result.x < 0.0f) || (result.y < 0.0f) ||
           (result.z < 0.0f) || (result.w < 0.0f) ||
           (result.x >= rectSize.x) ||
           (result.y >= rectSize.y) ||
           (result.z >= rectSize.x) ||
           (result.w >= rectSize.y))
    {
        // NOTE(michiel): Clamp line
        f32 slope = (start.y - end.y) / (start.x - end.x);
        f32 oneOverSlope = (start.x - end.x) / (start.y - end.y);

        if (result.x < 0.0f)
        {
            result.y -= slope * start.x;
            result.x = 0.0f;
            start = result.xy;
            fprintf(stdout, "Clipped X<0: %f,%f\n", result.x, result.y);
        }
        if (result.y < 0.0f)
        {
            result.x -= oneOverSlope * start.y;
            result.y = 0.0f;
            start = result.xy;
            fprintf(stdout, "Clipped Y<0: %f,%f\n", result.x, result.y);
        }
        if (result.z < 0.0f)
        {
            result.z = 0.0f;
            result.w += slope * (result.z - end.x);
            end = result.zw;
            fprintf(stdout, "Clipped Z<0: %f,%f\n", result.z, result.w);
        }
        if (result.w < 0.0f)
        {
            result.w = 0.0f;
            result.z += oneOverSlope * (result.w - end.y);
            end = result.zw;
            fprintf(stdout, "Clipped W<0: %f,%f\n", result.z, result.w);
        }

        if (result.x >= rectSize.x)
        {
            result.x = rectSize.x - 1.0f;
            result.y += slope * (result.x - start.x);
            start = result.xy;
            fprintf(stdout, "Clipped X>N: %f,%f\n", result.x, result.y);
        }
        if (result.y >= rectSize.y)
        {
            result.y = rectSize.y - 1.0f;
            result.x += oneOverSlope * (result.y - start.y);
            start = result.xy;
            fprintf(stdout, "Clipped Y>N: %f,%f\n", result.x, result.y);
        }
        if (result.z >= rectSize.x)
        {
            result.z = rectSize.x - 1.0f;
            result.w += slope * (result.z - end.x);
            end = result.zw;
            fprintf(stdout, "Clipped Z>N: %f,%f\n", result.z, result.w);
        }
        if (result.w >= rectSize.y)
        {
            result.w = rectSize.y - 1.0f;
            result.z += oneOverSlope * (result.w - end.y);
            end = result.zw;
            fprintf(stdout, "Clipped W>N: %f,%f\n", result.z, result.w);
        }
    }

    return result;
}
#endif

int main(int argc, char **argv)
{
    v2 startLine;
    v2 endLine;
    v2 rectSize;
    v4 clamped;

    startLine = V2(-10, -20);
    endLine = V2(20, 20);
    rectSize = V2(40, 40);
    clamped = clamp_line(startLine, endLine, rectSize);
    fprintf(stdout, "Rect size: %f x %f\n", rectSize.x, rectSize.y);
    fprintf(stdout, "Clamped (%f,%f) -> (%f,%f) to (%f,%f) -> (%f,%f)\n",
            startLine.x, startLine.y, endLine.x, endLine.y,
            clamped.x, clamped.y, clamped.z, clamped.w);

    startLine = V2(20, 20);
    endLine = V2(-20, -10);
    rectSize = V2(40, 40);
    clamped = clamp_line(startLine, endLine, rectSize);
    fprintf(stdout, "Rect size: %f x %f\n", rectSize.x, rectSize.y);
    fprintf(stdout, "Clamped (%f,%f) -> (%f,%f) to (%f,%f) -> (%f,%f)\n",
            startLine.x, startLine.y, endLine.x, endLine.y,
            clamped.x, clamped.y, clamped.z, clamped.w);

    startLine = V2(20, 20);
    endLine = V2(50, 10);
    rectSize = V2(40, 40);
    clamped = clamp_line(startLine, endLine, rectSize);
    fprintf(stdout, "Rect size: %f x %f\n", rectSize.x, rectSize.y);
    fprintf(stdout, "Clamped (%f,%f) -> (%f,%f) to (%f,%f) -> (%f,%f)\n",
            startLine.x, startLine.y, endLine.x, endLine.y,
            clamped.x, clamped.y, clamped.z, clamped.w);

    startLine = V2(20, 20);
    endLine = V2(10, 50);
    rectSize = V2(40, 40);
    clamped = clamp_line(startLine, endLine, rectSize);
    fprintf(stdout, "Rect size: %f x %f\n", rectSize.x, rectSize.y);
    fprintf(stdout, "Clamped (%f,%f) -> (%f,%f) to (%f,%f) -> (%f,%f)\n",
            startLine.x, startLine.y, endLine.x, endLine.y,
            clamped.x, clamped.y, clamped.z, clamped.w);

    startLine = V2(20, 20);
    endLine = V2(50, 50);
    rectSize = V2(40, 40);
    clamped = clamp_line(startLine, endLine, rectSize);
    fprintf(stdout, "Rect size: %f x %f\n", rectSize.x, rectSize.y);
    fprintf(stdout, "Clamped (%f,%f) -> (%f,%f) to (%f,%f) -> (%f,%f)\n",
            startLine.x, startLine.y, endLine.x, endLine.y,
            clamped.x, clamped.y, clamped.z, clamped.w);

    startLine = V2(-20, -20);
    endLine = V2(50, 50);
    rectSize = V2(40, 40);
    clamped = clamp_line(startLine, endLine, rectSize);
    fprintf(stdout, "Rect size: %f x %f\n", rectSize.x, rectSize.y);
    fprintf(stdout, "Clamped (%f,%f) -> (%f,%f) to (%f,%f) -> (%f,%f)\n",
            startLine.x, startLine.y, endLine.x, endLine.y,
            clamped.x, clamped.y, clamped.z, clamped.w);

    startLine = V2(-10, -20);
    endLine = V2(20, 20);
    rectSize = V2(40, 40);
    clamped = clamp_line(startLine, endLine, rectSize);
    fprintf(stdout, "Rect size: %f x %f\n", rectSize.x, rectSize.y);
    fprintf(stdout, "Clamped (%f,%f) -> (%f,%f) to (%f,%f) -> (%f,%f)\n",
            startLine.x, startLine.y, endLine.x, endLine.y,
            clamped.x, clamped.y, clamped.z, clamped.w);

    startLine = V2(-20, -10);
    endLine = V2(20, 20);
    rectSize = V2(40, 40);
    clamped = clamp_line(startLine, endLine, rectSize);
    fprintf(stdout, "Rect size: %f x %f\n", rectSize.x, rectSize.y);
    fprintf(stdout, "Clamped (%f,%f) -> (%f,%f) to (%f,%f) -> (%f,%f)\n",
            startLine.x, startLine.y, endLine.x, endLine.y,
            clamped.x, clamped.y, clamped.z, clamped.w);

    startLine = V2(20, 20);
    endLine = V2(-20, -10);
    rectSize = V2(40, 40);
    clamped = clamp_line(startLine, endLine, rectSize);
    fprintf(stdout, "Rect size: %f x %f\n", rectSize.x, rectSize.y);
    fprintf(stdout, "Clamped (%f,%f) -> (%f,%f) to (%f,%f) -> (%f,%f)\n",
            startLine.x, startLine.y, endLine.x, endLine.y,
            clamped.x, clamped.y, clamped.z, clamped.w);

    startLine = V2(20, 20);
    endLine = V2(-10, -20);
    rectSize = V2(40, 40);
    clamped = clamp_line(startLine, endLine, rectSize);
    fprintf(stdout, "Rect size: %f x %f\n", rectSize.x, rectSize.y);
    fprintf(stdout, "Clamped (%f,%f) -> (%f,%f) to (%f,%f) -> (%f,%f)\n",
            startLine.x, startLine.y, endLine.x, endLine.y,
            clamped.x, clamped.y, clamped.z, clamped.w);

    startLine = V2(20, -10);
    endLine = V2(20, 20);
    rectSize = V2(40, 40);
    clamped = clamp_line(startLine, endLine, rectSize);
    fprintf(stdout, "Rect size: %f x %f\n", rectSize.x, rectSize.y);
    fprintf(stdout, "Clamped (%f,%f) -> (%f,%f) to (%f,%f) -> (%f,%f)\n",
            startLine.x, startLine.y, endLine.x, endLine.y,
            clamped.x, clamped.y, clamped.z, clamped.w);

    startLine = V2(-10, 20);
    endLine = V2(20, 20);
    rectSize = V2(40, 40);
    clamped = clamp_line(startLine, endLine, rectSize);
    fprintf(stdout, "Rect size: %f x %f\n", rectSize.x, rectSize.y);
    fprintf(stdout, "Clamped (%f,%f) -> (%f,%f) to (%f,%f) -> (%f,%f)\n",
            startLine.x, startLine.y, endLine.x, endLine.y,
            clamped.x, clamped.y, clamped.z, clamped.w);

    startLine = V2(20, 20);
    endLine = V2(-10, 20);
    rectSize = V2(40, 40);
    clamped = clamp_line(startLine, endLine, rectSize);
    fprintf(stdout, "Rect size: %f x %f\n", rectSize.x, rectSize.y);
    fprintf(stdout, "Clamped (%f,%f) -> (%f,%f) to (%f,%f) -> (%f,%f)\n",
            startLine.x, startLine.y, endLine.x, endLine.y,
            clamped.x, clamped.y, clamped.z, clamped.w);

    startLine = V2(20, 20);
    endLine = V2(20, -10);
    rectSize = V2(40, 40);
    clamped = clamp_line(startLine, endLine, rectSize);
    fprintf(stdout, "Rect size: %f x %f\n", rectSize.x, rectSize.y);
    fprintf(stdout, "Clamped (%f,%f) -> (%f,%f) to (%f,%f) -> (%f,%f)\n",
            startLine.x, startLine.y, endLine.x, endLine.y,
            clamped.x, clamped.y, clamped.z, clamped.w);

    startLine = V2(745.726501, -110.792328);
    endLine = V2(745.897461, -110.809471);
    rectSize = V2(1440, 960);
    clamped = clamp_line(startLine, endLine, rectSize);
    fprintf(stdout, "Rect size: %f x %f\n", rectSize.x, rectSize.y);
    fprintf(stdout, "Clamped (%f,%f) -> (%f,%f) to (%f,%f) -> (%f,%f)\n",
            startLine.x, startLine.y, endLine.x, endLine.y,
            clamped.x, clamped.y, clamped.z, clamped.w);

#if 0
    rectSize = V2(40, 40);
    startLine = V2(20, 20);
    for (u32 y = 0; y < 400; ++y)
    {
        endLine.y = (f32)y / 4.0f - 70.0f;
        for (u32 x = 0; x < 400; ++x)
        {
            endLine.x = (f32)x / 4.0f - 70.0f;
            clamped = clamp_line(startLine, endLine, rectSize);
            if (clamped.x < 0.0f) {
                fprintf(stderr, "ERROR: (%f,%f)x(%f,%f) -> (%f,%f)x(%f,%f)\n",
                        startLine.x, startLine.y, endLine.x, endLine.y,
                        clamped.x, clamped.y, clamped.z, clamped.w);
            }
            if (clamped.y < 0.0f) {
                fprintf(stderr, "ERROR: (%f,%f)x(%f,%f) -> (%f,%f)x(%f,%f)\n",
                        startLine.x, startLine.y, endLine.x, endLine.y,
                        clamped.x, clamped.y, clamped.z, clamped.w);
            }
            if (clamped.z < 0.0f) {
                fprintf(stderr, "ERROR: (%f,%f)x(%f,%f) -> (%f,%f)x(%f,%f)\n",
                        startLine.x, startLine.y, endLine.x, endLine.y,
                        clamped.x, clamped.y, clamped.z, clamped.w);
            }
            if (clamped.w < 0.0f) {
                fprintf(stderr, "ERROR: (%f,%f)x(%f,%f) -> (%f,%f)x(%f,%f)\n",
                        startLine.x, startLine.y, endLine.x, endLine.y,
                        clamped.x, clamped.y, clamped.z, clamped.w);
            }
            if (clamped.x >= rectSize.x) {
                fprintf(stderr, "ERROR: (%f,%f)x(%f,%f) -> (%f,%f)x(%f,%f)\n",
                        startLine.x, startLine.y, endLine.x, endLine.y,
                        clamped.x, clamped.y, clamped.z, clamped.w);
            }
            if (clamped.y >= rectSize.y) {
                fprintf(stderr, "ERROR: (%f,%f)x(%f,%f) -> (%f,%f)x(%f,%f)\n",
                        startLine.x, startLine.y, endLine.x, endLine.y,
                        clamped.x, clamped.y, clamped.z, clamped.w);
            }
            if (clamped.z >= rectSize.x) {
                fprintf(stderr, "ERROR: (%f,%f)x(%f,%f) -> (%f,%f)x(%f,%f)\n",
                        startLine.x, startLine.y, endLine.x, endLine.y,
                        clamped.x, clamped.y, clamped.z, clamped.w);
            }
            if (clamped.w >= rectSize.y) {
                fprintf(stderr, "ERROR: (%f,%f)x(%f,%f) -> (%f,%f)x(%f,%f)\n",
                        startLine.x, startLine.y, endLine.x, endLine.y,
                        clamped.x, clamped.y, clamped.z, clamped.w);
            }
        }
    }
#endif

    return 0;
}
