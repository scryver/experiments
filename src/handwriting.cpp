#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "random.h"
#include "drawing.cpp"

#include "matrix.h"
#include "neurons.h"

#include "mnist.h"

struct HandwriteState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    MnistSet train;
    MnistSet test;
    MnistSet validation;
    
    u32 trainCount;
    Training *training;
    u32 testCount;
    Training *tests;
    u32 validationCount;
    Training *validations;
    
    Neural brain;
    
    u32 count;
    
    u32 inputCount;
    f32 *inputs;
};

internal Training *
mnist_to_training(MnistSet *mnist)
{
    Training *result = allocate_array(Training, mnist->count);
    
    for (u32 tIdx = 0; tIdx < mnist->count; ++tIdx)
    {
        Training *item = result + tIdx;
        u8 label = mnist->labels[tIdx];
        Image *image = mnist->images + tIdx;
        
        item->inputCount = image->width * image->height;
        item->inputs = allocate_array(f32, item->inputCount);
        item->outputCount = 10;
        item->outputs = allocate_array(f32, item->outputCount);
        
        for (u32 iIdx = 0; iIdx < item->inputCount; ++iIdx)
        {
            item->inputs[iIdx] = (f32)(((u8 *)image->pixels)[iIdx]) / 255.0f;
        }
        item->outputs[(u32)label] = 1.0f;
        }
    
    return result;
}

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
        
        handwrite->train = parse_mnist("data/train-labels-idx1-ubyte", "data/train-images-idx3-ubyte");
        handwrite->test = parse_mnist("data/t10k-labels-idx1-ubyte", "data/t10k-images-idx3-ubyte");
        
        i_expect(handwrite->train.count == 60000);
        handwrite->validation = split_mnist(&handwrite->train, 50000);
        i_expect(handwrite->train.count == 50000);
        i_expect(handwrite->validation.count == 10000);
        
        handwrite->trainCount = handwrite->train.count;
        handwrite->training = mnist_to_training(&handwrite->train);
        handwrite->testCount = handwrite->test.count;
        handwrite->tests = mnist_to_training(&handwrite->test);
        handwrite->validationCount = handwrite->validation.count;
        handwrite->validations = mnist_to_training(&handwrite->validation);
        
        state->initialized = true;
    }
    
    u32 preCorrect = evaluate(&handwrite->brain,
                              handwrite->validationCount, handwrite->validations);
    
    u32 epochs = 5;
    stochastic_gradient_descent(&handwrite->randomizer, &handwrite->brain,
                                epochs, 10, 0.5f,
                                handwrite->trainCount, handwrite->training,
                                5.0f, false);
    handwrite->count += epochs;
    
    u32 postCorrect = evaluate(&handwrite->brain,
                               handwrite->validationCount, handwrite->validations);
    
    f32 preProcent = 100.0f * (f32)preCorrect / (f32)handwrite->validationCount;
    f32 postProcent = 100.0f * (f32)postCorrect / (f32)handwrite->validationCount;
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
    draw_mnist(image, 10, 10, handwrite->train.images);
    draw_mnist(image, 10, 60, handwrite->test.images);
    draw_mnist(image, 10, 110, handwrite->validation.images);
    //fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    ++handwrite->ticks;
}
