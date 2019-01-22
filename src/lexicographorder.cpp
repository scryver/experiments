#include <stdint.h>
#include <stdio.h>

#include "../libberdip/platform.h"
//#include "interface.h"

int main(int argc, char **argv)
{
    u32 testArray[] = {0, 1, 2, 3};
    
    while (true)
    {
        fprintf(stdout, "Perms: ");
        for (u32 i = 0; i < array_count(testArray); ++i)
        {
            fprintf(stdout, "%u", testArray[i]);
            if (i < array_count(testArray) - 1)
            {
                fprintf(stdout, ", ");
            }
        }
        fprintf(stdout, "\n");
        
        u32 largestX = 0;
        u32 largestXIndex = 0xFFFFFFFF;
        for (u32 i = 0; i < array_count(testArray) - 1; ++i)
        {
            if (testArray[i] < testArray[i + 1])
            {
                largestX = testArray[i];
                largestXIndex = i;
            }
        }
        
        if (largestXIndex == 0xFFFFFFFF)
        {
            fprintf(stdout, "Done!\n");
            break;
        }
        
        u32 largestY = 0;
        u32 largestYIndex = 0xFFFFFFFF;
        for (u32 i = largestXIndex; i < array_count(testArray); ++i)
        {
            if (largestX < testArray[i])
            {
                largestY = testArray[i];
                largestYIndex = i;
            }
        }
        i_expect(largestYIndex != 0xFFFFFFFF);
        testArray[largestXIndex] = largestY;
        testArray[largestYIndex] = largestX;
        
        u32 minRev = largestXIndex + 1;
        u32 maxRev = minRev + (array_count(testArray) - minRev) / 2;

#if 0        
        fprintf(stdout, "Pre  : ");
        for (u32 i = 0; i < array_count(testArray); ++i)
        {
            fprintf(stdout, "%u", testArray[i]);
            if (i < array_count(testArray) - 1)
            {
                fprintf(stdout, ", ");
            }
        }
        fprintf(stdout, "| min: %u, max: %u\n", minRev, maxRev);
        #endif

        for (u32 rev = minRev; rev < maxRev; ++rev)
        {
            u32 offset = array_count(testArray) - 1;
            offset -= (rev - minRev);
            u32 temp = testArray[rev];
            testArray[rev] = testArray[offset];
            testArray[offset] = temp;
        }
    }
}