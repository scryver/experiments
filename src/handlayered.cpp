#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"
#include "../libberdip/std_file.c"
#include "aitraining.h"
#include "neuronlayer.h"

#include "mnist.h"

struct HandwriteState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    TrainingSet train;
    TrainingSet test;
    TrainingSet validation;
    
    NeuralCake brain;
    
    u32 epochCount;
    
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
        
        init_neural_network(&handwrite->brain, 3);
        add_max_pooling_feature_map_layer(&handwrite->brain, 20, 28, 28, 5, 5, 2, 2);
        add_fully_connected_layer(&handwrite->brain, 20 * 12 * 12, 100);
        add_fully_connected_layer(&handwrite->brain, 100, 10, true);
        finish_network(&handwrite->brain);
        
        randomize_weights(&handwrite->randomizer, &handwrite->brain);
        
        handwrite->train = parse_training("data/mnist-f32train");
        handwrite->test = parse_training("data/mnist-f32test");
        handwrite->validation = parse_training("data/mnist-f32validation");
        
        state->initialized = true;
    }
    
    NeuralLayer *feature = handwrite->brain.layers;
    u32 resolution = 20;
    u32 rows = 5;
    u32 columns = 5;
    u32 xOffset = 10;
    u32 yOffset = 10;
    f32 *weights = feature->featureMap.weights;
    for (u32 m = 0; m < feature->featureMap.mapCount && m < 7 * 5; ++m)
    {
        f32 *wMap = weights + m * rows * columns;
        for (u32 y = 0; y < rows; ++y)
    {
            f32 *wRow = wMap + y * columns;
        for (u32 x = 0; x < columns; ++x)
        {
            f32 gray = activate_neuron(wRow[x]);
            v4 colour = V4(gray, gray, gray, 1);
            fill_rectangle(image, xOffset + x * resolution, yOffset + y * resolution,
                           resolution, resolution, colour);
        }
    }
        
        xOffset += resolution * columns + 5;
        
        if (((m + 1) % 7) == 0)
        {
            yOffset += resolution * rows + 5;
            xOffset = 10;
        }
    }
    
    if (handwrite->ticks)
    {
    u32 preCorrect = evaluate(&handwrite->brain, handwrite->validation);
    
    u32 epochs = 1;
    stochastic_gradient_descent(&handwrite->randomizer, &handwrite->brain,
                                epochs, 10, 0.5f, handwrite->train, 5.0f);
    handwrite->epochCount += epochs;
    
    u32 postCorrect = evaluate(&handwrite->brain, handwrite->validation);
    
    f32 preProcent = 100.0f * (f32)preCorrect / (f32)handwrite->validation.count;
    f32 postProcent = 100.0f * (f32)postCorrect / (f32)handwrite->validation.count;
    fprintf(stdout, "%4d: From %4u to %4u in %4d epoch%s. (%2.2f%% -> %2.2f%%, %+0.2f%%)\n",
            handwrite->epochCount,
            preCorrect, postCorrect, epochs, epochs == 1 ? "" : "s",
            preProcent, postProcent, postProcent - preProcent);
    }
    
    ++handwrite->ticks;
}
