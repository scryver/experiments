#include <time.h>

#ifndef WITH_FFTW
#define WITH_FFTW  1
#endif

#include "../libberdip/platform.h"
#include "../libberdip/multilane.h"
#include "../libberdip/complex.h"

#include "../libberdip/fft.cpp"
#include "fft_impl.cpp"

#if WITH_FFTW
#include <fftw3.h>
#endif

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
#define DO_FFT_RECURSE 0

struct Stats
{
    String name;
    f64 secondsTotal;
    f32 secondsMax;
    f32 secondsMin;
};

internal Stats
create_stats(String name)
{
    Stats result;
    result.name = name;
    result.secondsTotal = 0.0;
    result.secondsMax = -F32_MAX;
    result.secondsMin = F32_MAX;
    return result;
}

internal void
accum_stats(Stats *stats, f32 secondsElapsed)
{
    stats->secondsTotal += secondsElapsed;
    if (stats->secondsMax < secondsElapsed)
    {
        stats->secondsMax = secondsElapsed;
    }
    if (stats->secondsMin > secondsElapsed)
    {
        stats->secondsMin = secondsElapsed;
    }
}

struct AllStats
{
    u32 statCount;
    Stats stats[32];
};

internal Stats *
create_stat(AllStats *allStats, String name)
{
    i_expect(allStats->statCount < array_count(allStats->stats));
    allStats->stats[allStats->statCount++] = create_stats(name);
    return allStats->stats + allStats->statCount - 1;
}

internal void
print_timings(AllStats *allStats, f32 oneOverTests)
{
    for (u32 statIndex = 0; statIndex < allStats->statCount; ++statIndex)
    {
        Stats *stat = allStats->stats + statIndex;
        fprintf(stdout, "  %.*s: %f sec, min: %f, max: %f\n", STR_FMT(stat->name), stat->secondsTotal * oneOverTests,
                stat->secondsMin, stat->secondsMax);
    }
}

int main(int argc, char **argv)
{
    fft_iter_inplace_address(8);
    
    u32 tests = 32;
    u32 testCount = 64*8192;
    
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
    
    AllStats allStats = {};
    
#if DO_DFT
    Stats *dftTime = create_stat(&allStats, static_string("DFT                          "));
    for (u32 testIndex = 0; testIndex < tests; ++testIndex)
    {
        start = get_wall_clock();
        dft(testCount, signal, fftSignal);
        end = get_wall_clock();
        accum_stats(dftTime, get_seconds_elapsed(start, end));
    }
#endif
    
#if DO_FFT_RECURSE
    Stats *fftRecTime = create_stat(&allStats, static_string("FFT Recursive                "));
    for (u32 testIndex = 0; testIndex < tests; ++testIndex)
    {
        fft_recurse(testCount, signal, fftSignal);
        start = get_wall_clock();
        fft_recurse(testCount, signal, fftSignal);
        end = get_wall_clock();
        accum_stats(fftRecTime, get_seconds_elapsed(start, end));
    }
#endif
    
    Stats *fftIterTime = create_stat(&allStats, static_string("FFT Iterate                  "));
    for (u32 testIndex = 0; testIndex < tests; ++testIndex)
    {
        fft_iter(testCount, signal, fftSignal);
        start = get_wall_clock();
        fft_iter(testCount, signal, fftSignal);
        end = get_wall_clock();
        accum_stats(fftIterTime, get_seconds_elapsed(start, end));
    }
    
    Stats *fftIterInPlaceCopyTime = create_stat(&allStats, static_string("FFT Iterate inplace with copy"));
    for (u32 testIndex = 0; testIndex < tests; ++testIndex)
    {
        start = get_wall_clock();
        for (u32 x = 0; x < testCount; ++x)
        {
            fftSignal[x] = {signal[x], 0};
        }
        fft_iter_inplace(testCount, fftSignal);
        end = get_wall_clock();
        accum_stats(fftIterInPlaceCopyTime, get_seconds_elapsed(start, end));
    }
    
    Stats *fftIterInPlaceTime = create_stat(&allStats, static_string("FFT Iterate inplace no copy  "));
    for (u32 x = 0; x < testCount; ++x)
    {
        fftSignal[x] = {signal[x], 0};
    }
    for (u32 testIndex = 0; testIndex < tests; ++testIndex)
    {
        start = get_wall_clock();
        fft_iter_inplace(testCount, fftSignal);
        end = get_wall_clock();
        accum_stats(fftIterInPlaceTime, get_seconds_elapsed(start, end));
    }
    
    Stats *fftIterInPlaceCopy2Time = create_stat(&allStats, static_string("FFT Iterate inplace w/copy2  "));
    for (u32 testIndex = 0; testIndex < tests; ++testIndex)
    {
        start = get_wall_clock();
        for (u32 x = 0; x < testCount; ++x)
        {
            fftSignal[x] = {signal[x], 0};
        }
        fft_iter_inplace2(testCount, fftSignal);
        end = get_wall_clock();
        accum_stats(fftIterInPlaceCopy2Time, get_seconds_elapsed(start, end));
    }
    
    Stats *fftIterInPlaceCopy3Time = create_stat(&allStats, static_string("FFT Iterate in libberdip     "));
    for (u32 testIndex = 0; testIndex < tests; ++testIndex)
    {
        start = get_wall_clock();
        for (u32 x = 0; x < testCount; ++x)
        {
            fftSignal[x] = {signal[x], 0};
        }
        fft(testCount, fftSignal);
        end = get_wall_clock();
        accum_stats(fftIterInPlaceCopy3Time, get_seconds_elapsed(start, end));
    }
    
#if WITH_FFTW
    fftwf_complex *in;
    fftwf_plan p;
    in  = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * testCount);
    p   = fftwf_plan_dft_1d(testCount, in, in, FFTW_FORWARD, FFTW_ESTIMATE);
    
    Stats *fftwTime = create_stat(&allStats, static_string("FFTW library Estimate        "));
    for (u32 testIndex = 0; testIndex < tests; ++testIndex)
    {
        start = get_wall_clock();
        fftwf_execute(p);
        end = get_wall_clock();
        accum_stats(fftwTime, get_seconds_elapsed(start, end));
    }
    fftwf_destroy_plan(p);
    
    p   = fftwf_plan_dft_1d(testCount, in, in, FFTW_FORWARD, FFTW_MEASURE);
    Stats *fftwOptTime = create_stat(&allStats, static_string("FFTW library Measure         "));
    for (u32 testIndex = 0; testIndex < tests; ++testIndex)
    {
        start = get_wall_clock();
        fftwf_execute(p);
        end = get_wall_clock();
        accum_stats(fftwOptTime, get_seconds_elapsed(start, end));
    }
    fftwf_destroy_plan(p);
    
    fftwf_free(in);
#endif
    
    f32 oneOverTests = 1.0f / (f32)tests;
    fprintf(stdout, "Timings for %u tests of size %u:\n", tests, testCount);
    print_timings(&allStats, oneOverTests);
    
#if 0    
#if DO_DFT
    fprintf(stdout, "  DFT                          : %f sec\n", dftTime * oneOverTests);
#endif
#if DO_FFT_RECURSE
    fprintf(stdout, "  FFT Recursive                : %f sec\n", fftRecTime * oneOverTests);
#endif
    fprintf(stdout, "  FFT Iterate                  : %f sec\n", fftIterTime * oneOverTests);
    fprintf(stdout, "  FFT Iterate inplace with copy: %f sec\n", fftIterInPlaceCopyTime * oneOverTests);
    fprintf(stdout, "  FFT Iterate inplace no copy  : %f sec\n", fftIterInPlaceTime * oneOverTests);
    fprintf(stdout, "  FFT Iterate inplace w/copy2  : %f sec\n", fftIterInPlaceCopy2Time * oneOverTests);
    fprintf(stdout, "  FFT Iterate in libberdip     : %f sec\n", fftIterInPlaceCopy3Time * oneOverTests);
    fprintf(stdout, "  FFTW library                 : %f sec\n", fftwTime * oneOverTests);
#endif
    
}
