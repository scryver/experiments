#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct City
{
    v2 position;
};

struct RandomSwap
{
    u32 *current;
    
    u32 *bestEver;
    f32 bestDistanceSqr;
};

struct BruteForce
{
    u32 *current;
    
    u32 *bestEver;
    f32 bestDistanceSqr;
    
    b32 doneSearching;
};

struct Evolve
{
    u32 populationCount;
    u32 **population;
    f32 *fitness;
    u32 **nextGeneration;
    
    u32 *bestEver;
    f32 bestDistanceSqr;
    u32 *currentBest;
};

struct SalesmanState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    b32 doneSearching;
    
    u32 cityCount;
    City *cities;
    
    RandomSwap randomSwap;
    BruteForce bruter;
    Evolve evolver;
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

internal void
shuffle(RandomSeriesPCG *random, u32 indexCount, u32 *indices, u32 times)
{
    for (u32 i = 0; i < times; ++i)
    {
        u32 indexA = random_choice(random, indexCount);
        u32 indexB = random_choice(random, indexCount);
        swap(indexCount, indices, indexA, indexB);
    }
}

internal f32
calc_distance_sqr(u32 cityCount, City *cities)
{
    i_expect(cityCount);
    f32 result = 0;
    City *prevCity = 0;
    for (u32 cityIndex = 0; cityIndex < cityCount; ++cityIndex)
    {
        City *city = cities + cityIndex;
        if (prevCity)
        {
            v2 dist = city->position - prevCity->position;
            result += length_squared(dist);
        }
        prevCity = city;
    }
    return result;
}

internal f32
calc_distance_sqr(u32 cityCount, City *cities, u32 *path)
{
    i_expect(cityCount);
    f32 result = 0;
    City *prevCity = 0;
    for (u32 pathIndex = 0; pathIndex < cityCount; ++pathIndex)
    {
        u32 cityIndex = path[pathIndex];
        City *city = cities + cityIndex;
        if (prevCity)
        {
            v2 dist = city->position - prevCity->position;
            result += length_squared(dist);
        }
        prevCity = city;
    }
    return result;
}

internal inline void
copy_indices(u32 count, u32 *src, u32 *dst)
{
    for (u32 i = 0; i < count; ++i)
    {
        dst[i] = src[i];
    }
}

internal void
calc_fitness(SalesmanState *state)
{
    f32 bestDistSqr = F32_INF;
    for (u32 populationIndex = 0;
         populationIndex < state->evolver.populationCount;
         ++populationIndex)
    {
        u32 *population = state->evolver.population[populationIndex];
        f32 distSqr = calc_distance_sqr(state->cityCount, state->cities, population);
        if (bestDistSqr > distSqr)
        {
            bestDistSqr = distSqr;
            state->evolver.currentBest = population;
        }
        state->evolver.fitness[populationIndex] = 1.0f / (square_root(distSqr) + 1.0f);
    }
    
    if (state->evolver.bestDistanceSqr > bestDistSqr)
    {
        state->evolver.bestDistanceSqr = bestDistSqr;
        copy_indices(state->cityCount, state->evolver.currentBest, state->evolver.bestEver);
    }
}

internal void
normalize_fitness(SalesmanState *state)
{
    f64 sum = 0.0;
    for (u32 fitnessIndex = 0; fitnessIndex < state->evolver.populationCount; ++fitnessIndex)
    {
        sum += state->evolver.fitness[fitnessIndex];
    }
    f32 oneOverSum = 1.0f / sum;
    for (u32 fitnessIndex = 0; fitnessIndex < state->evolver.populationCount; ++fitnessIndex)
    {
        state->evolver.fitness[fitnessIndex] *= oneOverSum;
    }
}

internal void
cross_over(RandomSeriesPCG *random, u32 cityCount, u32 *parentA, u32 *parentB, 
           u32 *children)
{
    u32 startIndex = random_choice(random, cityCount / 2);
    u32 endIndex = random_choice(random, cityCount / 2);
    if ((startIndex + endIndex) >= cityCount)
    {
        endIndex = cityCount - startIndex;
    }
    
    u32 childIndex = 0;
    for (u32 i = startIndex; i < endIndex; ++i)
    {
        children[childIndex++] = parentA[i];
    }
    
    u32 firstHalf = childIndex;
    
    for (u32 i = 0; i < cityCount; ++i)
    {
        u32 parentN = parentB[i];
        b32 found = false;
        for (u32 j = 0; j < firstHalf; ++j)
        {
            if (children[j] == parentN)
            {
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            children[childIndex++] = parentN;
        }
    }
}

internal void
mutate(RandomSeriesPCG *random, u32 cityCount, u32 *indices,
       f32 mutationRate, f32 fullMutateRate)
{
    for (u32 i = 0; i < cityCount; ++i)
    {
        if (random_unilateral(random) < mutationRate)
        {
            u32 indexA = random_choice(random, cityCount);
            //u32 indexB = random_choice(random, cityCount);
            u32 indexB = indexA + 1;
            if (indexB >= cityCount)
            {
                indexB -= 2;
            }
            swap(cityCount, indices, indexA, indexB);
        }
        if (random_unilateral(random) < fullMutateRate)
        {
            u32 indexA = random_choice(random, cityCount);
            u32 indexB = random_choice(random, cityCount);
            swap(cityCount, indices, indexA, indexB);
        }
    }
}

internal s32
pick_one(SalesmanState *state)
{
    s32 index = 0;
    
    f32 r = random_unilateral(&state->randomizer);
    
    while ((r > 0.0f) && (index < state->evolver.populationCount))
    {
        r = r - state->evolver.fitness[index];
        ++index;
    }
    return index - 1;
}

internal void
next_generation(SalesmanState *state)
{
    for (u32 popIndex = 0; popIndex < state->evolver.populationCount; ++popIndex)
    {
        s32 nextGenIndexA = pick_one(state);
        s32 nextGenIndexB = pick_one(state);
        if ((nextGenIndexA >= 0) && (nextGenIndexB >= 0))
        {
            cross_over(&state->randomizer, state->cityCount, 
                       state->evolver.population[nextGenIndexA],
                       state->evolver.population[nextGenIndexB],
                       state->evolver.nextGeneration[popIndex]);
            
            mutate(&state->randomizer, state->cityCount,
                   state->evolver.nextGeneration[popIndex], 0.01f, 0.001f);
        }
    }
    
    u32 **tempAddr = state->evolver.population;
    state->evolver.population = state->evolver.nextGeneration;
    state->evolver.nextGeneration = tempAddr;
}

DRAW_IMAGE(draw_image)
{
    SalesmanState *salesmanState = (SalesmanState *)state->memory;
    if (!state->initialized)
    {
        salesmanState->randomizer = random_seed_pcg(time(0), 1357984324984344247ULL);
        //salesmanState->randomizer = random_seed_pcg(132049210274214ULL, 1357984324984344247ULL);
        
        salesmanState->cityCount = 20;
        salesmanState->cities = allocate_array(City, salesmanState->cityCount);
        
        for (u32 cityIndex = 0; cityIndex < salesmanState->cityCount; ++cityIndex)
        {
            City *city = salesmanState->cities + cityIndex;
            city->position.x = random_unilateral(&salesmanState->randomizer) * (image->width / 2 - 1);
            city->position.y = random_unilateral(&salesmanState->randomizer) * (image->height / 2 - 1);
        }
        
        //
        // NOTE(michiel): Random changes
        //
        salesmanState->randomSwap.current = allocate_array(u32, salesmanState->cityCount);
        salesmanState->randomSwap.bestEver = allocate_array(u32, salesmanState->cityCount);
        
        for (u32 cityIndex = 0; cityIndex < salesmanState->cityCount; ++cityIndex)
        {
            salesmanState->randomSwap.current[cityIndex] = cityIndex;
            salesmanState->randomSwap.bestEver[cityIndex] = cityIndex;
        }
        salesmanState->randomSwap.bestDistanceSqr = 
            calc_distance_sqr(salesmanState->cityCount, salesmanState->cities,
                              salesmanState->randomSwap.bestEver);
        
        //
        // NOTE(michiel): Brute-force changes
        //
        salesmanState->bruter.current = allocate_array(u32, salesmanState->cityCount);
        salesmanState->bruter.bestEver = allocate_array(u32, salesmanState->cityCount);
        
        for (u32 cityIndex = 0; cityIndex < salesmanState->cityCount; ++cityIndex)
        {
            salesmanState->bruter.current[cityIndex] = cityIndex;
            salesmanState->bruter.bestEver[cityIndex] = cityIndex;
        }
        salesmanState->bruter.bestDistanceSqr = 
            calc_distance_sqr(salesmanState->cityCount, salesmanState->cities,
                              salesmanState->bruter.bestEver);
        
        //
        // NOTE(michiel): Evolve changes
        //
        salesmanState->evolver.bestEver = allocate_array(u32, salesmanState->cityCount);
        
        salesmanState->evolver.populationCount = 500;
        salesmanState->evolver.population = allocate_array(u32 *, salesmanState->evolver.populationCount);
        salesmanState->evolver.fitness = allocate_array(f32, salesmanState->evolver.populationCount);
        salesmanState->evolver.nextGeneration = allocate_array(u32 *, salesmanState->evolver.populationCount);
        
        for (u32 populationIndex = 0;
             populationIndex < salesmanState->evolver.populationCount;
             ++populationIndex)
        {
            salesmanState->evolver.nextGeneration[populationIndex] =
                allocate_array(u32, salesmanState->cityCount);
            
            u32 *population = salesmanState->evolver.population[populationIndex] =
                allocate_array(u32, salesmanState->cityCount);
            
            for (u32 cityIndex = 0; cityIndex < salesmanState->cityCount; ++cityIndex)
            {
                population[cityIndex] = cityIndex;
            }
            shuffle(&salesmanState->randomizer, salesmanState->cityCount, 
                    population, 100);
        }
        
        salesmanState->evolver.bestDistanceSqr = F32_INF;
        
        state->initialized = true;
    }
    
    //
    // NOTE(michiel): Update
    //
    if (1) // ((salesmanState->ticks % 40) == 0)
    {
        //
        // NOTE(michiel): Random changes
        //
        {
            u32 randomA = random_choice(&salesmanState->randomizer, salesmanState->cityCount);
            u32 randomB = random_choice(&salesmanState->randomizer, salesmanState->cityCount);
            swap(salesmanState->cityCount, salesmanState->randomSwap.current, randomA, randomB);
            
            f32 distSqr = calc_distance_sqr(salesmanState->cityCount, salesmanState->cities,
                                            salesmanState->randomSwap.current);
            if (salesmanState->randomSwap.bestDistanceSqr > distSqr)
            {
                copy_indices(salesmanState->cityCount, salesmanState->randomSwap.current,
                             salesmanState->randomSwap.bestEver);
                salesmanState->randomSwap.bestDistanceSqr = distSqr;
            }
        }
        
        //
        // NOTE(michiel): Brute-force changes
        //
        {
            if (!salesmanState->bruter.doneSearching)
            {
                u32 largestX = 0;
                u32 largestXIndex = U32_MAX;
                
                for (u32 i = 0; i < salesmanState->cityCount - 1; ++i)
                {
                    if (salesmanState->bruter.current[i] < salesmanState->bruter.current[i + 1])
                    {
                        largestX = salesmanState->bruter.current[i];
                        largestXIndex = i;
                    }
                }
                
                if (largestXIndex == U32_MAX)
                {
                    salesmanState->bruter.doneSearching = true;
                }
                else
                {
                    u32 largestY = 0;
                    u32 largestYIndex = U32_MAX;
                    
                    for (u32 i = largestXIndex; i < salesmanState->cityCount; ++i)
                    {
                        if (largestX < salesmanState->bruter.current[i])
                        {
                            largestY = salesmanState->bruter.current[i];
                            largestYIndex = i;
                        }
                    }
                    
                    i_expect(largestYIndex != U32_MAX);
                    salesmanState->bruter.current[largestXIndex] = largestY;
                    salesmanState->bruter.current[largestYIndex] = largestX;
                    
                    u32 minRev = largestXIndex + 1;
                    u32 maxRev = minRev + (salesmanState->cityCount - minRev) / 2;
                    
                    for (u32 rev = minRev; rev < maxRev; ++rev)
                    {
                        u32 offset = salesmanState->cityCount - 1;
                        offset -= (rev - minRev);
                        u32 temp = salesmanState->bruter.current[rev];
                        salesmanState->bruter.current[rev] = salesmanState->bruter.current[offset];
                        salesmanState->bruter.current[offset] = temp;
                    }
                }
            }
            
            f32 distSqr = calc_distance_sqr(salesmanState->cityCount, salesmanState->cities,
                                            salesmanState->bruter.current);
            if (salesmanState->bruter.bestDistanceSqr > distSqr)
            {
                copy_indices(salesmanState->cityCount, salesmanState->bruter.current,
                             salesmanState->bruter.bestEver);
                salesmanState->bruter.bestDistanceSqr = distSqr;
            }
        }
        
        //
        // NOTE(michiel): Evolve changes
        //
        calc_fitness(salesmanState);
        normalize_fitness(salesmanState);
        next_generation(salesmanState);
    }
    
    //
    // NOTE(michiel): Render
    //
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
#if 0
    v4 colours[] {
        {1, 0, 0, 1},
        {0, 1, 0, 1},
        {0, 0, 0, 1},
        {0.5f, 0, 0, 1},
        {0, 0.5f, 0, 1},
        {0, 0, 0.5f, 1},
        {0.5f, 0.2f, 0, 1},
        {0.5f, 0, 0.2f, 1},
        {0.2f, 0.5f, 0, 1},
        {0, 0.5f, 0.2f, 1},
        {0.2f, 0, 0.5f, 1},
        {0, 0.2f, 0.5f, 1},
        {0.2f, 0.2f, 0.2f, 1},
        {0.5f, 0.5f, 0.5f, 1},
        {1, 1, 1, 1},
    };
    
    for (u32 populationIndex = 0; populationIndex < salesmanState->populationCount; ++populationIndex)
    {
        City *prevCity = 0;
        for (u32 pathIndex = 0; pathIndex < salesmanState->cityCount; ++pathIndex)
        {
            u32 cityIndex = salesmanState->population[populationIndex][pathIndex];
            City *city = salesmanState->cities + cityIndex;
            
            v4 baseColour = colours[populationIndex % array_count(colours)];
            
            fill_circle(image, city->position.x, city->position.y, 4, baseColour);
            if (prevCity)
            {
                draw_line(image, prevCity->position.x, prevCity->position.y, 
                          city->position.x, city->position.y,
                          baseColour * 0.7f);
            }
            prevCity = city;
        }
    }
#endif
    
    u32 halfWidth = image->width / 2;
    u32 halfHeight = image->height / 2;
    
    if (salesmanState->randomSwap.bestEver)
    {
        City *prevCity = 0;
        for (u32 pathIndex = 0; pathIndex < salesmanState->cityCount; ++pathIndex)
        {
            u32 cityIndex = salesmanState->randomSwap.current[pathIndex];
            City *city = salesmanState->cities + cityIndex;
            
            v4 baseColour = V4(0.7f, 0, 0, 1);
            
            fill_circle(image, city->position.x, city->position.y + halfHeight,
                        4, baseColour);
            if (prevCity)
            {
                draw_line(image, prevCity->position.x, prevCity->position.y + halfHeight, 
                          city->position.x, city->position.y + halfHeight,
                          baseColour * 0.7f);
            }
            prevCity = city;
        }
    }
    
    if (salesmanState->bruter.bestEver)
    {
        City *prevCity = 0;
        for (u32 pathIndex = 0; pathIndex < salesmanState->cityCount; ++pathIndex)
        {
            u32 cityIndex = salesmanState->bruter.current[pathIndex];
            City *city = salesmanState->cities + cityIndex;
            
            v4 baseColour = V4(0, 0.7f, 0, 1);
            
            fill_circle(image, city->position.x + halfWidth, city->position.y + halfHeight,
                        4, baseColour);
            if (prevCity)
            {
                draw_line(image, prevCity->position.x + halfWidth, prevCity->position.y + halfHeight, 
                          city->position.x + halfWidth, city->position.y + halfHeight,
                          baseColour * 0.7f);
            }
            prevCity = city;
        }
    }
    
    if (salesmanState->evolver.currentBest)
    {
        City *prevCity = 0;
        for (u32 pathIndex = 0; pathIndex < salesmanState->cityCount; ++pathIndex)
        {
            u32 cityIndex = salesmanState->evolver.currentBest[pathIndex];
            City *city = salesmanState->cities + cityIndex;
            
            v4 baseColour = V4(0.7f, 0.7f, 0.7f, 1);
            
            fill_circle(image, city->position.x + halfWidth, city->position.y, 4, baseColour);
            if (prevCity)
            {
                draw_line(image, prevCity->position.x + halfWidth, prevCity->position.y, 
                          city->position.x + halfWidth, city->position.y,
                          baseColour * 0.7f);
            }
            prevCity = city;
        }
    }
    
    if ((salesmanState->randomSwap.bestEver) && 
        (salesmanState->bruter.bestEver) && 
        (salesmanState->evolver.bestEver))
    {
        u32 *bestEver = 0;
        v4 colour = {};
        
        if ((salesmanState->randomSwap.bestDistanceSqr < salesmanState->bruter.bestDistanceSqr) &&
            (salesmanState->randomSwap.bestDistanceSqr < salesmanState->evolver.bestDistanceSqr))
        {
            bestEver = salesmanState->randomSwap.bestEver;
            colour = V4(0.7f, 0, 0, 1);
        }
        
        if ((salesmanState->bruter.bestDistanceSqr < salesmanState->randomSwap.bestDistanceSqr) &&
            (salesmanState->bruter.bestDistanceSqr < salesmanState->evolver.bestDistanceSqr))
        {
            bestEver = salesmanState->bruter.bestEver;
            colour = V4(0, 0.7f, 0, 1);
        }
        
        if ((salesmanState->evolver.bestDistanceSqr < salesmanState->randomSwap.bestDistanceSqr) &&
            (salesmanState->evolver.bestDistanceSqr < salesmanState->bruter.bestDistanceSqr))
        {
            bestEver = salesmanState->evolver.bestEver;
            colour = V4(0, 0, 0.7f, 1);
        }
        
        if (bestEver)
        {
            City *prevBestCity = 0;
            for (u32 bestIndex = 0; bestIndex < salesmanState->cityCount; ++bestIndex)
            {
                u32 cityIndex = salesmanState->evolver.bestEver[bestIndex];
                City *bestCity = salesmanState->cities + cityIndex;
                
                if (prevBestCity)
                {
                    draw_line(image, prevBestCity->position.x, prevBestCity->position.y, 
                              bestCity->position.x, bestCity->position.y, colour);
                }
                prevBestCity = bestCity;
            }
        }
    }
    
    ++salesmanState->ticks;
}
