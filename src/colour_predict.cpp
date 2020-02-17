#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"
#include "../libberdip/std_file.c"

#include "matrix.h"
#include "aitraining.h"
#include "neural_net.h"
#include "neurons.h"

struct ColourState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    Neural brain;
    
    u32 inputCount;
    f32 *inputs;
};

internal void
print_network(Neural *network)
{
    fprintf(stdout, "Network  : %p\n", (void *)network);
    
    fprintf(stdout, "  inputs : %d\n", network->inputCount);
    fprintf(stdout, "  layers : %d\n", network->layerCount);
    fprintf(stdout, "           [");
    for (s32 layerIndex = 0; layerIndex < (s32)network->layerCount - 1; ++layerIndex)
    {
        fprintf(stdout, "%d", network->layerSizes[layerIndex]);
        if (layerIndex == network->layerCount - 2)
        {
            fprintf(stdout, "]\n");
        }
        else
        {
            fprintf(stdout, ", ");
        }
    }
    fprintf(stdout, "  outputs: %d\n", network->outputCount);
    
    fprintf(stdout, "  internals:\n");
    
    u32 rows = network->inputCount;
    f32 *hidden = network->hidden;
    f32 *h2hWeight = network->weights;
    f32 *hiddenBias = network->biases;
    for (u32 layerIndex = 0; layerIndex < network->layerCount - 1; ++layerIndex)
    {
        u32 count;
        
        if (layerIndex < network->layerCount - 1)
        {
            count = network->layerSizes[layerIndex];
            fprintf(stdout, "    hidden[%d]: [", layerIndex + 1);
            for (u32 index = 0; index < count; ++index)
            {
                fprintf(stdout, "%f", hidden[index]);
                if (index == count - 1)
                {
                    fprintf(stdout, "]\n");
                }
                else
                {
                    fprintf(stdout, ", ");
                }
            }
            
            fprintf(stdout, "    hiddenWeight[%d]:\n", layerIndex + 1);
            for (u32 row = 0; row < count; ++row)
            {
                fprintf(stdout, "      [");
                for (u32 col = 0; col < rows; ++col)
                {
                    fprintf(stdout, "% 5.2f", h2hWeight[row * rows + col]);
                    if (col == rows - 1)
                    {
                        fprintf(stdout, "]\n");
                    }
                    else
                    {
                        fprintf(stdout, ", ");
                    }
                }
            }
            
            h2hWeight += count * rows;
            
            fprintf(stdout, "    hiddenBias[%d]: [", layerIndex + 1);
            for (u32 index = 0; index < count; ++index)
            {
                fprintf(stdout, "%f", hiddenBias[index]);
                if (index == count - 1)
                {
                    fprintf(stdout, "]\n");
                }
                else
                {
                    fprintf(stdout, ", ");
                }
            }
        }
        else
        {
            count = network->layerSizes[network->layerCount - 1];
            fprintf(stdout, "    outputWeights:\n");
            for (u32 row = 0; row < count; ++row)
            {
                fprintf(stdout, "      [");
                for (u32 col = 0; col < rows; ++col)
                {
                    fprintf(stdout, "% 5.2f", h2hWeight[row * rows + col]);
                    if (col == rows - 1)
                    {
                        fprintf(stdout, "]\n");
                    }
                    else
                    {
                        fprintf(stdout, ", ");
                    }
                }
            }
        }
        rows = count;
        hidden += count;
        hiddenBias += count;
    }
    
    fprintf(stdout, "    outputs: [");
    for (u32 index = 0; index < network->outputCount; ++index)
    {
        fprintf(stdout, "%f", network->outputs[index]);
        if (index == network->outputCount - 1)
        {
            fprintf(stdout, "]\n");
        }
        else
        {
            fprintf(stdout, ", ");
        }
    }
    
    fprintf(stdout, "    outputBias: [");
    for (u32 index = 0; index < network->outputCount; ++index)
    {
        fprintf(stdout, "%f", network->biases[index + network->totalNeurons - network->outputCount]);
        if (index == network->outputCount - 1)
        {
            fprintf(stdout, "]\n");
        }
        else
        {
            fprintf(stdout, ", ");
        }
    }
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(ColourState) <= state->memorySize);
    
    ColourState *colourState = (ColourState *)state->memory;
    if (!state->initialized)
    {
        //colourState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        colourState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        colourState->inputCount = 2;
        colourState->inputs = allocate_array(f32, colourState->inputCount);
        
        u32 hiddenCounts[10] = {100, 100, 100, 10, 10, 10, 10, 10, 10, 10};
        init_neural_network(&colourState->brain, colourState->inputCount, 3, hiddenCounts, 1);
        randomize_weights(&colourState->randomizer, &colourState->brain);
        
        // print_network(&colourState->brain);
        
        state->initialized = true;
    }
    
    f32 trainigData[4][3] = 
    {
        {0, 0, 0},
        {0, 1, 1},
        {1, 0, 1},
        {1, 1, 0},
    };
    
#if 1
    for (u32 t = 0; t < 100; ++t)
    {
        u32 randomIndex = random_choice(&colourState->randomizer, 4);
        train(&colourState->brain, 2, &trainigData[randomIndex][0], 
              1, &trainigData[randomIndex][2], 0.1f);
    }
#endif
    
    if ((colourState->ticks % 100) == 0)
    {
        u32 iteration = colourState->ticks * 100;
        fprintf(stdout, "Iteration: %u\n", iteration);
        for (u32 i = 0; i < 4; ++i)
        {
            colourState->inputs[0] = trainigData[i][0];
            colourState->inputs[1] = trainigData[i][1];
            predict(&colourState->brain, colourState->inputCount, colourState->inputs);
            fprintf(stdout, "Got: %f, expected: %f, error: %+f\n",
                    *colourState->brain.outputs,
                    trainigData[i][2],
                    trainigData[i][2] - *colourState->brain.outputs);
        }
        fprintf(stdout, "\n");
        // print_network(&colourState->brain);
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
            colourState->inputs[0] = iX;
            colourState->inputs[1] = iY;
            predict(&colourState->brain, colourState->inputCount, colourState->inputs);
            f32 gray = *colourState->brain.outputs;
            v4 colour = V4(gray, gray, gray, 1);
            fill_rectangle(image, x * resolution, y * resolution, resolution, resolution,
                           colour);
        }
    }
    //fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    ++colourState->ticks;
}
