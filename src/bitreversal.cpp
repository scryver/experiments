#include "../libberdip/platform.h"

struct ReversalBits
{
    u32 indices[4];
};

internal ReversalBits
reversal_bits(u32 startIndex, u32 bitCount)
{
    i_expect((startIndex & 0x3) == 0);
    
    u32 half = 1 << (bitCount - 1);
    ReversalBits result = {};
    if (startIndex == 0)
    {
        result.indices[0] = 0;
        result.indices[1] = half;
        result.indices[2] = 1 << (bitCount - 2);
        result.indices[3] = (1 << (bitCount - 2)) ^ half;
    }
    else
    {
        u8 addOne[32]; // NOTE(michiel): Max 32bit
        addOne[0] = 0;
        u32 reversals = 1;
        u32 index = startIndex >> 1;
        while (index > 1)
        {
            if (index & 0x1)
            {
                addOne[reversals] = 1;
            }
            else
            {
                addOne[reversals] = 0;
            }
            ++reversals;
            index >>= 1;
        }
        
        u32 value = half;
        for (u32 reverse = 0; reverse < reversals; ++reverse)
        {
            value >>= 1;
            if (addOne[reversals - reverse - 1])
            {
                value = value ^ half;
            }
        }
        
        result.indices[0] = value;
        result.indices[1] = value ^ half;
        
        addOne[0] = 0;
        reversals = 1;
        index = (startIndex + 2) >> 1;
        while (index > 1)
        {
            if (index & 0x1)
            {
                addOne[reversals] = 1;
            }
            else
            {
                addOne[reversals] = 0;
            }
            ++reversals;
            index >>= 1;
        }
        
        value = half;
        for (u32 reverse = 0; reverse < reversals; ++reverse)
        {
            value >>= 1;
            if (addOne[reversals - reverse - 1])
            {
                value = value ^ half;
            }
        }
        
        result.indices[2] = value;
        result.indices[3] = value ^ half;
        
    }
    return result;
}

int main(int argc, char **argv)
{
    u32 bitCount = 4;
    ReversalBits bits = reversal_bits(0, bitCount);
    fprintf(stdout, " 0: %2u,  1: %2u,  2: %2u,  3: %2u\n",
            bits.indices[0], bits.indices[1], bits.indices[2], bits.indices[3]);
    bits = reversal_bits(4, bitCount);
    fprintf(stdout, " 4: %2u,  5: %2u,  6: %2u,  7: %2u\n",
            bits.indices[0], bits.indices[1], bits.indices[2], bits.indices[3]);
    bits = reversal_bits(8, bitCount);
    fprintf(stdout, " 8: %2u,  9: %2u, 10: %2u, 11: %2u\n",
            bits.indices[0], bits.indices[1], bits.indices[2], bits.indices[3]);
    bits = reversal_bits(12, bitCount);
    fprintf(stdout, "12: %2u, 13: %2u, 14: %2u, 15: %u\n",
            bits.indices[0], bits.indices[1], bits.indices[2], bits.indices[3]);
    
    return 0;
}