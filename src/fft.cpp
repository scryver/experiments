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
    v2 *reconstructSignal;
    
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

internal void
fft_iter_inplace2(u32 dftCount, Complex32 *signal)
{
    i_expect(is_pow2(dftCount));
    i_expect(dftCount > 4);
    
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
    
    Complex32 Wm4;
    Wm4.real = cos_f32(-0.25f);
    Wm4.imag = sin_f32(-0.25f);
    f32_4x W4_reals = F32_4x(1, Wm4.real, 1, Wm4.real);
    f32_4x W4_imags = F32_4x(0, Wm4.imag, 0, Wm4.imag);
    
    Complex32 Wm8_1;
    Wm8_1.real = cos_f32(-0.125f);
    Wm8_1.imag = sin_f32(-0.125f);
    Complex32 Wm8_2 = Wm8_1 * Wm8_1;
    Complex32 Wm8_3 = Wm8_2 * Wm8_1;
    f32_4x W8_reals = F32_4x(1, Wm8_1.real, Wm8_2.real, Wm8_3.real);
    f32_4x W8_imags = F32_4x(0, Wm8_1.imag, Wm8_2.imag, Wm8_3.imag);
    unused(W8_reals);
    unused(W8_imags);
    for (u32 k = 0; k < dftCount; k += 8)
    {
        f32 *kGrab = (f32 *)(signal + k);
        f32 *kPut  = (f32 *)(signal + k);
        
        f32_4x EO2_0 = F32_4x(kGrab);
        kGrab += 4;
        f32_4x EO2_1 = F32_4x(kGrab);
        kGrab += 4;
        f32_4x EO2_2 = F32_4x(kGrab);
        kGrab += 4;
        f32_4x EO2_3 = F32_4x(kGrab);
        
        f32_4x E2_02;
        f32_4x O2_02;
        f32_4x E2_13;
        f32_4x O2_13;
        E2_02.m = _mm_shuffle_ps(EO2_0.m, EO2_2.m, MULTILANE_SHUFFLE_MASK(0, 1, 0, 1));
        O2_02.m = _mm_shuffle_ps(EO2_0.m, EO2_2.m, MULTILANE_SHUFFLE_MASK(2, 3, 2, 3));
        E2_13.m = _mm_shuffle_ps(EO2_1.m, EO2_3.m, MULTILANE_SHUFFLE_MASK(0, 1, 0, 1));
        O2_13.m = _mm_shuffle_ps(EO2_1.m, EO2_3.m, MULTILANE_SHUFFLE_MASK(2, 3, 2, 3));
        
        f32_4x E4_02 = E2_02 + O2_02;
        f32_4x ac_4x = E2_13 + O2_13;
        f32_4x E4_13 = E2_02 - O2_02;
        f32_4x bd_4x = E2_13 - O2_13;
        
        f32_4x X_reals;
        X_reals.m = _mm_shuffle_ps(ac_4x.m, bd_4x.m, MULTILANE_SHUFFLE_MASK(0, 2, 0, 2));
        X_reals.m = _mm_shuffle_epi32(X_reals.m, MULTILANE_SHUFFLE_MASK(0, 2, 1, 3));
        f32_4x X_imags;
        X_imags.m = _mm_shuffle_ps(ac_4x.m, bd_4x.m, MULTILANE_SHUFFLE_MASK(1, 3, 1, 3));
        X_imags.m = _mm_shuffle_epi32(X_imags.m, MULTILANE_SHUFFLE_MASK(0, 2, 1, 3));
        
        f32_4x mulX0 = W4_reals * X_reals;
        f32_4x mulX1 = W4_imags * X_imags;
        f32_4x mulX2 = W4_reals * X_imags;
        f32_4x mulX3 = W4_imags * X_reals;
        
        f32_4x O4_reals = mulX0 - mulX1;
        f32_4x O4_imags = mulX2 + mulX3;
        
        f32_4x E4_0_4x;
        f32_4x E4_1_4x;
        f32_4x O4_0_4x;
        f32_4x O4_1_4x;
        E4_0_4x.m = _mm_shuffle_ps(E4_02.m, E4_13.m, MULTILANE_SHUFFLE_MASK(0, 1, 0, 1));
        E4_1_4x.m = _mm_shuffle_ps(E4_02.m, E4_13.m, MULTILANE_SHUFFLE_MASK(2, 3, 2, 3));
        O4_0_4x.m = _mm_shuffle_ps(O4_reals.m, O4_imags.m, MULTILANE_SHUFFLE_MASK(0, 1, 0, 1));
        O4_0_4x.m = _mm_shuffle_epi32(O4_0_4x.m, MULTILANE_SHUFFLE_MASK(0, 2, 1, 3));
        O4_1_4x.m = _mm_shuffle_ps(O4_reals.m, O4_imags.m, MULTILANE_SHUFFLE_MASK(2, 3, 2, 3));
        O4_1_4x.m = _mm_shuffle_epi32(O4_1_4x.m, MULTILANE_SHUFFLE_MASK(0, 2, 1, 3));
        
        f32_4x ef_4x = E4_1_4x + O4_1_4x;
        f32_4x gh_4x = E4_1_4x - O4_1_4x;
        
        f32_4x Y_reals;
        f32_4x Y_imags;
        Y_reals.m = _mm_shuffle_ps(ef_4x.m, gh_4x.m, MULTILANE_SHUFFLE_MASK(0, 2, 0, 2));
        Y_imags.m = _mm_shuffle_ps(ef_4x.m, gh_4x.m, MULTILANE_SHUFFLE_MASK(1, 3, 1, 3));
        
        f32_4x mulY0 = W8_reals * Y_reals;
        f32_4x mulY1 = W8_imags * Y_imags;
        f32_4x mulY2 = W8_reals * Y_imags;
        f32_4x mulY3 = W8_imags * Y_reals;
        
        f32_4x O8_reals = mulY0 - mulY1;
        f32_4x O8_imags = mulY2 + mulY3;
        
        f32_4x E8_0_4x = E4_0_4x + O4_0_4x;
        f32_4x E8_1_4x = E4_0_4x - O4_0_4x;
        f32_4x O8_0_4x;
        f32_4x O8_1_4x;
        O8_0_4x.m = _mm_shuffle_ps(O8_reals.m, O8_imags.m, MULTILANE_SHUFFLE_MASK(0, 1, 0, 1));
        O8_0_4x.m = _mm_shuffle_epi32(O8_0_4x.m, MULTILANE_SHUFFLE_MASK(0, 2, 1, 3));
        O8_1_4x.m = _mm_shuffle_ps(O8_reals.m, O8_imags.m, MULTILANE_SHUFFLE_MASK(2, 3, 2, 3));
        O8_1_4x.m = _mm_shuffle_epi32(O8_1_4x.m, MULTILANE_SHUFFLE_MASK(0, 2, 1, 3));
        
        _mm_store_ps(kPut, (E8_0_4x + O8_0_4x).m);
        kPut += 4;
        _mm_store_ps(kPut, (E8_1_4x + O8_1_4x).m);
        kPut += 4;
        _mm_store_ps(kPut, (E8_0_4x - O8_0_4x).m);
        kPut += 4;
        _mm_store_ps(kPut, (E8_1_4x - O8_1_4x).m);
        kPut += 4;
    }
    u32 halfM = 8;
    u32 m = 16;
    
    while (m <= dftCount)
    {
        f32 oneOverM = 1.0f / (f32)m;
        Complex32 Wm;
        Wm.real = cos_f32(-oneOverM);
        Wm.imag = sin_f32(-oneOverM);
        Complex32 Wm2 = Wm * Wm;
        Complex32 Wm3 = Wm2 * Wm;
        Complex32 Wm4 = Wm3 * Wm;
        
#if 1
        for (u32 k = 0; k < dftCount; k += m)
        {
            Complex32 w;
            w.real = 1.0f;
            w.imag = 0.0f;
            Complex32 *src0 = signal + k;
            Complex32 *src1 = signal + k + halfM;
            
            Complex32 w2 = w * Wm;
            Complex32 w3 = w * Wm2;
            Complex32 w4 = w * Wm3;
            
            for (u32 j = 0; j < halfM; j += 4)
            {
                f32_4x w_real_4x = F32_4x(w.real, w2.real, w3.real, w4.real);
                f32_4x w_imag_4x = F32_4x(w.imag, w2.imag, w3.imag, w4.imag);
                
                f32 *EGrab = (f32 *)(src0 + j);
                f32 *OGrab = (f32 *)(src1 + j);
                f32 *EPut  = (f32 *)(src0 + j);
                f32 *OPut  = (f32 *)(src1 + j);
                
                f32_4x a01 = F32_4x(OGrab);
                OGrab += 4;
                f32_4x a23 = F32_4x(OGrab);
                
                f32_4x a_real;
                f32_4x a_imag;
                a_real.m = _mm_shuffle_ps(a01.m, a23.m, MULTILANE_SHUFFLE_MASK(0, 2, 0, 2));
                a_imag.m = _mm_shuffle_ps(a01.m, a23.m, MULTILANE_SHUFFLE_MASK(1, 3, 1, 3));
                
                f32_4x mulX0 = w_real_4x * a_real;
                f32_4x mulX1 = w_imag_4x * a_imag;
                f32_4x mulX2 = w_real_4x * a_imag;
                f32_4x mulX3 = w_imag_4x * a_real;
                
                f32_4x O_real = mulX0 - mulX1;
                f32_4x O_imag = mulX2 + mulX3;
                
                w = w * Wm4;
                w2 = w2 * Wm4;
                w3 = w3 * Wm4;
                w4 = w4 * Wm4;
                
                f32_4x E01 = F32_4x(EGrab);
                EGrab += 4;
                f32_4x E23 = F32_4x(EGrab);
                
                f32_4x O01;
                f32_4x O23;
                O01.m = _mm_shuffle_ps(O_real.m, O_imag.m, MULTILANE_SHUFFLE_MASK(0, 1, 0, 1));
                O01.m = _mm_shuffle_epi32(O01.m, MULTILANE_SHUFFLE_MASK(0, 2, 1, 3));
                O23.m = _mm_shuffle_ps(O_real.m, O_imag.m, MULTILANE_SHUFFLE_MASK(2, 3, 2, 3));
                O23.m = _mm_shuffle_epi32(O23.m, MULTILANE_SHUFFLE_MASK(0, 2, 1, 3));
                
                f32_4x add01 = E01 + O01;
                f32_4x add23 = E23 + O23;
                f32_4x sub01 = E01 - O01;
                f32_4x sub23 = E23 - O23;
                
                _mm_store_ps(EPut, add01.m);
                EPut += 4;
                _mm_store_ps(OPut, sub01.m);
                OPut += 4;
                _mm_store_ps(EPut, add23.m);
                _mm_store_ps(OPut, sub23.m);
            }
        }
#else
        for (u32 k = 0; k < dftCount; k += m)
        {
            Complex32 w;
            w.real = 1.0f;
            w.imag = 0.0f;
            Complex32 *src0 = signal + k;
            Complex32 *src1 = signal + k + halfM;
            for (u32 j = 0; j < halfM; ++j)
            {
                Complex32 E = *src0;
                Complex32 O = w * *src1;
                *src0++ = E + O;
                *src1++ = E - O;
                w *= Wm;
            }
        }
#endif
        
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
        fftState->reconstructSignal = allocate_array(v2, fftState->origSignalCount);
        
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
            
            v2 *point2 = fftState->reconstructSignal + idx;
            point2->x = mod;
            point2->y = 0.0f;
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
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    draw_lines(image, fftState->origSignalCount, fftState->origSignal, V2(10.0f, 0.25f * size.y), V2(0.45f * size.x, -0.2f * size.y), V4(1, 1, 1, 1));
    
    TempMemory tempMem = temporary_memory(&fftState->drawArena);
    
    //dft(fftState->origSignalCount, fftState->origSignal, fftState->dftSignal);
    //fft_recurse(fftState->origSignalCount, fftState->origSignal, fftState->dftSignal);
    //fft_iter(fftState->origSignalCount, fftState->origSignal, fftState->dftSignal);
    for (u32 idx = 0; idx < fftState->origSignalCount; ++idx)
    {
        fftState->dftSignal[idx] = {fftState->origSignal[idx].y, 0};
    }
    //fft_iter_inplace(fftState->origSignalCount, fftState->dftSignal);
    fft_iter_inplace2(fftState->origSignalCount, fftState->dftSignal);
    
    Complex32 *idftBuf = arena_allocate_array(&fftState->drawArena, Complex32, fftState->origSignalCount);
    //idft(fftState->origSignalCount, fftState->dftSignal, fftState->reconstructSignal);
    //ifft_recurse(fftState->origSignalCount, fftState->dftSignal, idftBuf);
    //ifft_iter(fftState->origSignalCount, fftState->dftSignal, idftBuf);
    for (u32 idx = 0; idx < fftState->origSignalCount; ++idx)
    {
        idftBuf[idx] = fftState->dftSignal[idx];
    }
    ifft_iter_inplace(fftState->origSignalCount, idftBuf);
    
    for (u32 idx = 0; idx < fftState->origSignalCount; ++idx)
    {
        fftState->reconstructSignal[idx].y = idftBuf[idx].real;
    }
    
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
    
    draw_lines(image, fftState->origSignalCount, dftSig, V3(10.0f, size.y - 10.0f, 0.75f * size.y),
               V3(0.5f * size.x - 20.0f, -0.5f * size.y + 20.0f, -0.25f * size.y / F32_PI + 20.0f), V4(1, 1, 1, 1), V4(1, 0, 0, 1));
    
    draw_lines(image, fftState->origSignalCount, fftState->reconstructSignal, V2(0.5f * size.x + 10.0f, 0.75f * size.y), V2(0.45f * size.x, -0.2f * size.y), V4(1, 1, 1, 1));
    
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

