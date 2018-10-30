#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "random.h"
#include "drawing.cpp"

#include "aitraining.h"
#include "neuronlayer.h"

struct HandwriteState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    u32 prevMouseDown;
    
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
        
        init_neural_network(&handwrite->brain, 2);
        add_feature_map_layer(&handwrite->brain, 3, 5, 5, 3, 3);
        add_fully_connected_layer(&handwrite->brain, 3 * 3 * 3, 4, true);
        finish_network(&handwrite->brain);
        
        randomize_weights(&handwrite->randomizer, &handwrite->brain);
        
        state->initialized = true;
    }
    
    TrainingSet testDataSet = {};
    testDataSet.count = 3;
    testDataSet.set = allocate_array(Training, testDataSet.count);
    
    for (u32 test = 0; test < testDataSet.count; ++test)
    {
    Training *testData = testDataSet.set + test;
    testData->inputCount = 25;
    testData->outputCount = 4;
    testData->inputs = allocate_array(f32, testData->inputCount);
    testData->outputs = allocate_array(f32, testData->outputCount);
    }
    
    
    {
        Training *testData = testDataSet.set;
    
        for (u32 y = 0; y < 5; ++y)
        {
            f32 *inputs = testData->inputs + y * 5;
            inputs[0] = 0.0f;
            inputs[1] = 0.1f;
            inputs[2] = 0.8f;
            inputs[3] = 0.1f;
            inputs[4] = 0.0f;
        }
        
        //for (u32 y = 0; y < 4; ++y)
        {
            f32 *outputs = testData->outputs; // + y * 4;
            outputs[0] = 0.0f;
            outputs[1] = 1.0f;
            outputs[2] = 0.0f;
            outputs[3] = 0.0f;
        }
        
        ++testData;
        for (u32 y = 0; y < 5; ++y)
        {
            f32 *inputs = testData->inputs + y * 5;
            inputs[0] = 0.9f;
            inputs[1] = 0.1f + 0.1f * y;
            inputs[2] = 0.0f + 0.1f * y;
            inputs[3] = 0.0f + 0.1f * y;
            inputs[4] = 0.0f + 0.1f * y;
        }
        
        //for (u32 y = 0; y < 4; ++y)
        {
            f32 *outputs = testData->outputs; // + y * 4;
            outputs[0] = 0.0f;
            outputs[1] = 0.0f;
            outputs[2] = 1.0f;
            outputs[3] = 0.0f;
        }
        
        ++testData;
        for (u32 y = 0; y < 5; ++y)
        {
            f32 *inputs = testData->inputs + y * 5;
            inputs[0] = 0.0f + 0.15f * y;
            inputs[1] = 0.1f + 0.15f * y;
            inputs[2] = 0.8f + 0.015f * y;
            inputs[3] = 0.1f + 0.15f * y;
            inputs[4] = 0.0f + 0.15f * y;
        }
        
        //for (u32 y = 0; y < 4; ++y)
        {
            f32 *outputs = testData->outputs; // + y * 4;
            outputs[0] = 1.0f;
            outputs[1] = 0.0f;
            outputs[2] = 0.0f;
            outputs[3] = 0.0f;
        }
        
    }
    
    NeuralLayer *feature = handwrite->brain.layers;
    
    u32 resolution = 20;
    for (u32 t = 0; t < testDataSet.count; ++t)
    {
        u32 xBase = 10 + t * resolution * (5 + 8);
        u32 xOffset = xBase;
            u32 yOffset = 10;
    Training *train = testDataSet.set + t;
        predict(&handwrite->brain, train);
    
        f32 *inputs = train->inputs;
        for (u32 y = 0; y < 5; ++y)
        {
            f32 *inRow = inputs + y * 5;
            for (u32 x = 0; x < 5; ++x)
            {
                f32 gray = inRow[x];
                v4 colour = V4(gray, gray, gray, 1.0f);
                fill_rectangle(image, xOffset + x * resolution, yOffset + y * resolution,
                               resolution, resolution, colour);
            }
        }
        outline_rectangle(image, xOffset, yOffset, resolution * 5, resolution * 5,
                          V4(0, 1, 0, 1));
        
    xOffset = xBase;
    yOffset += resolution * 5 + 5;
    
    u32 rows = 3;
    u32 columns = 3;
    f32 *weights = feature->featureMap.weights;
    for (u32 m = 0; m < feature->featureMap.mapCount && m < 7 * 3; ++m)
    {
        f32 *wMap = weights + m * rows * columns;
        for (u32 y = 0; y < rows; ++y)
        {
            f32 *wRow = wMap + y * columns;
            for (u32 x = 0; x < columns; ++x)
            {
                    f32 gray = wRow[x];
                    //f32 gray = activate_neuron(wRow[x]);
                v4 colour = V4(gray, gray, gray, 1);
                fill_rectangle(image, xOffset + x * resolution, yOffset + y * resolution,
                               resolution, resolution, colour);
            }
        }
        outline_rectangle(image, xOffset, yOffset, resolution * columns, resolution * rows,
                          V4(0, 1, 0, 1));
        
        xOffset += resolution * columns + 5;
        
        if (((m + 1) % 7) == 0)
        {
            yOffset += resolution * rows + 5;
            xOffset = xBase;
        }
    }
    
     xOffset = xBase;
    yOffset += resolution * rows + 5;
    
    for (u32 y = 0; y < 2; ++y)
    {
        f32 *outRow = train->outputs + y * 2;
        for (u32 x = 0; x < 2; ++x)
        {
            f32 gray = outRow[x];
            v4 colour = V4(gray, gray, gray, 1.0f);
            fill_rectangle(image, xOffset + x * resolution, yOffset + y * resolution,
                           resolution, resolution, colour);
        }
    }
        outline_rectangle(image, xOffset, yOffset, resolution * 2, resolution * 2,
                          V4(0, 1, 0, 1));
        
        xOffset += resolution * 2 + 5;
    
    f32 *outputs = feature->outputs;
    for (u32 m = 0; m < feature->featureMap.mapCount && m < 7 * 3; ++m)
    {
        u32 outputStep = feature->outputCount / feature->featureMap.mapCount;
    for (u32 y = 0; y < 2; ++y)
    {
        f32 *outRow = outputs + y * 2;
        for (u32 x = 0; x < 2; ++x)
        {
            f32 gray = outRow[x];
            v4 colour = V4(gray, gray, gray, 1.0f);
            fill_rectangle(image, xOffset + x * resolution, yOffset + y * resolution,
                           resolution, resolution, colour);
        }
        }
        outline_rectangle(image, xOffset, yOffset, resolution * 2, resolution * 2,
                          V4(0, 1, 0, 1));
        
        xOffset += resolution * 2 + 5;
        outputs += outputStep;
    }

    {
        TempMemory tempMem = temporary_memory();
        
        f32 *deltaWeights = allocate_array(f32, 3 * 3 * feature->featureMap.mapCount);
        f32 *deltaBias = allocate_array(f32, feature->featureMap.mapCount);
    back_propagate(&handwrite->brain, train, deltaWeights, deltaBias);
        
        xOffset = xBase;
        yOffset += resolution * 2 + 5;
        
        f32 *weights = deltaWeights;
        f32 *bias = deltaBias;
        for (u32 m = 0; m < feature->featureMap.mapCount && m < 7 * 3; ++m)
        {
        for (u32 y = 0; y < rows; ++y)
        {
            f32 *wRow = weights + y * columns;
            for (u32 x = 0; x < columns; ++x)
            {
                        f32 gray = wRow[x];
                //f32 gray = activate_neuron(wRow[x]);
                        v4 colour = V4(gray, gray, gray, 1);
                fill_rectangle(image, xOffset + x * resolution, yOffset + y * resolution,
                               resolution, resolution, colour);
            }
            }
            outline_rectangle(image, xOffset, yOffset, resolution * columns, resolution * rows,
                              V4(0, 1, 0, 1));
            
            weights += feature->featureMap.featureWidth * feature->featureMap.featureHeight;
        
        f32 b = activate_neuron(*bias++);
        v4 colour = V4(b, b, b, 1.0f);
        fill_rectangle(image, xOffset, yOffset + resolution * rows + 5, resolution, resolution, colour);
                outline_rectangle(image, xOffset, yOffset + resolution * rows + 5, resolution, resolution, V4(0, 1, 0, 1));
                
                xOffset += resolution * columns + 5;
        }
        
        xOffset = xBase;
            yOffset += resolution * (rows + 2) + 5;
        
        f32 *outputs = feature->outputs;
        for (u32 m = 0; m < feature->featureMap.mapCount && m < 7 * 3; ++m)
        {
            u32 outputStep = feature->outputCount / feature->featureMap.mapCount;
            for (u32 y = 0; y < 2; ++y)
            {
                f32 *outRow = outputs + y * 2;
                for (u32 x = 0; x < 2; ++x)
                {
                    f32 gray = activate_neuron(outRow[x]);
                    v4 colour = V4(gray, gray, gray, 1.0f);
                    fill_rectangle(image, xOffset + x * resolution, yOffset + y * resolution,
                                   resolution, resolution, colour);
                }
            }
            outline_rectangle(image, xOffset, yOffset, resolution * 2, resolution * 2,
                              V4(0, 1, 0, 1));
            
            xOffset += resolution * 2 + 5;
            outputs += outputStep;
        }
        
        destroy_temporary(tempMem);
    }
    }
    
    //if ((handwrite->ticks % 256) == 0)
    if ((mouse.mouseDowns & Mouse_Left) && !(handwrite->prevMouseDown & Mouse_Left))
    {
        u32 preCorrect = evaluate(&handwrite->brain, testDataSet);
        
        u32 epochs = 1;
        stochastic_gradient_descent(&handwrite->randomizer, &handwrite->brain,
                                    epochs, 1, 0.05f, testDataSet, 2.0f);
        handwrite->epochCount += epochs;
        
        u32 postCorrect = evaluate(&handwrite->brain, testDataSet);
        
        f32 preProcent = 100.0f * (f32)preCorrect / (f32)testDataSet.count;
        f32 postProcent = 100.0f * (f32)postCorrect / (f32)testDataSet.count;
        fprintf(stdout, "%4d: From %4u to %4u in %4d epoch%s. (%2.2f%% -> %2.2f%%, %+0.2f%%)\n",
                handwrite->epochCount,
                preCorrect, postCorrect, epochs, epochs == 1 ? "" : "s",
                preProcent, postProcent, postProcent - preProcent);
    }
    if ((mouse.mouseDowns & Mouse_Right) && !(handwrite->prevMouseDown & Mouse_Right))
    {
        randomize_weights(&handwrite->randomizer, &handwrite->brain);
    }

    handwrite->prevMouseDown = mouse.mouseDowns;
    ++handwrite->ticks;
}
