#include "interface.h"
DRAW_IMAGE(draw_image);

#include <time.h>

#include "main.cpp"

enum Walls
{
    Wall_Top = 0x1,
    Wall_Right = 0x2,
    Wall_Bottom = 0x4,
    Wall_Left = 0x8,
};
struct Cell
{
    u32 x;
    u32 y;
    u32 walls;
    b32 visited;
};

struct Grid
{
    u32 columns;
    u32 rows;
    Cell *cells;
};

struct MazeState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    u32 cellCount;
    Grid grid;
    
    Cell *current;
    
    u32 stackCount;
    Cell **visitedStack;
};

global u32 width = 10;

internal inline Cell *
get_cell(Grid *grid, u32 x, u32 y)
{
    Cell *result = 0;
    if ((x < grid->columns) &&
        (y < grid->rows))
    {
        result = grid->cells + (y * grid->columns) + x;
    }
    return result;
}

internal Cell *
next_cell(RandomSeriesPCG *randomizer, Grid *grid, Cell *current)
{
    u32 neighbourCount = 0;
    Cell *neighbours[4] = {};
    
    if (current->y > 0)
    {
        // NOTE(michiel): Top neighbour
        neighbours[neighbourCount] = get_cell(grid, current->x, current->y - 1);
        if (!neighbours[neighbourCount]->visited)
        {
            ++neighbourCount;
        }
    }
    if (current->x < (grid->columns - 1))
    {
        // NOTE(michiel): Right neighbour
        neighbours[neighbourCount] = get_cell(grid, current->x + 1, current->y);
        if (!neighbours[neighbourCount]->visited)
        {
            ++neighbourCount;
        }
    }
    if (current->y < (grid->rows - 1))
    {
        // NOTE(michiel): Bottom neighbour
        neighbours[neighbourCount] = get_cell(grid, current->x, current->y + 1);
        if (!neighbours[neighbourCount]->visited)
        {
            ++neighbourCount;
        }
    }
    if (current->x > 0)
    {
        // NOTE(michiel): Left neighbour
        neighbours[neighbourCount] = get_cell(grid, current->x - 1, current->y);
        if (!neighbours[neighbourCount]->visited)
        {
            ++neighbourCount;
        }
    }
    
    Cell *result = 0;
    if (neighbourCount)
    {
        u32 randomNeighbour = random_choice(randomizer, neighbourCount);
        result = neighbours[randomNeighbour];
    }
    
    return result;
}

internal void
remove_walls(Cell *a, Cell *b)
{
    s32 x = a->x - b->x;
    if (x == 1)
    {
        a->walls &= ~Wall_Left;
        b->walls &= ~Wall_Right;
    }
    else if (x == -1)
    {
        a->walls &= ~Wall_Right;
        b->walls &= ~Wall_Left;
    }
    
    s32 y = a->y - b->y;
    if (y == 1)
    {
        a->walls &= ~Wall_Top;
        b->walls &= ~Wall_Bottom;
    }
    else if (y == -1)
    {
        a->walls &= ~Wall_Bottom;
        b->walls &= ~Wall_Top;
    }
}

DRAW_IMAGE(draw_image)
{
    MazeState *mazeState = (MazeState *)state->memory;
    if (!state->initialized)
    {
        mazeState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        Grid *grid = &mazeState->grid;
        grid->columns = image->width / width;
        grid->rows = image->height / width;
        grid->cells = allocate_array(Cell, grid->columns * grid->rows);
        
        mazeState->cellCount = 0;
        for (u32 row = 0; row < grid->rows; ++row)
        {
            for (u32 col = 0; col < grid->columns; ++col)
            {
                Cell *cell = grid->cells + mazeState->cellCount++;
                cell->x = col;
                cell->y = row;
                cell->walls = Wall_Left|Wall_Right|Wall_Bottom|Wall_Top;
                cell->visited = false;
            }
        }
        
        mazeState->current = mazeState->grid.cells;
        
        mazeState->stackCount = 0;
        mazeState->visitedStack = allocate_array(Cell *, mazeState->cellCount);
        
        state->initialized = true;
    }
    
    Grid *grid = &mazeState->grid;
    Cell *current = mazeState->current;
    current->visited = true;
    
    if (1) // (mazeState->ticks % 16) == 0)
    {
        Cell *next = next_cell(&mazeState->randomizer, grid, current);
        if (next)
        {
            i_expect(mazeState->stackCount < mazeState->cellCount);
            mazeState->visitedStack[mazeState->stackCount++] = current;
            remove_walls(current, next);
            mazeState->current = next;
        }
        else if (mazeState->stackCount)
        {
            mazeState->current = mazeState->visitedStack[--mazeState->stackCount];
        }
    }
    //
    // NOTE(michiel): Draw cells
    //
    
    for (u32 cellIndex = 0; cellIndex < mazeState->cellCount; ++cellIndex)
    {
        Cell *cell = grid->cells + cellIndex;
        
        u32 minX = cell->x * width;
        u32 minY = cell->y * width;
        u32 maxX = (cell->x + 1) * width;
        u32 maxY = (cell->y + 1) * width;
        
        if (cell->walls & Wall_Left)
        {
            minX += 1;
        }
        if (cell->walls & Wall_Right)
        {
            maxX -= 1;
        }
        if (cell->walls & Wall_Top)
        {
            minY += 1;
        }
        if (cell->walls & Wall_Bottom)
        {
            maxY -= 1;
        }
        
        for (u32 y = minY; (y < maxY) && (y < image->height); ++y)
        {
            for (u32 x = minX; (x < maxX) && (x < image->width); ++x)
            {
                u32 colour = 0xFF00FF00;
                if (cell == current)
                {
                    colour = 0xFFFFFFFF;
                }
                else if (cell->visited)
                {
                    colour = 0xFFFF0000;
                }
                
                image->pixels[y * image->rowStride + x] = colour;
            }
        }
    }
    
    ++mazeState->ticks;
}
