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
    Ray_Hit,
    Ray_Deflection,
    Ray_Reflection,
    Ray_Miss,
    
    RayTypeCount,
};

struct Ray
{
    RayType type;
    v2u start;
    v2u end;
};

struct PlayingGrid
{
    v2u size;
    
    u32 atomCount;
    v2u atomLocations[16];
    
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
    
    grid->rayCount = (grid->size.x + grid->size.y) * 2;
    grid->rays = allocate_array(Ray, grid->rayCount);
    
    v2u pos = V2U(1, 0);
    for (u32 rayIdx = 0; rayIdx < grid->rayCount; ++rayIdx)
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
                // the other option shouldn't occur due to our rayCount.
                pos.x = 1;
                pos.y = 9;
            }
        }
    }
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
            v2u pos = startP + V2U(x, y) * tileWidth;
            
            v4 colour = V4(0.8f, 0.8f, 0.8f, 1);
            if (((x == 0) && (y == 0)) ||
                ((x == 0) && (y == 9)) ||
                ((x == 9) && (y == 0)) ||
                ((x == 9) && (y == 9)))
            {
                colour = V4(0, 0, 0, 1);
            }
            else if ((x == 0) || (y == 0) || (x == 9) || (y == 9))
            {
                colour = V4(0.5f, 0.5f, 0.5f, 1);
            }
            
            if ((mouseP.x >= pos.x) &&
                (mouseP.y >= pos.y) &&
                (mouseP.x < (pos.x + tileWidth)) &&
                (mouseP.y < (pos.y + tileWidth)))
            {
                colour *= 1.2f;
                colour.a = clamp01(colour.a);
            }
            
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
        v2u pos = startP + (ray->start * tileWidth);
        fill_triangle(image, pos + V2U(5, tileWidth - 5), pos + V2U(halfTileWidth, 5),
                      pos + V2U(tileWidth - 5, tileWidth - 5), V4(0.7f, 0.7f, 1.0f, 1));
        if (ray->type != Ray_None)
        {
            
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
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    v2u mouseP = V2U(mouse.pixelPosition);
    draw_playing_grid(image, grid, blackBox->tileWidth, mouseP);
    
    {
        u32 tileWidth = blackBox->tileWidth;
        v2u startP = (V2U(image->width, image->height) - (grid->size + V2U(2, 2)) * tileWidth) / 2;
        
        Rectangle2u topRow = rect_min_dim(startP + V2U(tileWidth, 0), V2U(grid->size.x * tileWidth, tileWidth));
        Rectangle2u botRow = rect_min_dim(startP + V2U(tileWidth, tileWidth * (grid->size.y + 1)),
                                          V2U(grid->size.x * tileWidth, tileWidth));
        Rectangle2u lefRow = rect_min_dim(startP + V2U(0, tileWidth), V2U(tileWidth, grid->size.y * tileWidth));
        Rectangle2u rigRow = rect_min_dim(startP + V2U(tileWidth * (grid->size.x + 1), tileWidth),
                                          V2U(tileWidth, grid->size.y * tileWidth));
        
        v4 lineColour = V4(0, 0.7f, 0.2f, 1);
        if (in_rectangle(topRow, mouseP))
        {
            draw_line(image, mouseP.x, mouseP.y, mouseP.x, botRow.min.y, lineColour);
        }
        else if (in_rectangle(botRow, mouseP))
        {
            draw_line(image, mouseP.x, mouseP.y, mouseP.x, topRow.max.y, lineColour);
        }
        else if (in_rectangle(lefRow, mouseP))
        {
            draw_line(image, mouseP.x, mouseP.y, rigRow.min.x, mouseP.y, lineColour);
        }
        else if (in_rectangle(rigRow, mouseP))
        {
            draw_line(image, mouseP.x, mouseP.y, lefRow.max.x, mouseP.y, lineColour);
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

