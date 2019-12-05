#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "../libberdip/perlin.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct ConjectureState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    PerlinNoise perlin;
    f32 modPerlin;
    f32 modPerlin2;
    
    u32 maxSequenceCount;
    u32 sequenceCount;
    s32 *sequence;
};

internal s32
collatz(s32 number)
{
    s32 result;
    if (number & 0x1)
    {
        // NOTE(michiel): Odd
        //result = 3 * number + 1;
        result = (3 * number + 1) / 2;
    }
    else
    {
        // NOTE(michiel): Even
        result = number / 2;
    }
    return result;
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(ConjectureState) <= state->memorySize);
    
    v2 size = V2((f32)image->width, (f32)image->height);
    
    ConjectureState *conjecture = (ConjectureState*)state->memory;
    if (!state->initialized)
    {
        // conjecture->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        conjecture->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        init_perlin_noise(&conjecture->perlin, &conjecture->randomizer);
        
        conjecture->maxSequenceCount = 1024 * 1024;
        conjecture->sequence = allocate_array(s32, conjecture->maxSequenceCount);
        
        state->initialized = true;
        
#if 0        
        s32 number = (s32)(random_next_u32(&conjecture->randomizer) >> 8);
        u32 steps = 0;
        //fprintf(stdout, "%d\n", number);
        do
        {
            number = collatz(number);
            ++steps;
            //fprintf(stdout, "%d\n", number);
        } while (number != 1);
        fprintf(stdout, "%u => Steps: %u\n", conjecture->inputNumber, steps);
        state->closeProgram = true;
#endif
        
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    f32 len = 5.0f;
    f32 angle = (F32_PI / 12.0f) * absolute(perlin_noise(&conjecture->perlin, conjecture->modPerlin)); // 0.15f;
    //s32 number = 100;
    s32 max = 100 + s32_from_f32_round(10000.0f * absolute(perlin_noise(&conjecture->perlin, conjecture->modPerlin2)));
    for (s32 number = 1; number < max; ++number)
    {
        s32 n = number;
        conjecture->sequenceCount = 0;
        while (n != 1)
        {
            if (conjecture->sequenceCount < conjecture->maxSequenceCount)
            {
                conjecture->sequence[conjecture->sequenceCount++] = n;
            }
            else
            {
                break;
            }
            n = collatz(n);
        }
        if (conjecture->sequenceCount < conjecture->maxSequenceCount)
        {
            conjecture->sequence[conjecture->sequenceCount++] = 1;
        }
        
        v2 baseP = V2(size.x * 0.5f, size.y - 20.0f);
        //v2 baseP = 0.5f * size;
        //v2 baseP = V2(0, size.y * 0.5f);
        f32 a = angle;
        for (u32 idx = 0; idx < conjecture->sequenceCount; ++idx)
        {
            u32 realIdx = conjecture->sequenceCount - idx - 1;
            s32 value = conjecture->sequence[realIdx];
            
            //v2 point = V2(len, 0);
            v2 point = V2(0, -len);
            if (value & 0x1)
            {
                a -= angle;
            }
            else
            {
                a += angle;
            }
            point = rotate(point, a);
            draw_line(image, baseP, baseP + point, V4(1, 1, 0, 0.5f / ((f32)idx + 1)));
            baseP += point;
        }
    }
    
    conjecture->modPerlin += 0.01f;
    if (conjecture->modPerlin > 256.0f)
    {
        conjecture->modPerlin -= 256.0f;
    }
    
    conjecture->modPerlin2 += 0.021f;
    if (conjecture->modPerlin2 > 256.0f)
    {
        conjecture->modPerlin2 -= 256.0f;
    }
    
    conjecture->prevMouseDown = mouse.mouseDowns;
    conjecture->seconds += dt;
    ++conjecture->ticks;
    if (conjecture->seconds > 1.0f)
    {
        conjecture->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", conjecture->ticks,
                1000.0f / (f32)conjecture->ticks);
        conjecture->ticks = 0;
    }
}

