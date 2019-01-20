#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>     // PROT_*, MAP_*, munmap

#include "../libberdip/platform.h"
#include "../src/interface.h"

#include "../libberdip/random.h"
#include "../libberdip/std_file.c"

#define MATRIX_TEST 1
#include "../src/matrix.h"
#include "../src/aitraining.h"
#include "../src/neural_net.h"
#include "../src/neurons.h"

int main(int argc, char **argv)
{
    test_matrix();
    
    RandomSeriesPCG random = random_seed_pcg(120497190247, 19827491724124);
    NeuralNetwork oldNet = {};
    Neural newNet = {};
    
    u32 inputCount = 4;
    f32 inputs[4] = {1.0f, 0.1f, 0.2f, -0.4f};
    u32 hiddenDepth = 1;
    u32 hiddenCount[1] = {3};
    u32 outputCount = 2;
    
    f32 learningRate = 1.0f;
    
    init_neural_network(&random, &oldNet, inputCount, hiddenCount[0], outputCount,
                        learningRate);
    init_neural_network(&newNet, inputCount, hiddenDepth, hiddenCount, outputCount);
    
    f32 *h2hWeights = newNet.weights;
    for (u32 row = 0; row < oldNet.inputToHiddenWeights.rows; ++row)
    {
        f32 *rowOld = oldNet.inputToHiddenWeights.m + row * oldNet.inputToHiddenWeights.columns;
        f32 *rowNew = h2hWeights;
        for (u32 col = 0; col < oldNet.inputToHiddenWeights.columns; ++col)
        {
            rowNew[col] = rowOld[col];
        }
        h2hWeights += oldNet.inputToHiddenWeights.columns;
    }

#if 0    
    f32 *rowOld = oldNet.hiddenBias.m;
    f32 *rowNew = newNet.hiddenBias;
    for (u32 row = 0; row < oldNet.hiddenBias.rows; ++row)
    {
        *rowNew++ = *rowOld++;
    }
    #endif

    for (u32 row = 0; row < oldNet.hiddenBias.rows; ++row)
    {
        f32 *rowOld = oldNet.hiddenBias.m + row * oldNet.hiddenBias.columns;
        f32 *rowNew = newNet.biases + row * oldNet.hiddenBias.columns;
        for (u32 col = 0; col < oldNet.hiddenBias.columns; ++col)
        {
            rowNew[col] = rowOld[col];
        }
    }
    
    for (u32 row = 0; row < oldNet.hiddenToOutputWeights.rows; ++row)
    {
        f32 *rowOld = oldNet.hiddenToOutputWeights.m + row * oldNet.hiddenToOutputWeights.columns;
        f32 *rowNew = h2hWeights + row * oldNet.hiddenToOutputWeights.columns;
        for (u32 col = 0; col < oldNet.hiddenToOutputWeights.columns; ++col)
        {
            rowNew[col] = rowOld[col];
        }
    }
    
    for (u32 row = 0; row < oldNet.outputBias.rows; ++row)
    {
        f32 *rowOld = oldNet.outputBias.m + row * oldNet.outputBias.columns;
        f32 *rowNew = newNet.biases + newNet.totalNeurons - newNet.outputCount + row * oldNet.outputBias.columns;
        for (u32 col = 0; col < oldNet.outputBias.columns; ++col)
        {
            rowNew[col] = rowOld[col];
        }
    }
    
    feed_forward(&oldNet, inputCount, inputs);
    predict(&newNet, inputCount, inputs);
    
    fprintf(stdout, "Orig: | %f %f |, new: | %f %f |\n",
            oldNet.outputs.m[0], oldNet.outputs.m[1],
            newNet.outputs[0], newNet.outputs[1]);
    
    fprintf(stdout, "H Or: | %f %f %f |, new: | %f %f %f |\n",
            oldNet.hidden.m[0], oldNet.hidden.m[1], oldNet.hidden.m[2],
            newNet.hidden[0], newNet.hidden[1], newNet.hidden[2]);
    
    feed_forward(&oldNet, inputCount, inputs);
    predict(&newNet, inputCount, inputs);
    
    fprintf(stdout, "Orig: | %f %f |, new: | %f %f |\n",
            oldNet.outputs.m[0], oldNet.outputs.m[1],
            newNet.outputs[0], newNet.outputs[1]);
    
    fprintf(stdout, "H Or: | %f %f %f |, new: | %f %f %f |\n",
            oldNet.hidden.m[0], oldNet.hidden.m[1], oldNet.hidden.m[2],
            newNet.hidden[0], newNet.hidden[1], newNet.hidden[2]);
    
    feed_forward(&oldNet, inputCount, inputs);
    predict(&newNet, inputCount, inputs);
    
    fprintf(stdout, "Orig: | %f %f |, new: | %f %f |\n",
            oldNet.outputs.m[0], oldNet.outputs.m[1],
            newNet.outputs[0], newNet.outputs[1]);
    
    fprintf(stdout, "H Or: | %f %f %f |, new: | %f %f %f |\n",
            oldNet.hidden.m[0], oldNet.hidden.m[1], oldNet.hidden.m[2],
            newNet.hidden[0], newNet.hidden[1], newNet.hidden[2]);
    
    fprintf(stdout, "Training\n");

f32 trainingData[6] = {0.1f, 0.2f, 0.3f, 0.4f, -1.0f, 1.0f};
    
    #if 1
    train(&oldNet, inputCount, trainingData, outputCount, trainingData + inputCount);
    train(&newNet, inputCount, trainingData, outputCount, trainingData + inputCount,
          learningRate);
    
    feed_forward(&oldNet, inputCount, inputs);
    predict(&newNet, inputCount, inputs);
    
    fprintf(stdout, "Orig: | %f %f |, new: | %f %f |\n",
            oldNet.outputs.m[0], oldNet.outputs.m[1],
            newNet.outputs[0], newNet.outputs[1]);
    
    fprintf(stdout, "H Or: | %f %f %f |, new: | %f %f %f |\n",
            oldNet.hidden.m[0], oldNet.hidden.m[1], oldNet.hidden.m[2],
            newNet.hidden[0], newNet.hidden[1], newNet.hidden[2]);
    
    #else
    
    feed_forward(&oldNet, inputCount, inputs);
    predict(&newNet, inputCount, inputs);
    
    //
    // NOTE(michiel): Old gradient/weight calc
    //
    
    Matrix answer = matrix_from_array(outputCount, 1, trainingData + inputCount);
    
    // NOTE(michiel): Calculate errors
    Matrix outputErrors = answer - oldNet.outputs;
    
    // NOTE(michiel): Calculate output gradient
    Matrix gradients = {};
    init_matrix(&gradients, oldNet.outputs.rows, oldNet.outputs.columns);
    matrix_copy(gradients.rows, gradients.columns, oldNet.outputs.m, gradients.m);
    map(&gradients, dsigmoid);
    hadamard(outputErrors, &gradients);
    gradients *= oldNet.learningRate;
    
    // NOTE(michiel): Calculate hidden weight deltas
    Matrix hiddenT = transpose(oldNet.hidden);
    Matrix weightsHDeltas = gradients * hiddenT;
    
    // NOTE(michiel): Add the deltas
    oldNet.outputBias += gradients;
    oldNet.hiddenToOutputWeights += weightsHDeltas;
    
    //
    // NOTE(michiel): New gradient/weight calc
    //
    
    f32 *currentErrors = newNet.trainErrorsA;
    f32 *nextErrors = newNet.trainErrorsB;
    
    // E = T - O;
    subtract_array(newNet.outputCount, trainingData + inputCount, 
                   newNet.outputs, currentErrors);
    // G = hadamard(1 / (1 - exp(-O)), E) * learningRate;
    calculate_gradient(newNet.outputCount, newNet.outputs, currentErrors,
                       newNet.trainGradient, learningRate);
    // OB += G
    add_to_array(newNet.outputCount, newNet.trainGradient, newNet.outputBias);
    // OW += G * transpose(H[-1]);
    add_weights(newNet.outputCount, newNet.hiddenCount[0], 
                newNet.trainGradient, newNet.hidden, newNet.h2oWeights);
    
    fprintf(stdout, "Weights Orig: | %f %f |, new: | %f %f |\n",
            oldNet.hiddenToOutputWeights.m[0], oldNet.hiddenToOutputWeights.m[1],
            newNet.h2oWeights[0], newNet.h2oWeights[1]);
    fprintf(stdout, "              | %f %f |, new: | %f %f |\n",
            oldNet.hiddenToOutputWeights.m[2], oldNet.hiddenToOutputWeights.m[3],
            newNet.h2oWeights[2], newNet.h2oWeights[3]);
    fprintf(stdout, "              | %f %f |, new: | %f %f |\n",
            oldNet.hiddenToOutputWeights.m[4], oldNet.hiddenToOutputWeights.m[5],
            newNet.h2oWeights[4], newNet.h2oWeights[5]);
    
    fprintf(stdout, "Bias Orig: | %f %f |, new: | %f %f |\n",
            oldNet.outputBias.m[0], oldNet.outputBias.m[1],
            newNet.outputBias[0], newNet.outputBias[1]);
    
    //
    // NOTE(michiel): Old propagate errors
    //
    
    fprintf(stdout, "Weights Orig: | % 5.2f % 5.2f % 5.2f |, new: | % 5.2f % 5.2f % 5.2f |\n",
            oldNet.inputToHiddenWeights.m[0], oldNet.inputToHiddenWeights.m[1],
            oldNet.inputToHiddenWeights.m[2],
            newNet.i2hWeights[0], newNet.i2hWeights[1], newNet.i2hWeights[2]);
    fprintf(stdout, "              | % 5.2f % 5.2f % 5.2f |, new: | % 5.2f % 5.2f % 5.2f |\n",
            oldNet.inputToHiddenWeights.m[3], oldNet.inputToHiddenWeights.m[4],
            oldNet.inputToHiddenWeights.m[5],
            newNet.i2hWeights[3], newNet.i2hWeights[4], newNet.i2hWeights[5]);
    fprintf(stdout, "              | % 5.2f % 5.2f % 5.2f |, new: | % 5.2f % 5.2f % 5.2f |\n",
            oldNet.inputToHiddenWeights.m[6], oldNet.inputToHiddenWeights.m[7],
            oldNet.inputToHiddenWeights.m[8],
            newNet.i2hWeights[6], newNet.i2hWeights[7], newNet.i2hWeights[8]);
    fprintf(stdout, "              | % 5.2f % 5.2f % 5.2f |, new: | % 5.2f % 5.2f % 5.2f |\n",
            oldNet.inputToHiddenWeights.m[9], oldNet.inputToHiddenWeights.m[10],
            oldNet.inputToHiddenWeights.m[11],
            newNet.i2hWeights[9], newNet.i2hWeights[10], newNet.i2hWeights[11]);
    
    // NOTE(michiel): Blame hidden layers
    Matrix weightsHTranspose = transpose(oldNet.hiddenToOutputWeights);
    Matrix hiddenErrors = weightsHTranspose * outputErrors;
    i_expect(hiddenErrors.columns == 1);
    
    // NOTE(michiel): Calculate hidden gradient
    Matrix hiddenGradients = {};
    init_matrix(&hiddenGradients, oldNet.hidden.rows, oldNet.hidden.columns);
    matrix_copy(hiddenGradients.rows, hiddenGradients.columns, oldNet.hidden.m,
                hiddenGradients.m);
    map(&hiddenGradients, dsigmoid);
    hadamard(hiddenErrors, &hiddenGradients);
    hiddenGradients *= oldNet.learningRate;
    
    // NOTE(michiel): Calculate hidden weight deltas
    Matrix inputT = transpose(matrix_from_array(inputCount, 1, inputs));
    Matrix weightsIDeltas = hiddenGradients * inputT;
    
    // NOTE(michiel): Add the deltas
    oldNet.hiddenBias += hiddenGradients;
    oldNet.inputToHiddenWeights += weightsIDeltas;
    fprintf(stdout, "orig hidden weights: r=%u, c=%u\n", oldNet.inputToHiddenWeights.rows,
            oldNet.inputToHiddenWeights.columns);
    fprintf(stdout, "new hidden weights: r=%u, c=%u\n", newNet.hiddenCount[0],
            newNet.inputCount);
    
    //
    // NOTE(michiel): New propagate errors
    //
    
    transpose_multiply(newNet.hiddenCount[0], outputCount, newNet.h2oWeights, 
                       currentErrors, nextErrors);
    calculate_gradient(newNet.hiddenCount[0], newNet.hidden, nextErrors,
                       newNet.trainGradient, learningRate);
    add_to_array(newNet.hiddenCount[0], newNet.trainGradient, newNet.hiddenBias);
    add_weights(newNet.hiddenCount[0], newNet.inputCount, newNet.trainGradient,
                inputs, newNet.i2hWeights);
    
    fprintf(stdout, "Weights Orig: | % 5.2f % 5.2f % 5.2f |, new: | % 5.2f % 5.2f % 5.2f |\n",
            oldNet.inputToHiddenWeights.m[0], oldNet.inputToHiddenWeights.m[1],
            oldNet.inputToHiddenWeights.m[2],
            newNet.i2hWeights[0], newNet.i2hWeights[1], newNet.i2hWeights[2]);
    fprintf(stdout, "              | % 5.2f % 5.2f % 5.2f |, new: | % 5.2f % 5.2f % 5.2f |\n",
            oldNet.inputToHiddenWeights.m[3], oldNet.inputToHiddenWeights.m[4],
            oldNet.inputToHiddenWeights.m[5],
            newNet.i2hWeights[3], newNet.i2hWeights[4], newNet.i2hWeights[5]);
    fprintf(stdout, "              | % 5.2f % 5.2f % 5.2f |, new: | % 5.2f % 5.2f % 5.2f |\n",
            oldNet.inputToHiddenWeights.m[6], oldNet.inputToHiddenWeights.m[7],
            oldNet.inputToHiddenWeights.m[8],
            newNet.i2hWeights[6], newNet.i2hWeights[7], newNet.i2hWeights[8]);
    fprintf(stdout, "              | % 5.2f % 5.2f % 5.2f |, new: | % 5.2f % 5.2f % 5.2f |\n",
            oldNet.inputToHiddenWeights.m[9], oldNet.inputToHiddenWeights.m[10],
            oldNet.inputToHiddenWeights.m[11],
            newNet.i2hWeights[9], newNet.i2hWeights[10], newNet.i2hWeights[11]);
    
    fprintf(stdout, "Bias Orig: | %f %f %f |, new: | %f %f %f |\n",
            oldNet.hiddenBias.m[0], oldNet.hiddenBias.m[1], oldNet.hiddenBias.m[2],
            newNet.hiddenBias[0], newNet.hiddenBias[1], newNet.hiddenBias[2]);
    
    #endif

    
    return 0;
}