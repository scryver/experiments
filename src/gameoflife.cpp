#include "interface.h"
DRAW_IMAGE(draw_image);

#include <time.h>

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct Cell
{
    v2u pos;
    
    b32 alive;
};

struct Grid
{
    u32 columns;
    u32 rows;
    Cell *cells[2];
    u32 currentActive;
};

enum RunState
{
    RunState_Paused,
    RunState_Slow,
    RunState_Normal,
};
struct GoLState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    u32 prevMouseDown;
    
    b32 border;
    RunState running;
    
    u32 cellCount;
    Grid grid;
};

global u32 width = 10;

internal inline Cell *
get_cell(Grid *grid, u32 x, u32 y)
{
    Cell *result = 0;
    if ((x < grid->columns) &&
        (y < grid->rows))
    {
        result = grid->cells[grid->currentActive] + (y * grid->columns) + x;
    }
    return result;
}

internal u32
get_alive_neighbours(Grid *grid, Cell *cell)
{
    u32 result = 0;
    
    Cell *neighbour;
    
    neighbour = get_cell(grid, cell->pos.x - 1, cell->pos.y - 1);
    if (neighbour && neighbour->alive)
    {
        ++result;
    }
    neighbour = get_cell(grid, cell->pos.x, cell->pos.y - 1);
    if (neighbour && neighbour->alive)
    {
        ++result;
    }
    neighbour = get_cell(grid, cell->pos.x + 1, cell->pos.y - 1);
    if (neighbour && neighbour->alive)
    {
        ++result;
    }
    
    neighbour = get_cell(grid, cell->pos.x - 1, cell->pos.y);
    if (neighbour && neighbour->alive)
    {
        ++result;
    }
    neighbour = get_cell(grid, cell->pos.x + 1, cell->pos.y);
    if (neighbour && neighbour->alive)
    {
        ++result;
    }
    
    neighbour = get_cell(grid, cell->pos.x - 1, cell->pos.y + 1);
    if (neighbour && neighbour->alive)
    {
        ++result;
    }
    neighbour = get_cell(grid, cell->pos.x, cell->pos.y + 1);
    if (neighbour && neighbour->alive)
    {
        ++result;
    }
    neighbour = get_cell(grid, cell->pos.x + 1, cell->pos.y + 1);
    if (neighbour && neighbour->alive)
    {
        ++result;
    }
    
    return result;
}

DRAW_IMAGE(draw_image)
{
    GoLState *gameState = (GoLState *)state->memory;
    if (!state->initialized)
    {
        gameState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        Grid *grid = &gameState->grid;
        grid->columns = image->width / width;
        grid->rows = image->height / width;
        grid->cells[0] = allocate_array(Cell, grid->columns * grid->rows);
        grid->cells[1] = allocate_array(Cell, grid->columns * grid->rows);
        grid->currentActive = 0;
        
        gameState->cellCount = grid->columns * grid->rows;
        u32 cellIndex = 0;
        for (u32 row = 0; row < grid->rows; ++row)
        {
            for (u32 col = 0; col < grid->columns; ++col)
            {
                Cell *cell = grid->cells[0] + cellIndex;
                cell->pos.x = col;
                cell->pos.y = row;
                cell->alive = false;
                cell = grid->cells[1] + cellIndex;
                cell->pos.x = col;
                cell->pos.y = row;
                cell->alive = false;
                
                ++cellIndex;
            }
        }
        
        state->initialized = true;
    }
    
    Grid *grid = &gameState->grid;
    
    v2 mouseP = mouse.pixelPosition;
    mouseP.x /= (f32)width;
    mouseP.y /= (f32)width;
    
    if (mouse.mouseDowns & Mouse_Left)
    {
        Cell *cell = get_cell(grid, u32_from_f32_truncate(mouseP.x), u32_from_f32_truncate(mouseP.y));
        cell->alive = true;
    }
    
    if ((mouse.mouseDowns & Mouse_Right) &&
        !(gameState->prevMouseDown & Mouse_Right))
    {
        switch(gameState->running)
        {
            case RunState_Paused: { gameState->running = RunState_Slow; } break;
            case RunState_Slow: { gameState->running = RunState_Normal; } break;
            case RunState_Normal: { gameState->running = RunState_Paused; } break;
            INVALID_DEFAULT_CASE;
        }
    }
    
    if ((mouse.mouseDowns & Mouse_Extended1) &&
        !(gameState->prevMouseDown & Mouse_Left))
    {
        for (u32 cellIndex = 0; cellIndex < gameState->cellCount; ++cellIndex)
        {
            Cell *cell = grid->cells[0] + cellIndex;
            cell->alive = false;
            cell = grid->cells[1] + cellIndex;
            cell->alive = false;
        }
    }
    
    if ((mouse.mouseDowns & Mouse_Extended2) &&
        !(gameState->prevMouseDown & Mouse_Extended2))
    {
        gameState->border = !gameState->border;
    }
    
    gameState->prevMouseDown = mouse.mouseDowns;
    
    if ((gameState->running == RunState_Normal) ||
        ((gameState->running == RunState_Slow) &&
         ((gameState->ticks % 30) == 0)))
    {
        u32 nextActive = (grid->currentActive + 1) & 1;
        for (u32 row = 0; row < grid->rows; ++row)
        {
            for (u32 col = 0; col < grid->columns; ++col)
            {
                Cell *current = grid->cells[grid->currentActive] + row * grid->columns + col;
                Cell *next = grid->cells[nextActive] + row * grid->columns + col;
                *next = *current;
                
                u32 aliveNeighbourCount = get_alive_neighbours(grid, current);
                if (current->alive && (aliveNeighbourCount < 2))
                {
                    next->alive = false;
                }
                if (current->alive && (aliveNeighbourCount > 3))
                {
                    next->alive = false;
                }
                if (!current->alive && (aliveNeighbourCount == 3))
                {
                    next->alive = true;
                }
            }
        }
        grid->currentActive = nextActive;
    }
    
    
    //
    // NOTE(michiel): Draw cells
    //
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 1, 0, 1));
    
    for (u32 cellIndex = 0; cellIndex < gameState->cellCount; ++cellIndex)
    {
        Cell *cell = grid->cells[grid->currentActive] + cellIndex;
        
        v2u min = cell->pos * width;
        v2u max = min + width;
        
        if (gameState->border)
        {
            // NOTE(michiel): Border
            outline_rectangle(image, min.x, min.y, max.x - min.x, max.y - min.y,
                              V4(1, 1, 1, 1));
            min += 1;
            max -= 1;
        }
        
        fill_rectangle(image, min.x, min.y, max.x - min.x, max.y - min.y,
                       cell->alive ? V4(0, 0, 0, 1) : V4(0.7f, 0.7f, 0.7f, 1));
    }
    
    ++gameState->ticks;
}
