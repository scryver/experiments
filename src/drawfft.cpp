#include "../libberdip/platform.h"
#include "../libberdip/std_memory.h"

global MemoryAPI gMemoryApi_;
global MemoryAPI *gMemoryApi = &gMemoryApi_;
global MemoryArena gMemoryArena_;
global MemoryArena *gMemoryArena = &gMemoryArena_;

global FileAPI gFileApi_;
global FileAPI *gFileApi = &gFileApi_;

#include "../libberdip/memory.cpp"
#include "../libberdip/std_memory.cpp"
#include "../libberdip/std_file.c"

internal String
print_fft_recurse(MemoryArena *memory, ApiFile *file, u32 dftCount, u32 signalIndex = 0, u32 dftIndex = 0, u32 step = 1)
{
    String result = {};
    result.data = arena_allocate_array(memory, u8, 10, default_memory_alloc());
    result = string_fmt(9, result.data, "d%u_%u", dftCount, signalIndex);

    if (dftCount == 1)
    {
        write_fmt_to_file(file, "  %.*s [shape=record, label=\"{ %u | { in: %u | out: %u } }\"]\n",
                          STR_FMT(result), dftCount, signalIndex, dftIndex);
    }
    else
    {
        String labelLeft  = print_fft_recurse(memory, file, dftCount / 2, signalIndex, dftIndex, step * 2);
        String labelRight = print_fft_recurse(memory, file, dftCount / 2, signalIndex + step, dftIndex + dftCount / 2, step * 2);

        write_fmt_to_file(file, "  %.*s [shape=record, label=\"{ %u | { in: %u | out: %u } }\"]\n",
                          STR_FMT(result), dftCount, signalIndex, dftIndex);
        write_fmt_to_file(file, "\n  %.*s -> %.*s\n  %.*s -> %.*s\n\n",
                          STR_FMT(result), STR_FMT(labelLeft), STR_FMT(result), STR_FMT(labelRight));
    }
    return result;
}

internal String
print_fft_recurse2(MemoryArena *memory, ApiFile *file, u32 dftCount, u32 signalIndex = 0, u32 dftIndex = 0, u32 step = 1)
{
    String result = {};
    result.data = arena_allocate_array(memory, u8, 10, default_memory_alloc());
    result = string_fmt(9, result.data, "d%u_%u", dftCount, signalIndex);

    if (dftCount == 16)
    {
        write_fmt_to_file(file, "  %.*s [shape=record, label=\"{ in: [", STR_FMT(result));
        BitScanResult highBit = find_most_significant_set_bit(dftCount * step);
        unused(highBit);
        for (u32 sampleIndex = signalIndex; sampleIndex < step * dftCount; sampleIndex += step)
        {
            //u32 reversedIndex = reverse_bits(sampleIndex, highBit.index);
            //write_fmt_to_file(file, "%u%s", reversedIndex, ((sampleIndex + step) < (step * dftCount)) ? ", " : "");
            write_fmt_to_file(file, "%u%s", sampleIndex, ((sampleIndex + step) < (step * dftCount)) ? ", " : "");
        }
        write_fmt_to_file(file, "] | out: [");
        for (u32 index = 0; index < dftCount; ++index)
        {
            write_fmt_to_file(file, "%u%s", dftIndex + index, ((index + 1) < dftCount) ? ", " : "");
        }
        write_fmt_to_file(file, "] }\"]\n");
    }
    else
    {
        String labelLeft  = print_fft_recurse2(memory, file, dftCount / 2, signalIndex, dftIndex, step * 2);
        String labelRight = print_fft_recurse2(memory, file, dftCount / 2, signalIndex + step, dftIndex + dftCount / 2, step * 2);

        write_fmt_to_file(file, "  %.*s [shape=record, label=\"{ %u | { in: %u | out: %u } }\"]\n",
                          STR_FMT(result), dftCount, signalIndex, dftIndex);
        write_fmt_to_file(file, "\n  %.*s -> %.*s\n  %.*s -> %.*s\n\n",
                          STR_FMT(result), STR_FMT(labelLeft), STR_FMT(result), STR_FMT(labelRight));
    }
    return result;
}

internal void
print_iter2(MemoryArena *memory, ApiFile *file, u32 dftCount)
{
    u32 baseDftCount = 16;
    u32 dftStep = dftCount / baseDftCount;
    BitScanResult highBit = find_most_significant_set_bit(dftStep);

    for (u32 blockIndex = 0; blockIndex < dftStep; ++blockIndex)
    {
        u32 reversedIndex = reverse_bits32(blockIndex, highBit.index);
        u32 sourceIndex = reversedIndex;
        u32 destIndex = blockIndex * baseDftCount;
        write_fmt_to_file(file, "block %u:\n", blockIndex);
        for (u32 dftIndex = 0; dftIndex < baseDftCount; ++dftIndex)
        {
            write_fmt_to_file(file, "  dft %u: [", destIndex + dftIndex);
            for (u32 sampleIndex = 0; sampleIndex < dftCount; sampleIndex += dftStep)
            {
                write_fmt_to_file(file, "%u%s", sourceIndex + sampleIndex, (sampleIndex + dftStep) < dftCount ? ", " : "");
            }
            write_fmt_to_file(file, "]\n");
        }
        write_fmt_to_file(file, "\n");
    }
}

s32 main(s32 argc, char **argv)
{
    std_memory_api(gMemoryApi);

    MemoryArena stringArena = {};
    ApiFile printFile = open_file(static_string("fft_recurse.dot"), FileOpen_Write);
    write_fmt_to_file(&printFile, "digraph FFTRecurse {\n");
    print_fft_recurse(&stringArena, &printFile, 16);
    write_fmt_to_file(&printFile, "}\n");
    close_file(&printFile);

    printFile = open_file(static_string("fft_recurse2.dot"), FileOpen_Write);
    write_fmt_to_file(&printFile, "digraph FFTRecurse {\n");
    print_fft_recurse2(&stringArena, &printFile, 128);
    write_fmt_to_file(&printFile, "}\n");
    close_file(&printFile);

    printFile = open_file(static_string("fft_iter2.dot"), FileOpen_Write);
    //write_fmt_to_file(&printFile, "digraph FFTRecurse {\n");
    print_iter2(&stringArena, &printFile, 1024);
    //write_fmt_to_file(&printFile, "}\n");
    close_file(&printFile);

    return 0;
}

/*
digraph Test {
    d1_0    [shape=record, label="{ 1 | { in: 0 | out: 0 } }"]
    d1_4    [shape=record, label="{ 1 | { in: 4 | out: 1 } }"]
    d2_0    [shape=record, label="{ 2 | { in: 0 | out: 0 } }"]

    d2_0 -> d1_0
    d2_0 -> d1_4

    d1_2    [shape=record, label="{ 1 | { in: 2 | out: 2 } }"]
    d1_6    [shape=record, label="{ 1 | { in: 6 | out: 3 } }"]
    d2_2    [shape=record, label="{ 2 | { in: 2 | out: 2 } }"]

    d2_2 -> d1_2
    d2_2 -> d1_6

    d4_0    [shape=record, label="{ 4 | { in: 0 | out: 0 } }"]

    d4_0 -> d2_0
    d4_0 -> d2_2

    d1_1    [shape=record, label="{ 1 | { in: 1 | out: 4 } }"]
    d1_5    [shape=record, label="{ 1 | { in: 5 | out: 5 } }"]
    d2_1    [shape=record, label="{ 2 | { in: 1 | out: 4 } }"]

    d2_1 -> d1_1
    d2_1 -> d1_5

    d1_3    [shape=record, label="{ 1 | { in: 3 | out: 6 } }"]
    d1_7    [shape=record, label="{ 1 | { in: 7 | out: 7 } }"]
    d2_3    [shape=record, label="{ 2 | { in: 3 | out: 6 } }"]

    d2_3 -> d1_3
    d2_3 -> d1_7

    d4_1    [shape=record, label="{ 4 | { in: 1 | out: 4 } }"]

    d4_1 -> d2_1
    d4_1 -> d2_3

    d8_0    [shape=record, label="{ 8 | { in: 0 | out: 0 } }"]

    d8_0 -> d4_0
    d8_0 -> d4_1
}
*/