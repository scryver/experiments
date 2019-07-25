#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "../libberdip/perlin.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

enum RayType
{
    Ray_None,
    Ray_Cast,
    Ray_Hit,
    Ray_Deflection,
    Ray_Reflection,
    Ray_Miss,
    
    RayTypeCount,
};

struct Ray
{
    RayType type;
    f32 tCast;
    v2u start;
    v2u end;
};

struct PlayingGrid
{
    v2u size;
    
    u32 atomCount;
    v2u atomLocations[16];
    
    u32 maxRayCount;
    u32 rayCount;
    Ray *rays;
};

struct BlackBoxState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    u32 tileWidth;
    
    PlayingGrid grid;
};

internal void
init_playing_grid(PlayingGrid *grid, v2u size)
{
    grid->size = size;
    
    grid->atomCount = 4;
    grid->atomLocations[0] = V2U(1, 1);
    grid->atomLocations[1] = V2U(1, 8);
    grid->atomLocations[2] = V2U(6, 4);
    grid->atomLocations[3] = V2U(2, 5);
    
    grid->maxRayCount = (grid->size.x + grid->size.y) * 2;
    grid->rays = allocate_array(Ray, grid->maxRayCount);
    
    v2u pos = V2U(1, 0);
    for (u32 rayIdx = 0; rayIdx < grid->maxRayCount; ++rayIdx)
    {
        Ray *ray = grid->rays + rayIdx;
        ray->start = ray->end = pos;
        
        // NOTE(michiel): Crazy increment to go around the grid in a single loop
        if (pos.x && (pos.x != (grid->size.x + 1))) {
            // NOTE(michiel): For the top and bottom row we want to increment x
            pos.x += 1;
            if (pos.x == (grid->size.x + 1)) {
                if (pos.y == 0) {
                    // NOTE(michiel): After the top row go to the right column
                    pos.x = grid->size.x + 1;
                } else {
                    // NOTE(michiel): After the bottom row go to the left
                    pos.x = 0;
                }
                pos.y = 1;
            }
        } else {
            // NOTE(michiel): For the left and right row we want to increment y
            pos.y += 1;
            if (pos.y == (grid->size.y + 1)) {
                // NOTE(michiel): After the right one we go to the bottom row
                // the other option shouldn't occur due to our maxRayCount.
                pos.x = 1;
                pos.y = 9;
            }
        }
    }
}

internal Ray *
fire_ray(PlayingGrid *grid, v2u start, v2u expectedEnd)
{
    Ray *result = 0;
    for (u32 rayIdx = 0; rayIdx < grid->rayCount; ++rayIdx)
    {
        Ray *testRay = grid->rays + rayIdx;
        if ((testRay->start == start) &&
            (testRay->type != Ray_None))
        {
            result = testRay;
            break;
        }
    }
    
    if (!result)
    {
        i_expect(grid->rayCount < grid->maxRayCount);
        result = grid->rays + grid->rayCount++;
    }
    
    result->type = Ray_Cast;
    result->start = start;
    result->end = expectedEnd;
    result->tCast = 0.0f;
    
    return result;
}

internal void
draw_playing_grid(Image *image, PlayingGrid *grid, u32 tileWidth, v2u mouseP)
{
    u32 halfTileWidth = tileWidth / 2;
    v2u startP = (V2U(image->width, image->height) - (grid->size + V2U(2, 2)) * tileWidth) / 2;
    
    // NOTE(michiel): Draw Grid
    for (u32 y = 0; y < (grid->size.y + 2); ++y)
    {
        for (u32 x = 0; x < (grid->size.x + 2); ++x)
        {
            v2u at = V2U(x, y);
            
            v4 colour = V4(0.8f, 0.8f, 0.8f, 1);
            if (((x == 0) && (y == 0)) ||
                ((x == 0) && (y == (grid->size.y + 1))) ||
                ((x == (grid->size.x + 1)) && (y == 0)) ||
                ((x == (grid->size.x + 1)) && (y == (grid->size.y + 1))))
            {
                colour = V4(0, 0, 0, 1);
            }
            else if ((x == 0) || (y == 0) || (x == (grid->size.x + 1)) || (y == (grid->size.y + 1)))
            {
                colour = V4(0.5f, 0.5f, 0.5f, 1);
            }
            
            if (mouseP == at)
            {
                colour *= 1.2f;
                colour.a = clamp01(colour.a);
            }
            
            v2u pos = startP + at * tileWidth;
            fill_rectangle(image, pos.x + 1, pos.y + 1, tileWidth - 2, tileWidth - 2, colour);
        }
    }
    
    // NOTE(michiel): Draw Atoms
    for (u32 atomIdx = 0; atomIdx < grid->atomCount; ++atomIdx)
    {
        v2u atomLoc = grid->atomLocations[atomIdx];
        v2u pos = startP + atomLoc * tileWidth;
        fill_circle(image, pos.x + halfTileWidth, pos.y + halfTileWidth,
                    tileWidth / 3, V4(0.8f, 0.2f, 0.0f, 1));
    }
    
    // NOTE(michiel): Draw rays
    for (u32 rayIdx = 0; rayIdx < grid->rayCount; ++rayIdx)
    {
        Ray *ray = grid->rays + rayIdx;
        if (ray->type != Ray_None)
        {
            v2u start = startP + ray->start * tileWidth;
            v2u end = startP + ray->end * tileWidth;
            v2u at = lerp(start, ray->tCast, end);
            draw_line(image, start.x + halfTileWidth, start.y + halfTileWidth,
                      at.x + halfTileWidth, at.y + halfTileWidth, V4(1, 0, 0, 1));
            
            fill_triangle(image, start + V2U(5, tileWidth - 5), start + V2U(halfTileWidth, 5),
                          start + V2U(tileWidth - 5, tileWidth - 5), V4(0.7f, 0.7f, 1.0f, 1));
        }
    }
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(BlackBoxState) <= state->memorySize);
    
    //v2 size = V2((f32)image->width, (f32)image->height);
    
    BlackBoxState *blackBox = (BlackBoxState *)state->memory;
    if (!state->initialized)
    {
        blackBox->randomizer = random_seed_pcg(624321596472ULL, 358549128268632912ULL);
        //blackBox->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        init_playing_grid(&blackBox->grid, V2U(8, 8));
        
        blackBox->tileWidth = 50;
        state->initialized = true;
    }
    
    PlayingGrid *grid = &blackBox->grid;
    
    // NOTE(michiel): Update rays
    for (u32 rayIdx = 0; rayIdx < grid->rayCount; ++rayIdx)
    {
        Ray *ray = grid->rays + rayIdx;
        if (ray->type == Ray_Cast)
        {
            ray->tCast += 0.01f;
            if (ray->tCast >= 1.0f)
            {
                ray->tCast = 1.0f;
                ray->type = Ray_Miss;
            }
        }
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    v2u mouseP = V2U(mouse.pixelPosition);
    v2u startP = (V2U(image->width, image->height) - (grid->size + V2U(2, 2)) * blackBox->tileWidth) / 2;
    mouseP -= startP;
    mouseP /= blackBox->tileWidth;
    
    draw_playing_grid(image, grid, blackBox->tileWidth, mouseP);
    
    {
        Rectangle2u topRow = rect_min_dim(V2U(1, 0), V2U(grid->size.x, 1));
        Rectangle2u botRow = rect_min_dim(V2U(1, grid->size.y + 1), V2U(grid->size.x, 1));
        Rectangle2u lefRow = rect_min_dim(V2U(0, 1), V2U(1, grid->size.y));
        Rectangle2u rigRow = rect_min_dim(V2U(grid->size.x + 1, 1), V2U(1, grid->size.y));
        
        b32 mouseClick = (mouse.mouseDowns & Mouse_Left) && !(blackBox->prevMouseDown & Mouse_Left);
        
        v4 lineColour = V4(0, 0.7f, 0.2f, 1);
        if (in_rectangle(topRow, mouseP))
        {
            if (mouseClick)
            {
                fire_ray(grid, mouseP, mouseP + V2U(0, grid->size.y + 1));
            }
            draw_line(image, mouse.pixelPosition.x, mouse.pixelPosition.y, 
                      mouse.pixelPosition.x, startP.y + botRow.min.y * blackBox->tileWidth, lineColour);
        }
        else if (in_rectangle(botRow, mouseP))
        {
            if (mouseClick)
            {
                fire_ray(grid, mouseP, mouseP - V2U(0, grid->size.y + 1));
            }
            draw_line(image, mouse.pixelPosition.x, mouse.pixelPosition.y,
                      mouse.pixelPosition.x, startP.y + topRow.max.y * blackBox->tileWidth, lineColour);
        }
        else if (in_rectangle(lefRow, mouseP))
        {
            if (mouseClick)
            {
                fire_ray(grid, mouseP, mouseP + V2U(grid->size.x + 1, 0));
            }
            draw_line(image, mouse.pixelPosition.x, mouse.pixelPosition.y, 
                      startP.x + rigRow.min.x * blackBox->tileWidth, mouse.pixelPosition.y, lineColour);
        }
        else if (in_rectangle(rigRow, mouseP))
        {
            if (mouseClick)
            {
                fire_ray(grid, mouseP, mouseP - V2U(grid->size.x + 1, 0));
            }
            draw_line(image, mouse.pixelPosition.x, mouse.pixelPosition.y,
                      startP.x + lefRow.max.x * blackBox->tileWidth, mouse.pixelPosition.y, lineColour);
        }
    }
    
    blackBox->prevMouseDown = mouse.mouseDowns;
    blackBox->seconds += dt;
    ++blackBox->ticks;
    if (blackBox->seconds > 1.0f)
    {
        blackBox->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", blackBox->ticks,
                1000.0f / (f32)blackBox->ticks);
        blackBox->ticks = 0;
    }
}

