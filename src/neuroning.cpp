#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "random.h"
#include "drawing.cpp"

#include "matrix.h"
#include "neurons.h"

struct HandwriteState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    Neural brain;
    
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
        
        handwrite->inputCount = 2;
        handwrite->inputs = allocate_array(f32, handwrite->inputCount);
        
        u32 hiddenCounts[] = {10};
        init_neural_network(&handwrite->brain, handwrite->inputCount,
                            array_count(hiddenCounts), hiddenCounts, 1);
        randomize_weights(&handwrite->randomizer, &handwrite->brain);
        
        state->initialized = true;
    }
    
    for (u32 i = 0; i < 1; ++i)
    {
        Training train[] = 
        {
            { 2, NULL, 1, NULL},
            { 2, NULL, 1, NULL},
            { 2, NULL, 1, NULL},
            { 2, NULL, 1, NULL},
        };
        
        f32 inputs[] = {0.0f, 0.0f, 1.0f, 1.0f, 0.0f};
        f32 outputs[] = {0.0f, 1.0f, 1.0f, 0.0f};
        
        train[0].inputs = inputs;
        train[1].inputs = inputs + 1;
        train[2].inputs = inputs + 3;
        train[3].inputs = inputs + 2;
        
        train[0].outputs = outputs;
        train[1].outputs = outputs + 1;
        train[2].outputs = outputs + 2;
        train[3].outputs = outputs + 3;
        
        u32 preCorrect = evaluate(&handwrite->brain, 4, train, eval_single);
        stochastic_gradient_descent(&handwrite->randomizer, &handwrite->brain,
                                    30, 2, 0.8f, 4, train);
        u32 postCorrect = evaluate(&handwrite->brain, 4, train, eval_single);

        if ((handwrite->ticks % 20) == 0)
        {    
            u32 epochs = 20 * 30;
        fprintf(stdout, "From %u to %u in %d epoch%s. (%f%% -> %f%%)\n",
                preCorrect, postCorrect, 
                    epochs, epochs > 1 ? "s" : "",
                    100.0f * (f32)preCorrect / 4.0f,
                100.0f * (f32)postCorrect / 4.0f);
        }
    }
    
    u32 resolution = 40;
    u32 rows = image->height / resolution;
    u32 columns = image->width / resolution;
    for (u32 y = 0; y < rows; ++y)
    {
        for (u32 x = 0; x < columns; ++x)
        {
            f32 iX = (f32)x / (f32)columns;
            f32 iY = (f32)y / (f32)rows;
            
            handwrite->inputs[0] = iX;
            handwrite->inputs[1] = iY;
            predict(&handwrite->brain, handwrite->inputCount, handwrite->inputs);
            f32 gray = *handwrite->brain.outputs;
            v4 colour = V4(gray, gray, gray, 1);
            
            fill_rectangle(image, x * resolution, y * resolution, resolution, resolution,
                           colour);
        }
    }
    //fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    ++handwrite->ticks;
}
