#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

// NOTE(michiel): For drawing mostly
enum CellFlags
{
    Cell_Empty    = 0x0,
    Cell_StartPos = 0x1,
Cell_Wall     = 0x2,
    Cell_Visited  = 0x4,
    Cell_Active   = 0x8,
    };
struct Cell 
{
    v2u p;
    enum32(CellFlags) flags;
    f32 cost;
    
    Cell *prev;
};

struct PathState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    u32 tileWidth;
    u32 rows;
    u32 columns;
    Cell *cells;
    
    u32 visitAt;
    u32 visitCount;
    v2u *toVisit;
    f32 *costs;
    
    Cell *pathStart;
};

internal inline Cell *
get_cell(u32 rows, u32 columns, Cell *cells, u32 x, u32 y)
{
    i_expect(x < columns);
    i_expect(y < rows);
    return cells + y * columns + x;
}

internal inline Cell *
get_cell(PathState *state, u32 x, u32 y)
{
    return get_cell(state->rows, state->columns, state->cells, x, y);
}

internal inline b32
not_visited(Cell *cell)
{
    return !(cell->flags & Cell_Visited) && !(cell->flags & Cell_Wall);
}

internal inline b32
not_in_waiting_queue(u32 queueCount, v2u *queue, v2u p, u32 searchStart = 0)
{
    b32 result = true;
    for (u32 queueIndex = searchStart; 
         queueIndex < queueCount;
         ++queueIndex)
    {
        if ((queue[queueIndex].x == p.x) &&
            (queue[queueIndex].y == p.y))
        {
             result = false;
            break;
        }
    }
    return result;
}

internal void
push_neighbour(PathState *state, u32 x, u32 y, Cell *prev = 0)
{
    Cell *cell = get_cell(state, x, y);
    if (not_visited(cell) &&
        not_in_waiting_queue(state->visitCount, state->toVisit, V2U(x, y),
                             state->visitAt))
    {
    cell->prev = prev;
    state->toVisit[state->visitCount++] = V2U(x, y);
    }
}

 internal void
bread_first(PathState *state)
{
    v2u at = state->toVisit[state->visitAt];
    Cell *cell = get_cell(state, at.x, at.y);
    
    if (at.y > 0)
    {
        push_neighbour(state, at.x, at.y - 1, cell);
    }
    if (at.x > 0)
    {
        push_neighbour(state, at.x - 1, at.y, cell);
    }
    if (at.y < state->rows - 1)
    {
        push_neighbour(state, at.x, at.y + 1, cell);
    }
    if (at.x < state->columns - 1)
    {
        push_neighbour(state, at.x + 1, at.y, cell);
    }
    
    cell->flags |= Cell_Visited;
    cell->flags &= ~Cell_Active;
    ++state->visitAt;
    if (state->visitAt < state->visitCount)
    {
    Cell *next = get_cell(state, state->toVisit[state->visitAt].x,
                          state->toVisit[state->visitAt].y);
    next->flags |= Cell_Active;
    }
}

internal void
push_cost_neighbour(PathState *state, u32 x, u32 y, Cell *prev, f32 costAt)
{
    Cell *cell = get_cell(state, x, y);
    
    if (not_visited(cell))
    {
        f32 cost = costAt + cell->cost;
        b32 found = false;
        
        for (u32 queueIndex = state->visitAt; queueIndex < state->visitCount; ++queueIndex)
        {
            if ((state->toVisit[queueIndex].x == cell->p.x) &&
                (state->toVisit[queueIndex].y == cell->p.y))
            {
                if (cost < state->costs[queueIndex])
                {
                    cell->prev = prev;
                    state->costs[queueIndex] = cost;
                }
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            cell->prev = prev;
            u32 insertionIndex = state->visitCount;
            for (u32 queueIndex = state->visitAt; queueIndex < state->visitCount; ++queueIndex)
            {
                if (state->costs[queueIndex] > cost)
                {
                    insertionIndex = queueIndex;
            break;
                }
            }
            
            for (u32 queueIndex = state->visitCount; queueIndex > insertionIndex; --queueIndex)
            {
                state->toVisit[queueIndex] = state->toVisit[queueIndex - 1];
                state->costs[queueIndex] = state->costs[queueIndex - 1];
            }
            
        state->toVisit[insertionIndex] = V2U(x, y);
                state->costs[insertionIndex] = cost;
            ++state->visitCount;
            }
    }
}

internal void
dijkstra(PathState *state)
{
    v2u at = state->toVisit[state->visitAt];
    f32 costAt = state->costs[state->visitAt];
    Cell *cell = get_cell(state, at.x, at.y);
    
    if (at.y > 0)
    {
        push_cost_neighbour(state, at.x, at.y - 1, cell, costAt);
        }
    if (at.x > 0)
    {
        push_cost_neighbour(state, at.x - 1, at.y, cell, costAt);
    }
    if (at.y < state->rows - 1)
    {
        push_cost_neighbour(state, at.x, at.y + 1, cell, costAt);
    }
    if (at.x < state->columns - 1)
    {
        push_cost_neighbour(state, at.x + 1, at.y, cell, costAt);
    }
    
    cell->flags |= Cell_Visited;
    cell->flags &= ~Cell_Active;
    ++state->visitAt;
    if (state->visitAt < state->visitCount)
    {
        Cell *next = get_cell(state, state->toVisit[state->visitAt].x,
                              state->toVisit[state->visitAt].y);
        next->flags |= Cell_Active;
    }
}

internal void
init_grid(PathState *state)
{
    v2u startingPos = V2U(8, 8);
    v2u walls[] = {
        V2U(4, 4),
        V2U(5, 12),
        V2U(14, 5),
        V2U(15, 20),
        V2U(22, 0),
        V2U(23, 5),
        V2U(22, 6),
        V2U(26, 7),
        V2U(2, 21),
        V2U(29, 22),
    };
    
    for (u32 row = 0; row < state->rows; ++row)
    {
        for (u32 col = 0; col < state->columns; ++col)
        {
            Cell *cell = get_cell(state->rows, state->columns, state->cells,
                                  col, row);
            cell->p.x = col;
            cell->p.y = row;
            cell->flags = 0;
            cell->cost = 1.0f; //random_unilateral(&state->randomizer) * 9.0f + 1.0f;
            if ((col == startingPos.x) &&
                (row == startingPos.y))
            {
                cell->flags |= Cell_StartPos;
            }
            for (u32 wIndex = 0; wIndex < array_count(walls); wIndex += 2)
            {
                v2u wallMin = walls[wIndex];
                v2u wallMax = walls[wIndex + 1];
                
                if ((wallMin.x <= col) && (col <= wallMax.x) &&
                    (wallMin.y <= row) && (row <= wallMax.y))
                {
                    cell->flags |= Cell_Wall;
                    cell->cost += 10000.0f;
                    break;
                }
            }
            
            if ((10 <= col) && (col < 30) &&
                (15 <= row) && (row < 20))
            {
                cell->cost += 10.0f;
            }
        }
    }
    
    state->visitAt = 0;
    state->visitCount = 1;
    state->toVisit[0] = startingPos;
    state->costs[0] = 0.0f;
    }

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(PathState) <= state->memorySize);
    
    // v2 size = V2((f32)image->width, (f32)image->height);
    
    PathState *pathState = (PathState *)state->memory;
    if (!state->initialized)
    {
        // pathState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        pathState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        pathState->tileWidth = 20;
        pathState->columns = image->width / pathState->tileWidth;
        pathState->rows = image->height / pathState->tileWidth;
        pathState->cells = allocate_array(Cell, pathState->columns * pathState->rows);
        pathState->toVisit = allocate_array(v2u, pathState->rows * pathState->columns);
        pathState->costs = allocate_array(f32, pathState->rows * pathState->columns);
        
        init_grid(pathState);
        
        state->initialized = true;
    }
    
    if ((mouse.mouseDowns & Mouse_Left)
        //&& !(pathState->prevMouseDown & Mouse_Left)
        )
    {
        bread_first(pathState);
    }
    
    if ((mouse.mouseDowns & Mouse_Extended2)
        //&& !(pathState->prevMouseDown & Mouse_Extended2)
        )
    {
        dijkstra(pathState);
    }
    
    if ((mouse.mouseDowns & Mouse_Right) &&
        !(pathState->prevMouseDown & Mouse_Right))
    {
        init_grid(pathState);
    }
    
    for (u32 cellIdx = 0; cellIdx < pathState->rows * pathState->columns; ++cellIdx)
    {
        Cell *cell = pathState->cells + cellIdx;
        f32 wh = pathState->tileWidth;
        f32 x = cell->p.x * wh;
        f32 y = cell->p.y * wh;
        
        if ((x <= (f32)mouse.pixelPosition.x) && ((f32)mouse.pixelPosition.x < (x + wh)) &&
            (y <= (f32)mouse.pixelPosition.y) && ((f32)mouse.pixelPosition.y < (y + wh)))
            {
            pathState->pathStart = cell;
            }
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    for (u32 cellIdx = 0; cellIdx < pathState->rows * pathState->columns; ++cellIdx)
    {
        Cell *cell = pathState->cells + cellIdx;
        
        v4 colour = V4(cell->cost / 10.0f, 0.5f, 0.5f, 1.0f);
        if (cell->flags & Cell_StartPos)
        {
            colour = V4(0.5f, 0.0f, 0.0f, 1.0f);
        }
        else if (cell->flags & Cell_Wall)
        {
            colour = V4(0.2f, 0.2f, 0.2f, 1.0f);
        }
         if (cell->flags & Cell_Active)
        {
            colour -= 0.2f;
            colour.g = 1.0f;
                colour = clamp01(colour);
        }
         if (cell->flags & Cell_Visited)
        {
                colour += 0.3f;
                colour = clamp01(colour);
        }
        
        u32 wh = pathState->tileWidth;
        u32 x = cell->p.x * wh;
        u32 y = cell->p.y * wh;
        fill_rectangle(image, x + 1, y + 1, wh - 2, wh - 2, colour);
    }
    
    for (u32 cellIdx = 0; cellIdx < pathState->rows * pathState->columns; ++cellIdx)
    {
        Cell *cell = pathState->cells + cellIdx;
        Cell *prev = cell->prev;
        
        if (prev)
        {
            u32 wh = pathState->tileWidth;
            u32 x1 = cell->p.x * wh + wh / 2;
            u32 y1 = cell->p.y * wh + wh / 2;
            u32 x2 = prev->p.x * wh + wh / 2;
            u32 y2 = prev->p.y * wh + wh / 2;
            draw_line(image, x2, y2, x1, y1, V4(0, 0, 0, 1));
            }
    }
    
    Cell *path = pathState->pathStart;
    while (path)
    {
        Cell *prev = path->prev;
        
        if (prev)
        {
            u32 wh = pathState->tileWidth;
            u32 x1 = path->p.x * wh + wh / 2;
            u32 y1 = path->p.y * wh + wh / 2;
            u32 x2 = prev->p.x * wh + wh / 2;
            u32 y2 = prev->p.y * wh + wh / 2;
            draw_line(image, x2, y2, x1, y1, V4(1, 0, 0, 1));
        }
        
        path = prev;
    }
    
    pathState->prevMouseDown = mouse.mouseDowns;
    pathState->seconds += dt;
    ++pathState->ticks;
    if (pathState->seconds > 1.0f)
    {
        pathState->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", pathState->ticks,
                1000.0f / (f32)pathState->ticks);
        f32 cost = 0.0f;
        for (u32 cellIndex = 0; cellIndex < pathState->visitCount; ++cellIndex)
        {
            v2u pos = pathState->toVisit[cellIndex];
            if ((pos.x == pathState->pathStart->p.x) &&
                (pos.y == pathState->pathStart->p.y))
            {
                cost = pathState->costs[cellIndex];
                break;
            }
        }
        fprintf(stdout, "Cost: %f\n", cost);
        pathState->ticks = 0;
    }
}
