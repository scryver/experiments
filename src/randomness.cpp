#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

DRAW_IMAGE(draw_image)
{
    if (!state->initialized)
    {
        RandomSeriesPCG randomize = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        
        for (u32 pixelIndex = 0; pixelIndex < (image->width * image->height); ++pixelIndex)
        {
            // u32 grayScale = random_choice(&randomize, 256) & 0xFF;
            u32 grayScale = slow_gaussian_choice(&randomize, 256, 8);
            //u32 grayScale = (u32)(255.0f * clamp01(random_gaussian(&randomize, 0.5f, 0.01f)));
            image->pixels[pixelIndex] =
                (0xFF << 24) | (grayScale << 16) | (grayScale << 8) | grayScale;
        }
        state->initialized = true;
    }
}
