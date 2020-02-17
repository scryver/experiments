#include "../libberdip/platform.h"

internal v4
clamp_line(v2 start, v2 end, v2 rectSize)
{
    v4 result = {};
    result.xy = start;
    result.zw = end;
    
    if ((result.x < 0.0f) || (result.y < 0.0f) ||
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
    
    return 0;
}
