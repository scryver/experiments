#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"
#include "../libberdip/std_file.c"

#include "matrix.h"
#include "aitraining.h"
#include "neurons.h"

struct NeuronCalcState
{
    RandomSeriesPCG randomizer;
    u32 ticks;
    
    Neural brain;
    
    u32 inputCount;
    f32 *inputs;
};

internal void
print_hidden2(u32 layerCount, u32 *layerSizes, f32 *hidden)
{
    // NOTE(michiel): Can be used for biases as well
    
    /*
    * ------
    * | 0.1|
    * |-0.3|
    * | 0.2|
    * ------
*
    * ------
    * |-0.4|
    * | 0.5| 
    * ------
*
    * ------
    * | 0.6|
    * ------
*/
    f32 *h = hidden;
    for (u32 layerIdx = 0; layerIdx < layerCount; ++layerIdx)
    {
        u32 count = layerSizes[layerIdx];
        fprintf(stdout, "-------------\n|");
        
        for (u32 c = 0; c < count; ++c)
        {
            fprintf(stdout, "% .8f|\n%s", *h++, c < (count - 1) ? "|" : "");
        }
        
        fprintf(stdout, "-------------\n\n");
    }
}

internal void
print_weights2(u32 inputCount, u32 layerCount, u32 *layerSizes, f32 *weights)
{
    /*
    * -----------
    * | 0.3  0.6|
    * |-1.0  0.7|
    * | 0.7  0.3|
    * -----------
    *
    * ----------------
    * |-1.0  0.4 -2.0|
    * | 3.0  0.5 -1.0|
    * ----------------
    *
    * -----------
    * |-1.2  0.3|
    * -----------
    */
    u32 prevCount = inputCount;
    f32 *w = weights;
    for (u32 layerIdx = 0; layerIdx < layerCount; ++layerIdx)
    {
        u32 count = layerSizes[layerIdx];
        fprintf(stdout, "-");
        for (u32 p = 0; p < prevCount; ++p)
        {
            fprintf(stdout, "------------");
        }
        fprintf(stdout, "\n|");
        
        for (u32 c = 0; c < count; ++c)
        {
            for (u32 p = 0; p < prevCount; ++p)
            {
                fprintf(stdout, "% .8f%s", *w++, p < (prevCount - 1) ? "," : "");
            }
            fprintf(stdout, "|\n%s", c < (count - 1) ? "|" : "");
        }
        
        fprintf(stdout, "-");
        for (u32 p = 0; p < prevCount; ++p)
        {
            fprintf(stdout, "------------");
        }
        fprintf(stdout, "\n\n");
        
        prevCount = count;
    }
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(NeuronCalcState) <= state->memorySize);
    
    NeuronCalcState *neuronCalc = (NeuronCalcState *)state->memory;
    if (!state->initialized)
    {
        neuronCalc->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        //neuronCalc->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        Neural *net = &neuronCalc->brain;
        neuronCalc->inputCount = 2;
        neuronCalc->inputs = allocate_array(f32, neuronCalc->inputCount);
        
        u32 hiddenCounts[] = {3, 2};
        init_neural_network(net, neuronCalc->inputCount, 
                            array_count(hiddenCounts), hiddenCounts, 1);
        
        neuronCalc->inputs[0] = 0.5f;
        neuronCalc->inputs[1] = 0.7f;
        
        f32 *weights = net->weights;
        i_expect(net->totalWeights == 14);
        // NOTE(michiel): Row 0 of Input -> Layer 0
        weights[0] = 0.3f;
        weights[1] = 0.6f;
        // NOTE(michiel): Row 1 of Input -> Layer 0
        weights[2] = -1.0f;
        weights[3] = 0.7f;
        // NOTE(michiel): Row 2 of Input -> Layer 0
        weights[4] = 0.7f;
        weights[5] = 0.3f;
        // NOTE(michiel): Row 0 of Layer 0 -> Layer 1
        weights[6] = -1.0f;
        weights[7] = 0.4f;
        weights[8] = -2.0f;
        // NOTE(michiel): Row 1 of Layer 0 -> Layer 1
        weights[9] = 3.0f;
        weights[10] = 0.5f;
        weights[11] = -1.0f;
        // NOTE(michiel): Row 0 of Layer 1 -> Output
        weights[12] = -1.2f;
        weights[13] = 0.3f;
        
        print_weights(neuronCalc->inputCount, net->layerCount, net->layerSizes, net->weights);
        
        f32 *biases = net->biases;
        i_expect(net->totalNeurons == 6);
        // NOTE(michiel): Layer 0
        biases[0] = 1.0f;
        biases[1] = -1.0f;
        biases[2] = 0.5f;
        // NOTE(michiel): Layer 1
        biases[3] = 0.6f;
        biases[4] = -0.8f;
        // NOTE(michiel): Output
        biases[5] = 0.5f;
        
        print_hidden(net->layerCount, net->layerSizes, net->biases);
        
        /*
        *       a
*  x1       d
*       b      y
*  x2       e
*       c
*
* a =  0.3*x1 + 0.6*x2 + 1.0 =  0.3*0.5 + 0.6*0.7 + 1 =  1.57 => 0.827783608
* b = -1.0*x1 + 0.7*x2 - 1.0 = -1.0*0.5 + 0.7*0.7 + 1 = -1.01 => 0.266979851
* c =  0.7*x1 + 0.3*x2 + 0.5 =  0.7*0.5 + 0.3*0.7 + 1 =  1.06 => 0.742690545
*
* d = -1.0*a + 0.4*b - 2.0*c + 0.6 = −1.606372759 => 0.167092817
* e =  3.0*a + 0.5*b - 1.0*c - 0.8 =  1.074150205 => 0.745385369
* 
* y = -1.2*d + 0.3*e + 0.5 = 0.52310423 => 0.627873352
 */
        
        predict(net, neuronCalc->inputCount, neuronCalc->inputs);
        fprintf(stdout, "Hand calc it would be 0.627873352, neurons says its %.9f\n",
                net->outputs[0]);
        print_hidden(net->layerCount, net->layerSizes, net->hidden);
        
        fprintf(stdout,
                "--------------  --------------  --------------\n"
                "| 0.827783608|  | 0.167092817|  | 0.627873352|\n"
                "| 0.266979851|  | 0.745385369|  --------------\n"
                "| 0.742690545|  --------------\n"
                "--------------\n");
        
        Training train = {};
        train.inputCount = 2;
        train.outputCount = 1;
        train.inputs = allocate_array(f32, 2);
        train.outputs = allocate_array(f32, 1);
        train.inputs[0] = 0.5f;
        train.inputs[1] = 0.7f;
        train.outputs[0] = 0.5f;
        
        f32 *deltaWeights = allocate_array(f32, net->totalWeights);
        f32 *deltaBiases  = allocate_array(f32, net->totalNeurons);
#if 1
        back_propagate(net, &train, deltaWeights, deltaBiases);
#else
        predict(train.inputCount, train.inputs, net->layerCount, net->layerSizes,
                net->hidden, net->weights, net->biases);
        
        TempMemory tempMem = temporary_memory();
        
        f32 *act = net->hidden;
        f32 *w = net->weights + net->totalWeights;
        
        f32 *dnb = deltaBiases + net->totalNeurons;
        f32 *dnw = deltaWeights + net->totalWeights;
        
        f32 *errors = allocate_array(f32, net->totalNeurons);
        f32 *nextErr = allocate_array(f32, net->totalNeurons);
        
        for (s32 layerIdx = net->layerCount - 1; layerIdx >= 0; --layerIdx)
        {
            u32 count = net->layerSizes[layerIdx];
            u32 prevCount = train.inputCount;
            if (layerIdx > 0)
            {
                prevCount = net->layerSizes[layerIdx - 1];
            }
            
            dnb -= count;
            dnw -= count * prevCount;
            
            if (layerIdx == net->layerCount - 1)
            {
                // NOTE(michiel): LayerIdx = -1
                i_expect(count == 1);
                i_expect(prevCount == 2);
                
                errors[0] = act[5] - train.outputs[0];
                
                dnb[0] = errors[0];
                dnw[0] = errors[0] * act[3];
                dnw[1] = errors[0] * act[4];
            }
            else if (layerIdx == net->layerCount - 2)
            {
                // NOTE(michiel): LayerIdx = -2
                i_expect(count == 2);
                i_expect(prevCount == 3);
                
                errors[0] = w[0] * nextErr[0] * (act[3] * (1.0f - act[3]));
                errors[1] = w[1] * nextErr[0] * (act[4] * (1.0f - act[4]));
                
                dnb[0] = errors[0];
                dnb[1] = errors[1];
                dnw[0] = errors[0] * act[0];
                dnw[1] = errors[0] * act[1];
                dnw[2] = errors[0] * act[2];
                dnw[3] = errors[1] * act[0];
                dnw[4] = errors[1] * act[1];
                dnw[5] = errors[1] * act[2];
            }
            else if (layerIdx == 0)
            {
                // NOTE(michiel): LayerIdx = -3 = Input
                i_expect(count == 3);
                i_expect(prevCount == 2);
                
                errors[0] = (w[0] * nextErr[0] + w[3] * nextErr[1]) * (act[0] * (1.0f - act[0]));
                errors[1] = (w[1] * nextErr[0] + w[4] * nextErr[1]) * (act[1] * (1.0f - act[1]));
                errors[2] = (w[2] * nextErr[0] + w[5] * nextErr[1]) * (act[2] * (1.0f - act[2]));
                
                dnb[0] = errors[0];
                dnb[1] = errors[1];
                dnb[2] = errors[2];
                dnw[0] = errors[0] * train.inputs[0];
                dnw[1] = errors[0] * train.inputs[1];
                dnw[2] = errors[1] * train.inputs[0];
                dnw[3] = errors[1] * train.inputs[1];
                dnw[4] = errors[2] * train.inputs[0];
                dnw[5] = errors[2] * train.inputs[1];
            }
            
            w -= count * prevCount;
            
            f32 *t = errors;
            errors = nextErr;
            nextErr = t;
        }
        
        destroy_temporary(tempMem);
#endif
        
        fprintf(stdout, "dWeights\n");
        print_weights(neuronCalc->inputCount, net->layerCount, net->layerSizes, deltaWeights);
        
        fprintf(stdout, "dBiases\n");
        print_hidden(net->layerCount, net->layerSizes, deltaBiases);
        
        state->initialized = true;
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    ++neuronCalc->ticks;
}
