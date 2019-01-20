#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "drawing.cpp"

#include "../libberdip/std_file.c"

#include "matrix.h"
#include "aitraining.h"
#include "neurons.h"

#include "mnist.h"

struct HandwriteState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    TrainingSet train;
    TrainingSet test;
    TrainingSet validation;
    
    Neural brain;
    
    u32 count;
    
    u32 inputCount;
    f32 *inputs;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(HandwriteState) <= state->memorySize);
    
    HandwriteState *handwrite = (HandwriteState *)state->memory;
    if (!state->initialized)
    {
        //handwrite->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        handwrite->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        handwrite->inputCount = 784; // 28 * 28 pixels
        handwrite->inputs = allocate_array(f32, handwrite->inputCount);
        
        u32 hiddenCounts[] = {100};
        init_neural_network(&handwrite->brain, handwrite->inputCount, 
                            array_count(hiddenCounts), hiddenCounts, 10);
        randomize_weights(&handwrite->randomizer, &handwrite->brain);
        
        handwrite->train = parse_training("data/mnist-f32train");
        handwrite->test = parse_training("data/mnist-f32test");
        handwrite->validation = parse_training("data/mnist-f32validation");
        
        state->initialized = true;
    }
    
    u32 preCorrect = evaluate(&handwrite->brain, handwrite->validation);
    
    u32 epochs = 5;
    stochastic_gradient_descent(&handwrite->randomizer, &handwrite->brain,
                                epochs, 10, 0.5f, handwrite->train,
                                5.0f, true, handwrite->test);
    handwrite->count += epochs;
    
    u32 postCorrect = evaluate(&handwrite->brain, handwrite->validation);
    
    f32 preProcent = 100.0f * (f32)preCorrect / (f32)handwrite->validation.count;
    f32 postProcent = 100.0f * (f32)postCorrect / (f32)handwrite->validation.count;
    fprintf(stdout, "%4d: From %4u to %4u in %4d epoch%s. (%2.2f%% -> %2.2f%%, %+0.2f%%)\n",
            handwrite->count,
            preCorrect, postCorrect, epochs, epochs == 1 ? "" : "s",
            preProcent, postProcent, postProcent - preProcent);
    
    u32 resolution = 40;
    u32 rows = image->height / resolution;
    u32 columns = image->width / resolution;
    for (u32 y = 0; y < rows; ++y)
    {
        for (u32 x = 0; x < columns; ++x)
        {
            f32 iX = (f32)x / (f32)columns;
            f32 iY = (f32)y / (f32)rows;
            f32 gray = iX * iY;
            v4 colour = V4(gray, gray, gray, 1);
            fill_rectangle(image, x * resolution, y * resolution, resolution, resolution,
                           colour);
        }
    }
    //fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    ++handwrite->ticks;
}
