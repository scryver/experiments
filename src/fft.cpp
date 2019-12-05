#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "../libberdip/perlin.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct FFTState
{
    Arena drawArena;
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    u32 origSignalCount;
    v2 *origSignal;
    Complex32 *dftSignal;
    
    v2 *origSinc;
    Complex32 *dftSinc;
};

internal void
dft(u32 dftCount, v2 *signal, Complex32 *dftSignal)
{
    f32 oneOverCount = 1.0f / (f32)dftCount;
    for (u32 dftIndex = 0; dftIndex < dftCount; ++dftIndex)
    {
        Complex32 sum = {};
        f32 powerStep = -(f32)dftIndex * oneOverCount;
        for (u32 sampleIndex = 0; sampleIndex < dftCount; ++sampleIndex)
        {
            f32 inp = signal[sampleIndex].y;
            f32 k = powerStep * (f32)sampleIndex;
            sum.real = sum.real + inp * cos_f32(k);
            sum.imag = sum.imag + inp * sin_f32(k);
        }
        dftSignal[dftIndex] = sum;
    }
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(FFTState) <= state->memorySize);
    
    v2 size = V2((f32)image->width, (f32)image->height);
    
    FFTState *fftState = (FFTState *)state->memory;
    if (!state->initialized)
    {
        // fftState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        fftState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        fftState->origSignalCount = 32;
        fftState->origSignal = allocate_array(v2, fftState->origSignalCount);
        fftState->dftSignal = allocate_array(Complex32, fftState->origSignalCount);
        
        f32 oneOverCount = 1.0f / (f32)fftState->origSignalCount;
        for (u32 idx = 0; idx < fftState->origSignalCount; ++idx)
        {
            v2 *point = fftState->origSignal + idx;
            f32 mod = oneOverCount * (f32)idx;
            point->x = mod;
            //point->y = ((f32)(idx % 4) / 3.0f) * 2.0f - 1.0f;
            point->y = sin(mod * 4.0f * F32_TAU) + 0.5f * sin(mod * 6.0f * F32_TAU - 0.5f * F32_PI);
            //point->y = random_bilateral(&fftState->randomizer);
        }
        
        state->initialized = true;
    }
    
    dft(fftState->origSignalCount, fftState->origSignal, fftState->dftSignal);
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    draw_lines(image, fftState->origSignalCount, fftState->origSignal, V2(10.0f, 0.25f * size.y), V2(0.45f * size.x, -0.2f * size.y), V4(1, 1, 1, 1));
    
    TempMemory tempMem = temporary_memory(&fftState->drawArena);
    
    f32 oneOverCount = 1.0f / (f32)fftState->origSignalCount;
    v3 *dftSig = arena_allocate_array(&fftState->drawArena, v3, fftState->origSignalCount);
    for (u32 idx = 0; idx < fftState->origSignalCount; ++idx)
    {
        Complex32 *dftPoint = fftState->dftSignal + idx;
        v3 *point = dftSig + idx;
        f32 mod = oneOverCount * (f32)idx;
        point->x = mod;
        
        v2 magAng = magnitude_angle(*dftPoint);
        point->y = magAng.x * oneOverCount;
        point->z = magAng.y;
    }
    
    draw_lines(image, fftState->origSignalCount, dftSig, V3(0.5f * size.x + 10.0f, 0.5f * size.y, 0.25f * size.y),
               V3(0.5f * size.x - 20.0f, -0.5f * size.y + 20.0f, -0.25f * size.y / F32_PI + 20.0f), V4(1, 1, 1, 1), V4(1, 0, 0, 1));
    
    destroy_temporary(tempMem);
    
    fftState->prevMouseDown = mouse.mouseDowns;
    fftState->seconds += dt;
    ++fftState->ticks;
    if (fftState->seconds > 1.0f)
    {
        fftState->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", fftState->ticks,
                1000.0f / (f32)fftState->ticks);
        fftState->ticks = 0;
    }
}

