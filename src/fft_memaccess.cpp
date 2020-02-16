#include <time.h>

#include "../libberdip/platform.h"
#include "../libberdip/multilane.h"

#include "complex.h"
#include "fft_impl.cpp"

internal void
fft_iter_inplace_address(u32 dftCount)
{
    i_expect(is_pow2(dftCount));
    i_expect(dftCount > 2);
    
    fprintf(stdout, "Preprocess swap:\n");
    BitScanResult highBit = find_most_significant_set_bit(dftCount);
    for (u32 index = 0; index < dftCount; ++index)
    {
        u32 reversedIndex = reverse_bits(index, highBit.index);
        if (reversedIndex > index)
        {
            fprintf(stdout, "%3u <=> %3u\n", index, reversedIndex);
        }
    }
    
    fprintf(stdout, "\nFFT methode:\n\n");
    u32 halfM = 1;
    u32 m = 2;
    while (m <= dftCount)
    {
        fprintf(stdout, "M = %u\n", m);
        for (u32 k = 0; k < dftCount; k += m)
        {
            for (u32 j = 0; j < halfM; ++j)
            {
                fprintf(stdout, "E = S[%2u], O = S[%2u] => S[%2u] = S[%2u] + w*S[%2u], S[%2u] = S[%2u] - w*S[%2u]\n",
                        k + j, k + j + halfM, k + j, k + j, k + j + halfM, k + j + halfM, k + j, k + j + halfM);
            }
        }
        halfM = m;
        m <<= 1;
        fprintf(stdout, "\n");
    }
}

internal struct timespec
get_wall_clock(void)
{
    struct timespec clock;
    clock_gettime(CLOCK_MONOTONIC, &clock);
    return clock;
}

internal f32
get_seconds_elapsed(struct timespec start, struct timespec end)
{
    return ((f32)(end.tv_sec - start.tv_sec)
            + ((f32)(end.tv_nsec - start.tv_nsec) * 1e-9f));
}

#define DO_DFT         0
#define DO_FFT_RECURSE 1

int main(int argc, char **argv)
{
    fft_iter_inplace_address(8);
    
    u32 testCount = 16*8192;
    
    f32 *signal = allocate_array(f32, testCount);
    Complex32 *fftSignal = allocate_array(Complex32, testCount);
    
    for (u32 x = 0; x < testCount; ++x)
    {
        if ((x & 0xFF) > 0x80)
        {
            signal[x] = -1.0f;
        }
        else
        {
            signal[x] = 1.0f;
        }
    }
    
    struct timespec start;
    struct timespec end;
    
#if DO_DFT
    start = get_wall_clock();
    dft(testCount, signal, fftSignal);
    end = get_wall_clock();
    f32 dftTime = get_seconds_elapsed(start, end);
#endif
    
#if DO_FFT_RECURSE
    fft_recurse(testCount, signal, fftSignal);
    start = get_wall_clock();
    fft_recurse(testCount, signal, fftSignal);
    end = get_wall_clock();
    f32 fftRecTime = get_seconds_elapsed(start, end);
#endif
    
    fft_iter(testCount, signal, fftSignal);
    start = get_wall_clock();
    fft_iter(testCount, signal, fftSignal);
    end = get_wall_clock();
    f32 fftIterTime = get_seconds_elapsed(start, end);
    
    start = get_wall_clock();
    for (u32 x = 0; x < testCount; ++x)
    {
        fftSignal[x] = {signal[x], 0};
    }
    fft_iter_inplace(testCount, fftSignal);
    end = get_wall_clock();
    f32 fftIterInPlaceCopyTime = get_seconds_elapsed(start, end);
    
    for (u32 x = 0; x < testCount; ++x)
    {
        fftSignal[x] = {signal[x], 0};
    }
    start = get_wall_clock();
    fft_iter_inplace(testCount, fftSignal);
    end = get_wall_clock();
    f32 fftIterInPlaceTime = get_seconds_elapsed(start, end);
    
    start = get_wall_clock();
    for (u32 x = 0; x < testCount; ++x)
    {
        fftSignal[x] = {signal[x], 0};
    }
    fft_iter_inplace2(testCount, fftSignal);
    end = get_wall_clock();
    f32 fftIterInPlaceCopy2Time = get_seconds_elapsed(start, end);
    
    start = get_wall_clock();
    for (u32 x = 0; x < testCount; ++x)
    {
        fftSignal[x] = {signal[x], 0};
    }
    fft_iter_inplace3(testCount, fftSignal);
    end = get_wall_clock();
    f32 fftIterInPlaceCopy3Time = get_seconds_elapsed(start, end);
    
    fprintf(stdout, "Timings:\n");
#if DO_DFT
    fprintf(stdout, "  DFT                          : %f sec\n", dftTime);
#endif
#if DO_FFT_RECURSE
    fprintf(stdout, "  FFT Recursive                : %f sec\n", fftRecTime);
#endif
    fprintf(stdout, "  FFT Iterate                  : %f sec\n", fftIterTime);
    fprintf(stdout, "  FFT Iterate inplace with copy: %f sec\n", fftIterInPlaceCopyTime);
    fprintf(stdout, "  FFT Iterate inplace no copy  : %f sec\n", fftIterInPlaceTime);
    fprintf(stdout, "  FFT Iterate inplace w/copy2  : %f sec\n", fftIterInPlaceCopy2Time);
    fprintf(stdout, "  FFT Iterate inplace w/copy3  : %f sec\n", fftIterInPlaceCopy3Time);
    
}
