#include "interface.h"
DRAW_IMAGE(draw_image);

#include <fftw3.h>

#include "main.cpp"

#include "../libberdip/fft.cpp"
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
    v2 *reconstructSignal;
    
    v2 *origSinc;
    Complex32 *dftSinc;
    
    fftwf_plan fftwPlan;
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

internal void
idft(u32 dftCount, Complex32 *signal, v2 *reconstruct)
{
    f32 oneOverCount = 1.0f / (f32)dftCount;
    for (u32 dftIndex = 0; dftIndex < dftCount; ++dftIndex)
    {
        Complex32 sum = {};
        f32 powerStep = (f32)dftIndex * oneOverCount;
        for (u32 sampleIndex = 0; sampleIndex < dftCount; ++sampleIndex)
        {
            f32 k = powerStep * (f32)sampleIndex;
            Complex32 x;
            x.real = cos_f32(k);
            x.imag = sin_f32(k);
            Complex32 inp = signal[sampleIndex];
            sum += inp * x;
        }
        reconstruct[dftIndex].y = (oneOverCount * sum).real;
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
        Complex32 TStep;
        TStep.real = cos_f32(-oneOverCount);
        TStep.imag = sin_f32(-oneOverCount);
        Complex32 T = {1, 0};
        for (u32 index = 0; index < halfCount; ++index, T *= TStep)
        {
            Complex32 E = dftSignal[index];
            Complex32 O = dftSignal[index + halfCount];
            dftSignal[index] = E + T * O;
            dftSignal[index + halfCount] = E - T * O;
        }
    }
}

internal void
fft_recurse2(u32 dftCount, v2 *signal, Complex32 *dftSignal, u32 step = 1, u32 N = 16)
{
    i_expect(is_pow2(dftCount));
    
    if (dftCount <= N)
    {
        f32 oneOverCount = 1.0f / (f32)dftCount;
        for (u32 dftIndex = 0; dftIndex < dftCount; ++dftIndex)
        {
            Complex32 sum = {};
            f32 powerStep = -(f32)dftIndex * oneOverCount;
            u32 powerIndex = 0;
            for (u32 sampleIndex = 0; sampleIndex < step * dftCount; sampleIndex += step)
            {
                f32 inp = signal[sampleIndex].y;
                f32 k = powerStep * (f32)(powerIndex++);
                sum.real = sum.real + inp * cos_f32(k);
                sum.imag = sum.imag + inp * sin_f32(k);
            }
            dftSignal[dftIndex] = sum;
        }
    }
    else
    {
        u32 halfCount = dftCount / 2;
        fft_recurse2(halfCount, signal, dftSignal, step * 2, N);
        fft_recurse2(halfCount, signal + step, dftSignal + halfCount, step * 2, N);
        
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

internal void
ifft_recurse(u32 dftCount, Complex32 *signal, Complex32 *reconstruct, u32 step = 1)
{
    i_expect(is_pow2(dftCount));
    
    if (dftCount == 1)
    {
        *reconstruct = *signal;
    }
    else
    {
        u32 halfCount = dftCount / 2;
        ifft_recurse(halfCount, signal, reconstruct, step * 2);
        ifft_recurse(halfCount, signal + step, reconstruct + halfCount, step * 2);
        
        f32 oneOverCount = 1.0f / (f32)dftCount;
        Complex32 TStep;
        TStep.real = cos_f32(oneOverCount);
        TStep.imag = sin_f32(oneOverCount);
        Complex32 T = {1, 0};
        f32 amplMod = step * oneOverCount;
        for (u32 index = 0; index < halfCount; ++index, T *= TStep)
        {
            Complex32 E = reconstruct[index];
            Complex32 O = T * reconstruct[index + halfCount];
            reconstruct[index] = (E + O) * amplMod;
            reconstruct[index + halfCount] = (E - O) * amplMod;
        }
    }
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
        if (reversedIndex > index)
        {
            dftSignal[index] = {signal[reversedIndex].y, 0};
            dftSignal[reversedIndex] = {signal[index].y, 0};
        }
        else if (reversedIndex == index)
        {
            dftSignal[index] = {signal[index].y, 0};
        }
    }
    
    u32 halfM = 1;
    u32 m = 2;
    while (m <= dftCount)
    {
        //Complex32 Wm;
        
        f32 oneOverM = 1.0f / (f32)m;
        //Wm.real = cos_f32(-oneOverM);
        //Wm.imag = sin_f32(-oneOverM);
        
        for (u32 k = 0; k < dftCount; k += m)
        {
            //Complex32 w;
            //w.real = 1.0f;
            //w.imag = 0.0f;
            for (u32 j = 0; j < halfM; ++j)
            {
                Complex32 w;
                w.real = cos_f32(-(f32)j * oneOverM);
                w.imag = sin_f32(-(f32)j * oneOverM);
                Complex32 E = dftSignal[k + j];
                Complex32 O = w * dftSignal[k + j + halfM];
                dftSignal[k + j] = E + O;
                dftSignal[k + j + halfM] = E - O;
                //w = w * Wm;
            }
        }
        halfM = m;
        m <<= 1;
    }
}

internal void
fft_iter2(u32 dftCount, v2 *signal, Complex32 *dftSignal)
{
    i_expect(is_pow2(dftCount));
    i_expect(dftCount > 2);
    
    u32 baseDftCount = 8;
    f32 oneOverCount = 1.0f / (f32)baseDftCount;
    
    u32 dftStep = dftCount / baseDftCount;
    BitScanResult highBit = find_most_significant_set_bit(dftStep);
    
    for (u32 blockIndex = 0; blockIndex < dftStep; ++blockIndex)
    {
        u32 reversedIndex = reverse_bits(blockIndex, highBit.index);
        v2 *source = signal + reversedIndex;
        Complex32 *dest = dftSignal + blockIndex * baseDftCount;
        for (u32 dftIndex = 0; dftIndex < baseDftCount; ++dftIndex)
        {
            Complex32 *sum = dest + dftIndex;
            sum->real = 0.0f;
            sum->imag = 0.0f;
            f32 powerStep = -(f32)dftIndex * oneOverCount;
            u32 powerIndex = 0;
            for (u32 sampleIndex = 0; sampleIndex < dftCount; sampleIndex += dftStep)
            {
                f32 inp = source[sampleIndex].y;
                f32 k = powerStep * (f32)(powerIndex++);
                sum->real = sum->real + inp * cos_f32(k);
                sum->imag = sum->imag + inp * sin_f32(k);
            }
        }
    }
    
    u32 halfM = baseDftCount;
    u32 m = 2 * baseDftCount;
    
    while (m <= dftCount)
    {
        f32 oneOverM = 1.0f / (f32)m;
        //Complex32 Wm;
        //Wm.real = cos_f32(-oneOverM);
        //Wm.imag = sin_f32(-oneOverM);
        
        for (u32 k = 0; k < dftCount; k += m)
        {
            //Complex32 w;
            //w.real = 1.0f;
            //w.imag = 0.0f;
            Complex32 *src0 = dftSignal + k;
            Complex32 *src1 = dftSignal + k + halfM;
            for (u32 j = 0; j < halfM; ++j)
            {
                Complex32 w;
                w.real = cos_f32(-(f32)j * oneOverM);
                w.imag = sin_f32(-(f32)j * oneOverM);
                Complex32 E = *src0;
                Complex32 O = w * *src1;
                *src0++ = E + O;
                *src1++ = E - O;
                //w *= Wm;
            }
        }
        halfM = m;
        m <<= 1;
    }
}

internal void
ifft_iter(u32 dftCount, Complex32 *signal, Complex32 *reconstruct)
{
    i_expect(is_pow2(dftCount));
    i_expect(dftCount > 2);
    
    BitScanResult highBit = find_most_significant_set_bit(dftCount);
    for (u32 index = 0; index < dftCount; ++index)
    {
        u32 reversedIndex = reverse_bits(index, highBit.index);
        if (reversedIndex > index)
        {
            reconstruct[index] = signal[reversedIndex];
            reconstruct[reversedIndex] = signal[index];
        }
        else if (reversedIndex == index)
        {
            reconstruct[reversedIndex] = signal[index];
        }
    }
    
    u32 halfM = 1;
    u32 m = 2;
    u32 d = dftCount / 2;
    while (m <= dftCount)
    {
        Complex32 Wm;
        
        f32 oneOverM = 1.0f / (f32)m;
        Wm.real = cos_f32(oneOverM);
        Wm.imag = sin_f32(oneOverM);
        
        f32 amplMod = (f32)d * oneOverM;
        for (u32 k = 0; k < dftCount; k += m)
        {
            Complex32 w;
            w.real = 1.0f;
            w.imag = 0.0f;
            for (u32 j = 0; j < halfM; ++j)
            {
                Complex32 E = reconstruct[k + j];
                Complex32 O = w * reconstruct[k + j + halfM];
                reconstruct[k + j] = (E + O) * amplMod;
                reconstruct[k + j + halfM] = (E - O) * amplMod;
                w = w * Wm;
            }
        }
        halfM = m;
        m <<= 1;
        d >>= 1;
    }
}

internal void
fft_iter_inplace(u32 dftCount, Complex32 *signal)
{
    i_expect(is_pow2(dftCount));
    i_expect(dftCount > 2);
    
    BitScanResult highBit = find_most_significant_set_bit(dftCount);
    // NOTE(michiel): First and last item stay where they are.
    for (u32 index = 1; index < dftCount - 2; ++index)
    {
        u32 reversedIndex = reverse_bits(index, highBit.index);
        if (reversedIndex > index)
        {
            Complex32 temp = signal[index];
            signal[index] = signal[reversedIndex];
            signal[reversedIndex] = temp;
        }
    }
    
    u32 halfM = 1;
    u32 m = 2;
    while (m <= dftCount)
    {
        f32 oneOverM = 1.0f / (f32)m;
        Complex32 Wm;
        Wm.real = cos_f32(-oneOverM);
        Wm.imag = sin_f32(-oneOverM);
        
        for (u32 k = 0; k < dftCount; k += m)
        {
            Complex32 w;
            w.real = 1.0f;
            w.imag = 0.0f;
            for (u32 j = 0; j < halfM; ++j)
            {
                Complex32 E = signal[k + j];
                Complex32 O = w * signal[k + j + halfM];
                signal[k + j] = E + O;
                signal[k + j + halfM] = E - O;
                w = w * Wm;
            }
        }
        halfM = m;
        m <<= 1;
    }
}

internal void
ifft_iter_inplace(u32 dftCount, Complex32 *signal)
{
    i_expect(is_pow2(dftCount));
    i_expect(dftCount > 2);
    
    BitScanResult highBit = find_most_significant_set_bit(dftCount);
    for (u32 index = 0; index < dftCount; ++index)
    {
        u32 reversedIndex = reverse_bits(index, highBit.index);
        if (reversedIndex > index)
        {
            Complex32 temp = signal[index];
            signal[index] = signal[reversedIndex];
            signal[reversedIndex] = temp;
        }
    }
    
    u32 halfM = 1;
    u32 m = 2;
    u32 d = dftCount / 2;
    while (m <= dftCount)
    {
        Complex32 Wm;
        
        f32 oneOverM = 1.0f / (f32)m;
        Wm.real = cos_f32(oneOverM);
        Wm.imag = sin_f32(oneOverM);
        
        f32 amplMod = (f32)d * oneOverM;
        for (u32 k = 0; k < dftCount; k += m)
        {
            Complex32 w;
            w.real = 1.0f;
            w.imag = 0.0f;
            for (u32 j = 0; j < halfM; ++j)
            {
                Complex32 E = signal[k + j];
                Complex32 O = w * signal[k + j + halfM];
                signal[k + j] = (E + O) * amplMod;
                signal[k + j + halfM] = (E - O) * amplMod;
                w = w * Wm;
            }
        }
        halfM = m;
        m <<= 1;
        d >>= 1;
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
        
        fftState->origSignalCount = 1024;
        fftState->origSignal = allocate_array(v2, fftState->origSignalCount);
        fftState->dftSignal = allocate_array(Complex32, fftState->origSignalCount);
        fftState->reconstructSignal = allocate_array(v2, fftState->origSignalCount);
        
#if 1
        f32 oneOverCount = 1.0f / (f32)fftState->origSignalCount;
        for (u32 idx = 0; idx < fftState->origSignalCount; ++idx)
        {
            v2 *point = fftState->origSignal + idx;
            f32 mod = oneOverCount * (f32)idx;
            point->x = mod;
            //point->y = ((f32)(idx % 4) / 3.0f) * 2.0f - 1.0f;
            point->y = sin_pi(mod * 4.0f * F32_TAU) + 0.5f * sin_pi(mod * 6.0f * F32_TAU - 0.5f * F32_PI);
            //point->y = sin(mod * 400.0f * F32_TAU) + 0.5f * sin(mod * 600.0f * F32_TAU - 0.5f * F32_PI);
            //point->y = random_bilateral(&fftState->randomizer);
            
            v2 *point2 = fftState->reconstructSignal + idx;
            point2->x = mod;
            point2->y = 0.0f;
        }
#else
        f32 oneOverCount = 1.0f / (f32)fftState->origSignalCount;
        u32 halfIdx = (fftState->origSignalCount + 1) / 2;
        for (u32 idx = 0; idx < fftState->origSignalCount; ++idx)
        {
            v2 *point = fftState->origSignal + idx;
            f32 mod = oneOverCount * (f32)idx;
            point->x = mod;
            
            f32 calcX = ((f32)idx - (f32)halfIdx);
            if (calcX == 0.0f) {
                point->y = 1.0f;
            } else {
                point->y = sin_pi(calcX) / calcX;
            }
            point->y *= 1.0f / F32_PI;
            
            v2 *point2 = fftState->reconstructSignal + idx;
            point2->x = mod;
            point2->y = 0.0f;
        }
#endif
        
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
        fftState->fftwPlan = fftwf_plan_dft_1d(fftState->origSignalCount, (fftwf_complex *)fftState->dftSignal,
                                               (fftwf_complex *)fftState->dftSignal, FFTW_FORWARD, FFTW_MEASURE);
        
        state->initialized = true;
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    draw_lines(image, fftState->origSignalCount, fftState->origSignal, V2(10.0f, 0.25f * size.y), V2(0.45f * size.x, -0.2f * size.y), V4(1, 1, 1, 1));
    
    TempMemory tempMem = temporary_memory(&fftState->drawArena);
    
    //dft(fftState->origSignalCount, fftState->origSignal, fftState->dftSignal);
    //fft_recurse(fftState->origSignalCount, fftState->origSignal, fftState->dftSignal);
    //fft_recurse2(fftState->origSignalCount, fftState->origSignal, fftState->dftSignal, 1, 2);
    //fft_iter(fftState->origSignalCount, fftState->origSignal, fftState->dftSignal);
    //fft_iter2(fftState->origSignalCount, fftState->origSignal, fftState->dftSignal);
    for (u32 idx = 0; idx < fftState->origSignalCount; ++idx)
    {
        fftState->dftSignal[idx] = {fftState->origSignal[idx].y, 0};
    }
    //fft_iter_inplace(fftState->origSignalCount, fftState->dftSignal);
    //fft(fftState->origSignalCount, fftState->dftSignal);
    //fftwf_execute(fftState->fftwPlan);
    fft_exact(fftState->origSignalCount, fftState->dftSignal);
    
    Complex32 *idftBuf = arena_allocate_array(&fftState->drawArena, Complex32, fftState->origSignalCount);
    //idft(fftState->origSignalCount, fftState->dftSignal, fftState->reconstructSignal);
    //ifft_recurse(fftState->origSignalCount, fftState->dftSignal, idftBuf);
    //ifft_iter(fftState->origSignalCount, fftState->dftSignal, idftBuf);
    for (u32 idx = 0; idx < fftState->origSignalCount; ++idx)
    {
        idftBuf[idx] = fftState->dftSignal[idx];
    }
    //ifft_iter_inplace(fftState->origSignalCount, idftBuf);
    //ifft(fftState->origSignalCount, idftBuf);
    ifft_exact(fftState->origSignalCount, idftBuf);
    
    for (u32 idx = 0; idx < fftState->origSignalCount; ++idx)
    {
        fftState->reconstructSignal[idx].y = idftBuf[idx].real;
    }
    
    f32 oneOverCount = 1.0f / (f32)fftState->origSignalCount;
    v3 *dftSig = arena_allocate_array(&fftState->drawArena, v3, fftState->origSignalCount);
    v3 *dftLogSig = arena_allocate_array(&fftState->drawArena, v3, fftState->origSignalCount);
    for (u32 idx = 0; idx < fftState->origSignalCount; ++idx)
    {
        Complex32 *dftPoint = fftState->dftSignal + idx;
        v3 *point = dftSig + idx;
        v3 *logPoint = dftLogSig + idx;
        f32 mod = oneOverCount * (f32)idx;
        point->x = mod;
        
        v2 magAng = magnitude_angle(*dftPoint);
        point->y = magAng.x * oneOverCount;
        point->z = magAng.y;
        
        logPoint->x = mod;
        //logPoint->x = log(1.0f + (f32)idx * (1.0f / (0.5f * (f32)fftState->origSignalCount)) * (0.5f * size.x - 20.0f));
        logPoint->y = 20.0f * log(point->y);
        logPoint->z = 0.0; //point->z;
    }
    
    draw_lines(image, fftState->origSignalCount, dftSig, V3(0.5f * size.x + 10.0f, 0.5f * size.y, 0.25f * size.y),
               V3(0.5f * size.x - 20.0f, -0.5f * size.y + 20.0f, -0.25f * size.y / F32_PI + 20.0f), V4(1, 1, 1, 1), V4(1, 0, 0, 1));
    
    draw_lines(image, fftState->origSignalCount, dftLogSig, V3(10.0f, 0.75f * size.y - 300.0f, 0.75f * size.y),
               V3(0.5f * size.x - 20.0f, -1.0f, -0.25f * size.y / F32_PI + 20.0f), V4(1, 1, 1, 1), V4(1, 0, 0, 1));
    
    //draw_lines(image, fftState->origSignalCount, dftLogSig, V3(10.0f, 0.75f * size.y - 300.0f, 0.75f * size.y),
    //V3(60.0f, -1.0f, -0.25f * size.y / F32_PI + 20.0f), V4(1, 1, 1, 1), V4(1, 0, 0, 1));
    
    draw_lines(image, fftState->origSignalCount, fftState->reconstructSignal, V2(0.5f * size.x + 10.0f, 0.75f * size.y), V2(0.45f * size.x, -0.2f * size.y), V4(1, 1, 1, 1));
    
    fftState->prevMouseDown = mouse.mouseDowns;
    fftState->seconds += dt;
    ++fftState->ticks;
    if (fftState->seconds > 1.0f)
    {
        fftState->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", fftState->ticks,
                1000.0f / (f32)fftState->ticks);
        fftState->ticks = 0;
        
        fprintf(stdout, "F0: %f\n", dftLogSig[0].y);
        fprintf(stdout, "FE: %f\n", dftLogSig[fftState->origSignalCount - 1].y);
        fprintf(stdout, "FH: %f\n", dftLogSig[fftState->origSignalCount / 2].y);
        fprintf(stdout, "FH: %f\n", dftLogSig[fftState->origSignalCount / 2 - 1].y);
        fprintf(stdout, "FH: %f\n", dftLogSig[fftState->origSignalCount / 2 - 2].y);
        fprintf(stdout, "FH: %f\n", dftLogSig[fftState->origSignalCount / 2 - 3].y);
        fprintf(stdout, "FH: %f\n", dftLogSig[fftState->origSignalCount / 2 - 4].y);
        fprintf(stdout, "FH: %f\n", dftLogSig[fftState->origSignalCount / 2 - 5].y);
        fprintf(stdout, "FH: %f\n", dftLogSig[fftState->origSignalCount / 2 - 6].y);
        fprintf(stdout, "FH: %f\n", dftLogSig[fftState->origSignalCount / 2 - 7].y);
        fprintf(stdout, "FH: %f\n", dftLogSig[fftState->origSignalCount / 2 - 8].y);
        fprintf(stdout, "FH-1: %f\n", dftLogSig[fftState->origSignalCount / 2 - 1].y);
        fprintf(stdout, "FH+1: %f\n", dftLogSig[fftState->origSignalCount / 2 + 1].y);
        fprintf(stdout, "FX: %f\n", dftLogSig[4].y);
        fprintf(stdout, "FX-1: %f\n", dftLogSig[3].y);
        fprintf(stdout, "FX+1: %f\n", dftLogSig[5].y);
        fprintf(stdout, "FY: %f\n", dftLogSig[6].y);
        fprintf(stdout, "FY-1: %f\n", dftLogSig[5].y);
        fprintf(stdout, "FY+1: %f\n", dftLogSig[7].y);
    }
    
    destroy_temporary(tempMem);
}

