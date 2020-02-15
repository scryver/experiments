#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "../libberdip/perlin.h"
#include "../libberdip/multilane.h"
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
dft(u32 dftCount, v2 *signal, Complex32 *dftSignal, u32 step = 1)
{
    f32 oneOverCount = 1.0f / (f32)dftCount;
    for (u32 dftIndex = 0; dftIndex < dftCount; ++dftIndex)
    {
        Complex32 sum = {};
        f32 powerStep = -(f32)dftIndex * oneOverCount;
        for (u32 sampleIndex = 0; sampleIndex < dftCount; sampleIndex += step)
        {
            f32 inp = signal[sampleIndex].y;
            f32 k = powerStep * (f32)sampleIndex;
            sum.real = sum.real + inp * cos_f32(k);
            sum.imag = sum.imag + inp * sin_f32(k);
        }
        dftSignal[dftIndex] = sum;
    }
}

internal void
fft_recurse(u32 dftCount, v2 *signal, Complex32 *dftSignal, u32 step = 1)
{
    i_expect(is_pow2(dftCount));
    
    if (dftCount == 1)
    {
        dftSignal[0].real = signal[0].y;
        dftSignal[0].imag = 0;
    }
    else
    {
        u32 halfCount = dftCount / 2;
        fft_recurse(halfCount, signal, dftSignal, step * 2);
        fft_recurse(halfCount, signal + step, dftSignal + halfCount, step * 2);
        
        f32 oneOverCount = 1.0f / (f32)dftCount;
        for (u32 index = 0; index < halfCount; ++index)
        {
            Complex32 T;
            T.real = cos_f32(-(f32)index * oneOverCount);
            T.imag = sin_f32(-(f32)index * oneOverCount);
            
            Complex32 E = dftSignal[index];
            Complex32 O = dftSignal[index + halfCount];
            dftSignal[index] = E + T * O;
            dftSignal[index + halfCount] = E - T * O;
        }
    }
}

internal u32
reverse_bits(u32 b, u32 msb)
{
    u32 mask = (1 << msb) - 1;
    u32 result = b;
    --msb;
    for (b >>= 1; b; b>>= 1)
    {
        result <<= 1;
        result |= b & 1;
        --msb;
    }
    result <<= msb;
    return result & mask;
}

internal f32_4x
reverse_bits_4x(f32_4x b, u32 msb)
{
    //u32 origMsb = msb;
    f32_4x one = S32_4x(1);
    f32_4x shiftCount = S32_4x(msb, 0, 0, 0);
    f32_4x mask = s32_4x_sub(s32_4x_sll(one, shiftCount), one);
    
    shiftCount = S32_4x(1, 0, 0, 0);
    
    f32_4x result = b;
    --msb;
    
    for (b = s32_4x_srl(b, shiftCount); any(b); b = s32_4x_srl(b, shiftCount))
    {
        result = s32_4x_sll(result, shiftCount);
        result = s32_4x_or(result, s32_4x_and(b, one));
        --msb;
    }
    
    shiftCount = S32_4x(msb, 0, 0, 0);
    result = s32_4x_sll(result, shiftCount);
    result = s32_4x_and(result, mask);
    return result;
}

internal void
fft_iter(u32 dftCount, v2 *signal, Complex32 *dftSignal)
{
    i_expect(is_pow2(dftCount));
    i_expect(dftCount > 2);
    
    BitScanResult highBit = find_most_significant_set_bit(dftCount);
    for (u32 index = 0; index < dftCount; ++index)
    {
        u32 reversedIndex = reverse_bits(index, highBit.index);
        Complex32 *c = dftSignal + reversedIndex;
        c->real = signal[index + 0].y;
        c->imag = 0.0f;
    }
    
    u32 halfM = 1;
    u32 m = 2;
    while (m <= dftCount)
    {
        Complex32 Wm;
        
        f32 oneOverM = 1.0f / (f32)m;
        Wm.real = cos_f32(-oneOverM);
        Wm.imag = sin_f32(-oneOverM);
        
        for (u32 k = 0; k < dftCount; k += m)
        {
            Complex32 w;
            w.real = 1.0f;
            w.imag = 0.0f;
            for (u32 j = 0; j < halfM; ++j)
            {
                Complex32 E = dftSignal[k + j];
                Complex32 O = w * dftSignal[k + j + halfM];
                dftSignal[k + j] = E + O;
                dftSignal[k + j + halfM] = E - O;
                w = w * Wm;
            }
        }
        halfM = m;
        m <<= 1;
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
        
        fftState->origSignalCount = 8192;
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
            //point->y = sin(mod * 400.0f * F32_TAU) + 0.5f * sin(mod * 600.0f * F32_TAU - 0.5f * F32_PI);
            //point->y = random_bilateral(&fftState->randomizer);
        }
        
#if 0        
        u32 indices[16];
        BitScanResult highBit = find_most_significant_set_bit(array_count(indices));
        f32_4x reversed = reverse_bits_4x(S32_4x(0, 1, 2, 3), highBit.index);
        indices[0] = reversed.u[0];
        indices[1] = reversed.u[1];
        indices[2] = reversed.u[2];
        indices[3] = reversed.u[3];
        
        reversed = reverse_bits_4x(S32_4x(4, 5, 6, 7), highBit.index);
        indices[4] = reversed.u[0];
        indices[5] = reversed.u[1];
        indices[6] = reversed.u[2];
        indices[7] = reversed.u[3];
        for (u32 i = 0; i < 8; ++i)
        {
            fprintf(stdout, "%u => %u\n", i, indices[i]);
        }
        state->closeProgram = true;
#endif
        
        state->initialized = true;
    }
    
    //dft(fftState->origSignalCount, fftState->origSignal, fftState->dftSignal);
    //fft_recurse(fftState->origSignalCount, fftState->origSignal, fftState->dftSignal);
    fft_iter(fftState->origSignalCount, fftState->origSignal, fftState->dftSignal);
    
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

