#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "random.h"
#include "perlin.h"

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(PerlinNoiseOld) + sizeof(PerlinNoise) <= state->memorySize);
    
    if (!state->initialized)
    {
        RandomSeriesPCG randomize = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        
        PerlinNoiseOld *noise = (PerlinNoiseOld *)state->memory;
        PerlinNoise *noise2 = (PerlinNoise *)(noise + 1);
        
        init_perlin_noise(noise, &randomize);
        init_perlin_noise(noise2, &randomize);
        
        struct timespec start;
        struct timespec end;
        f32 time1 = 0.0f;
        f32 time2 = 0.0f;
        
                for (u32 y = 0; y < image->height; ++y)
        {
            for (u32 x = 0; x < image->width; ++x)
            {
                f32 n = 0.0f;
                f32 scale = 0.02f;
                
                if (x < image->width / 2)
                {
                    start = get_wall_clock();
                 n = perlin_noise(noise, V2(scale * x, scale * y));
                    end = get_wall_clock();
                    time1 += get_seconds_elapsed(start, end);
                }
                else
                {
                    start = get_wall_clock();
                    n = perlin_noise(noise2, V2(scale * x, scale * y));
                    end = get_wall_clock();
                    time2 += get_seconds_elapsed(start, end);
                    //n = random_bilateral(&randomize);
                }
                
                u32 grayScale = (u32)round(255.0f * (0.5f * n + 0.5f)) & 0xFF;
                
            image->pixels[y * image->width + x] = 
                    (0xFF << 24) | (grayScale << 16) | (grayScale << 8) | grayScale;
                    }
        }
        
        fprintf(stdout, "Timing: 1 = %7.4f | 2 = %7.4f\n", time1, time2);
        
        f32 min1_f = 1000.0f;
        f32 max1_f = -1000.0f;
        f32 min1_v2 = 1000.0f;
        f32 max1_v2 = -1000.0f;
        f32 min1_v3 = 1000.0f;
        f32 max1_v3 = -1000.0f;
        
        f32 min2_f = 1000.0f;
        f32 max2_f = -1000.0f;
        f32 min2_v2 = 1000.0f;
        f32 max2_v2 = -1000.0f;
        f32 min2_v3 = 1000.0f;
        f32 max2_v3 = -1000.0f;
        
        for (u32 loop = 0; loop < 5000000; ++loop)
        {
            f32 result = perlin_noise(noise, 100.0f * random_bilateral(&randomize));
            if (min1_f > result)
            {
                min1_f = result;
            }
            if (max1_f < result)
            {
                max1_f = result;
            }
            
            result = perlin_noise(noise, V2(100.0f * random_bilateral(&randomize),
                                            100.0f * random_bilateral(&randomize)));
            if (min1_v2 > result)
            {
                min1_v2 = result;
            }
            if (max1_v2 < result)
            {
                max1_v2 = result;
            }
            
            result = perlin_noise(noise, V3(100.0f * random_bilateral(&randomize),
                                            100.0f * random_bilateral(&randomize),
                                            100.0f * random_bilateral(&randomize)));
            if (min1_v3 > result)
            {
                min1_v3 = result;
            }
            if (max1_v3 < result)
            {
                max1_v3 = result;
            }
            
            result = perlin_noise(noise2, 100.0f * random_bilateral(&randomize));
            if (min2_f > result)
            {
                min2_f = result;
            }
            if (max2_f < result)
            {
                max2_f = result;
            }
            
            result = perlin_noise(noise2, V2(100.0f * random_bilateral(&randomize),
                                            100.0f * random_bilateral(&randomize)));
            if (min2_v2 > result)
            {
                min2_v2 = result;
            }
            if (max2_v2 < result)
            {
                max2_v2 = result;
            }
            
            result = perlin_noise(noise2, V3(100.0f * random_bilateral(&randomize),
                                            100.0f * random_bilateral(&randomize),
                                            100.0f * random_bilateral(&randomize)));
            if (min2_v3 > result)
            {
                min2_v3 = result;
            }
            if (max2_v3 < result)
            {
                max2_v3 = result;
            }
        }
        
        fprintf(stdout, "Results:\n");
        fprintf(stdout, "        Orig:    | New:\n");
        fprintf(stdout, "f  min: %8.3f | %8.3f\n", min1_f, min2_f);
        fprintf(stdout, "   max: %8.3f | %8.3f\n", max1_f, max2_f);
        fprintf(stdout, "v2 min: %8.3f | %8.3f\n", min1_v2, min2_v2);
        fprintf(stdout, "   max: %8.3f | %8.3f\n", max1_v2, max2_v2);
        fprintf(stdout, "v3 min: %8.3f | %8.3f\n", min1_v3, min2_v3);
        fprintf(stdout, "   max: %8.3f | %8.3f\n", max1_v3, max2_v3);
        
        state->initialized = true;
    }
}
