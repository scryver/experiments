#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct City
{
    s32 x;
    s32 y;
};

struct SalesmanState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    b32 doneSearching;
    
    u32 cityCount;
    City *cities;
    u32 *currentPath;
    
    u32 *bestEver;
    u32 bestDistanceSqr;
};

internal void
swap(u32 cityCount, City *cities, u32 aIndex, u32 bIndex)
{
    i_expect(aIndex < cityCount);
    i_expect(bIndex < cityCount);
    City temp = cities[aIndex];
    cities[aIndex] = cities[bIndex];
    cities[bIndex] = temp;
}

internal void
swap(u32 indexCount, u32 *indices, u32 aIndex, u32 bIndex)
{
    i_expect(aIndex < indexCount);
    i_expect(bIndex < indexCount);
    u32 temp = indices[aIndex];
    indices[aIndex] = indices[bIndex];
    indices[bIndex] = temp;
}

internal u32
calc_distance_sqr(u32 cityCount, City *cities)
{
    i_expect(cityCount);
    u32 result = 0;
    City *prevCity = 0;
    for (u32 cityIndex = 0; cityIndex < cityCount; ++cityIndex)
    {
        City *city = cities + cityIndex;
        if (prevCity)
        {
            s32 dx = city->x - prevCity->x;
            s32 dy = city->y - prevCity->y;
            
            s32 distSqr = dx * dx + dy * dy;
            result += distSqr;
        }
        prevCity = city;
    }
    return result;
}

internal u32
calc_distance_sqr(u32 cityCount, City *cities, u32 *path)
{
    i_expect(cityCount);
    u32 result = 0;
    City *prevCity = 0;
    for (u32 pathIndex = 0; pathIndex < cityCount; ++pathIndex)
    {
        u32 cityIndex = path[pathIndex];
        City *city = cities + cityIndex;
        if (prevCity)
        {
            s32 dx = city->x - prevCity->x;
            s32 dy = city->y - prevCity->y;
            
            s32 distSqr = dx * dx + dy * dy;
            result += distSqr;
        }
        prevCity = city;
    }
    return result;
}

DRAW_IMAGE(draw_image)
{
    SalesmanState *salesmanState = (SalesmanState *)state->memory;
    if (!state->initialized)
    {
        //salesmanState->randomizer = random_seed_pcg(time(0), 1357984324984344247ULL);
        salesmanState->randomizer = random_seed_pcg(132049210274214ULL, 1357984324984344247ULL);
        
        salesmanState->cityCount = 8;
        salesmanState->cities = allocate_array(City, salesmanState->cityCount);
        salesmanState->currentPath = allocate_array(u32, salesmanState->cityCount);
        salesmanState->bestEver = allocate_array(u32, salesmanState->cityCount);
        
        for (u32 cityIndex = 0; cityIndex < salesmanState->cityCount; ++cityIndex)
        {
            City *city = salesmanState->cities + cityIndex;
            city->x = random_choice(&salesmanState->randomizer, image->width);
            city->y = random_choice(&salesmanState->randomizer, image->height);
            salesmanState->currentPath[cityIndex] = cityIndex;
            salesmanState->bestEver[cityIndex] = cityIndex;
        }
        
        salesmanState->bestDistanceSqr = calc_distance_sqr(salesmanState->cityCount, 
                                                           salesmanState->cities,
                                                           salesmanState->currentPath);
        state->initialized = true;
    }
    
    //
    // NOTE(michiel): Update
    //
    
    if (1) //(salesmanState->ticks % 32) == 0)
    {
#if 0
        // NOTE(michiel): Random swapping cities to try
        u32 randomA = random_choice(&salesmanState->randomizer, salesmanState->cityCount);
        u32 randomB = random_choice(&salesmanState->randomizer, salesmanState->cityCount);
        swap(salesmanState->cityCount, salesmanState->currentPath, randomA, randomB);
#else
        // NOTE(michiel): Do in order
        if (!salesmanState->doneSearching)
        {
            u32 largestX = 0;
            u32 largestXIndex = 0xFFFFFFFF;
            for (u32 i = 0; i < salesmanState->cityCount - 1; ++i)
            {
                if (salesmanState->currentPath[i] < salesmanState->currentPath[i + 1])
                {
                    largestX = salesmanState->currentPath[i];
                    largestXIndex = i;
                }
            }
            
            if (largestXIndex == 0xFFFFFFFF)
            {
                salesmanState->doneSearching = true;
            }
            else
            {
                u32 largestY = 0;
                u32 largestYIndex = 0xFFFFFFFF;
                for (u32 i = largestXIndex; i < salesmanState->cityCount; ++i)
                {
                    if (largestX < salesmanState->currentPath[i])
                    {
                        largestY = salesmanState->currentPath[i];
                        largestYIndex = i;
                    }
                }
                i_expect(largestYIndex != 0xFFFFFFFF);
                salesmanState->currentPath[largestXIndex] = largestY;
                salesmanState->currentPath[largestYIndex] = largestX;
                
                u32 minRev = largestXIndex + 1;
                u32 maxRev = minRev + (salesmanState->cityCount - minRev) / 2;
                
                for (u32 rev = minRev; rev < maxRev; ++rev)
                {
                    u32 offset = salesmanState->cityCount - 1;
                    offset -= (rev - minRev);
                    u32 temp = salesmanState->currentPath[rev];
                    salesmanState->currentPath[rev] = salesmanState->currentPath[offset];
                    salesmanState->currentPath[offset] = temp;
                }
            }
        }
#endif
        
        u32 distSqr = calc_distance_sqr(salesmanState->cityCount, salesmanState->cities,
                                        salesmanState->currentPath);
        if (salesmanState->bestDistanceSqr > distSqr)
        {
            salesmanState->bestDistanceSqr = distSqr;
            
            for (u32 cityIndex = 0; cityIndex < salesmanState->cityCount; ++cityIndex)
            {
                salesmanState->bestEver[cityIndex] = salesmanState->currentPath[cityIndex];
            }
            
            fprintf(stdout, "Best: %f\n", square_root((f32)salesmanState->bestDistanceSqr));
        }
    }
    
    //
    // NOTE(michiel): Render
    //
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    City *prevCity = 0;
    for (u32 pathIndex = 0; pathIndex < salesmanState->cityCount; ++pathIndex)
    {
        u32 cityIndex = salesmanState->currentPath[pathIndex];
        City *city = salesmanState->cities + cityIndex;
        
        fill_circle(image, city->x, city->y, 4, V4(1, 1, 1, 1));
        if (prevCity)
        {
            draw_line(image, prevCity->x, prevCity->y, city->x, city->y,
                      V4(0.7f, 0.7f, 0.7f, 1));
        }
        prevCity = city;
    }
    
    City *prevBestCity = 0;
    for (u32 bestIndex = 0; bestIndex < salesmanState->cityCount; ++bestIndex)
    {
        u32 cityIndex = salesmanState->bestEver[bestIndex];
        City *bestCity = salesmanState->cities + cityIndex;
        
        if (prevBestCity)
        {
            draw_line(image, prevBestCity->x, prevBestCity->y, bestCity->x, bestCity->y,
                      V4(0.7f, 0.7f, 1, 1));
        }
        prevBestCity = bestCity;
    }
    
    ++salesmanState->ticks;
}
