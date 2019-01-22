#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "matrix.h"
#include "drawing.cpp"

#include "neural_net.h"

struct NeuralState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    NeuralNetwork brain;
    
    u32 inputCount;
    f32 *inputs;
    };

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(NeuralState) <= state->memorySize);
    
    NeuralState *neuralState = (NeuralState *)state->memory;
    if (!state->initialized)
    {
        //neuralState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        neuralState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        neuralState->inputCount = 2;
        neuralState->inputs = allocate_array(f32, neuralState->inputCount);
        
        init_neural_network(&neuralState->randomizer, &neuralState->brain, 
                            neuralState->inputCount, 100, 1, 0.1f);
        
    state->initialized = true;
    }
    
    f32 trainigData[4][3] = 
    {
        {0, 0, 0},
        {0, 1, 1},
        {1, 0, 1},
        {1, 1, 0},
    };

    for (u32 t = 0; t < 100; ++t)
    {
        u32 randomIndex = random_choice(&neuralState->randomizer, 4);
        train(&neuralState->brain, 2, &trainigData[randomIndex][0], 
              1, &trainigData[randomIndex][2]);
        //train(&neuralState->brain, 2, &trainigData[0][0], 1, &trainigData[0][2]);
        //train(&neuralState->brain, 2, &trainigData[1][0], 1, &trainigData[1][2]);
        //train(&neuralState->brain, 2, &trainigData[2][0], 1, &trainigData[2][2]);
        //train(&neuralState->brain, 2, &trainigData[3][0], 1, &trainigData[3][2]);
    }
    
    if ((neuralState->ticks % 100) == 0)
    {
        u32 iteration = neuralState->ticks * 100;
        fprintf(stdout, "Iteration: %u\n", iteration);
    for (u32 i = 0; i < 4; ++i)
    {
        neuralState->inputs[0] = trainigData[i][0];
        neuralState->inputs[1] = trainigData[i][1];
        feed_forward(&neuralState->brain, neuralState->inputCount, neuralState->inputs);
        fprintf(stdout, "Got: %f, expected: %f, error: %+f\n",
                    neuralState->brain.outputs.m[0],
                trainigData[i][2],
                    trainigData[i][2] - neuralState->brain.outputs.m[0]);
    }
        fprintf(stdout, "\n");
    }
    
    u32 resolution = 10;
    u32 rows = image->height / resolution;
    u32 columns = image->width / resolution;
    for (u32 y = 0; y < rows; ++y)
    {
        for (u32 x = 0; x < columns; ++x)
        {
            f32 iX = (f32)x / (f32)columns;
            f32 iY = (f32)y / (f32)rows;
            neuralState->inputs[0] = iX;
            neuralState->inputs[1] = iY;
            feed_forward(&neuralState->brain, neuralState->inputCount, neuralState->inputs);
            f32 gray = neuralState->brain.outputs.m[0];
            v4 colour = V4(gray, gray, gray, 1);
            fill_rectangle(image, x * resolution, y * resolution, resolution, resolution,
                           colour);
        }
    }
    //fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    ++neuralState->ticks;
}
