#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct Acoustic2
{
    RandomSeriesPCG randomizer;
    f32 second;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    u32 ropePointCount;
    v2 *ropePoints;
    v2 *prevRopePoints;
    
    f32 tension;
    f32 linearDensity;
    f32 propagationSpeed;
    f32 CnoDt;
    
    b32 boundLeft;
    b32 boundRight;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(Acoustic2) <= state->memorySize);
    
    v2 size = V2((f32)image->width, (f32)image->height);
    
    Acoustic2 *acoust = (Acoustic2 *)state->memory;
    if (!state->initialized)
    {
        // acoust->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        acoust->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        acoust->boundLeft = true;
        acoust->boundRight = false;
        
        acoust->ropePointCount = 301;
        acoust->ropePoints = allocate_array(v2, acoust->ropePointCount);
        acoust->prevRopePoints = allocate_array(v2, acoust->ropePointCount);
        
        f32 xStep = (size.x - 10.0f) / (f32)acoust->ropePointCount;
        for (u32 rp = 0; rp < acoust->ropePointCount; ++rp)
        {
            acoust->ropePoints[rp].x = 10.0f + xStep * rp;
            acoust->ropePoints[rp].y = 0.0f;
            acoust->prevRopePoints[rp].x = 10.0f + xStep * rp;
            acoust->prevRopePoints[rp].y = 0.0f;
        }
        
        acoust->tension = 2000.0f; // NOTE(michiel): in N
        acoust->linearDensity = 1.0f; // NOTE(michiel): in kg/m
        acoust->propagationSpeed = square_root(acoust->tension / acoust->linearDensity); // NOTE(michiel): Speed in m/s
        // NOTE(michiel): sqrt((kg*m/s^2) / (kg/m)) => sqrt(m^2/s^2) => m / s
        acoust->CnoDt = acoust->propagationSpeed / (acoust->ropePoints[1].x - acoust->ropePoints[0].x);
        
        // NOTE(michiel): Initial condition
        u32 halfPoint = acoust->ropePointCount / 2;
#if 0        
        acoust->prevRopePoints[halfPoint + 2].y -= 5.0f;
        acoust->prevRopePoints[halfPoint + 1].y -= 10.0f;
        acoust->prevRopePoints[halfPoint + 0].y -= 12.0f;
        acoust->prevRopePoints[halfPoint - 1].y -= 10.0f;
        acoust->prevRopePoints[halfPoint - 2].y -= 5.0f;
#else
        u32 halfDist = 10;
        
        for (u32 n = 0; n < 2 * halfDist + 1; ++n)
        {
            acoust->prevRopePoints[halfPoint - halfDist + n].y = -0.5f + 0.5f * cos(F32_TAU * (f32)n / (2.0f * halfDist + 1.0f));
        }
#endif
        
        // NOTE(michiel): Initial propagation
        f32 Csqr = square(acoust->CnoDt * dt);
        for (u32 rp = 1; rp < acoust->ropePointCount - 1; ++rp)
        {
            f32 yi = acoust->prevRopePoints[rp].y;
            f32 yip1 = acoust->prevRopePoints[rp + 1].y;
            f32 yim1 = acoust->prevRopePoints[rp - 1].y;
            
            acoust->ropePoints[rp].y = yi - 0.5f * Csqr * (yip1 - 2.0f * yi + yim1);
        }
        
        state->initialized = true;
    }
    
    if ((mouse.mouseDowns & Mouse_Left) &&
        !(acoust->prevMouseDown & Mouse_Left))
    {
        // NOTE(michiel): Initial condition
        u32 halfPoint = acoust->ropePointCount / 2;
        u32 halfDist = 10;
        
        for (u32 n = 0; n < 2 * halfDist + 1; ++n)
        {
            acoust->prevRopePoints[halfPoint - halfDist + n].y = -0.5f + 0.5f * cos(F32_TAU * (f32)n / (2.0f * halfDist + 1.0f));
        }
        
        // NOTE(michiel): Initial propagation
        f32 Csqr = square(acoust->CnoDt * dt);
        for (u32 rp = 1; rp < acoust->ropePointCount - 1; ++rp)
        {
            f32 yi = acoust->prevRopePoints[rp].y;
            f32 yip1 = acoust->prevRopePoints[rp + 1].y;
            f32 yim1 = acoust->prevRopePoints[rp - 1].y;
            
            acoust->ropePoints[rp].y = yi - 0.5f * Csqr * (yip1 - 2.0f * yi + yim1);
        }
    }
    
#if 1
    f32 Csqr = 1.0f; // square(acoust->CnoDt * dt);
    for (u32 rp = 0; rp < acoust->ropePointCount; ++rp)
    {
        if ((rp == 0 && acoust->boundLeft) ||
            ((rp == acoust->ropePointCount - 1) &&
             acoust->boundRight))
        {
            continue;
        }
        
        f32 prevT = acoust->prevRopePoints[rp].y;
        
        f32 prevX = 0.0f;
        if (rp > 0)
        {
            prevX = acoust->ropePoints[rp - 1].y;
        }
        f32 curXT = acoust->ropePoints[rp].y;
        f32 nextX = prevX;
        if (rp < acoust->ropePointCount - 1)
        {
            nextX = acoust->ropePoints[rp + 1].y;
        }
        if (rp == 0)
        {
            prevX = nextX;
        }
        
        f32 nextXT = 2.0f * curXT - prevT + Csqr * (nextX - 2.0f * curXT + prevX);
        
        acoust->prevRopePoints[rp].y = nextXT;
    }
    
    v2 *temp = acoust->prevRopePoints;
    acoust->prevRopePoints = acoust->ropePoints;
    acoust->ropePoints = temp;
#endif
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    draw_lines(image, acoust->ropePointCount, acoust->ropePoints, V2(0, 0.5f * size.y), V2(1, 30), V4(1, 1, 1, 1));
    
    acoust->prevMouseDown = mouse.mouseDowns;
    acoust->second += dt;
    acoust->seconds += dt;
    ++acoust->ticks;
    if (acoust->second > 1.0f)
    {
        acoust->second -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", acoust->ticks,
                1000.0f / (f32)acoust->ticks);
        acoust->ticks = 0;
    }
}
