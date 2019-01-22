#include <stdio.h>
#include <stdlib.h>

#include "../libberdip/platform.h"
#include "../src/matrix.h"

MATRIX_MAP(sigmoid)
{
    f32 result = 0;
    result = 1.0f / (1.0f + exp(-a));
    return result;
}

MATRIX_MAP(dsigmoid)
{
    f32 result = 0;
    result = a * (1.0f - a);
    return result;
}


internal void
predict_layer_old(u32 inCount, u32 outCount,
             f32 *inputs, f32 *weights, f32 *bias, f32 *outputs)
{
    u32 rowOut = outCount;
    u32 colOut = 1;
    u32 rowWeight = outCount;
    u32 colWeight = inCount;
    matrix_multiply_matrix(rowWeight, colWeight, 1, weights, inputs, outputs);
    matrix_add_matrix(rowOut, colOut, bias, outputs);
    matrix_map(rowOut, colOut, outputs, sigmoid);
}

internal void
predict_layer(u32 inCount, u32 outCount, 
              f32 *inputs, f32 *weights, f32 *bias, f32 *outputs)
{
    // O = sigmoid(W * I + B);
    for (u32 row = 0; row < outCount; ++row)
    {
        f32 *rowW = weights + row * inCount;
        f32 *rowO = outputs + row;
        *rowO = 0.0f;
        // NOTE(michiel): Weights
        for (u32 col = 0; col < inCount; ++col)
        {
            *rowO += rowW[col] * inputs[col];
        }
        // NOTE(michiel): Bias
        *rowO += bias[row];
        
        // NOTE(michiel): Sigmoid
        *rowO = 1.0f / (1.0f + exp(-*rowO));
    }
}


internal void
calculate_gradient_old(u32 count, f32 *inputs, f32 *errors, f32 *result, f32 learningRate)
{
// NOTE(michiel): Calculate output gradient
    u32 rowGrad = count;
    u32 colGrad = 1;
    
    matrix_copy(rowGrad, colGrad, inputs, result);
matrix_map(rowGrad, colGrad, result, dsigmoid);
    matrix_hadamard_matrix(rowGrad, colGrad, errors, result);
    matrix_multiply_scalar(rowGrad, colGrad, result, learningRate);
}

internal void
calculate_gradient(u32 count, f32 *inputs, f32 *errors, f32 *result, f32 learningRate)
{
    for (u32 index = 0; index < count; ++index)
    {
        result[index] = errors[index] * (inputs[index] * (1.0f - inputs[index])) * learningRate;
    }
}

internal void
add_weights_old(u32 multCount, u32 transpCount, f32 *multiplier, f32 *transposer, f32 *weights)
{
    u32 rowTrans = 1;
    u32 colTrans = transpCount;
    
    f32 *tempArray = allocate_array(f32, transpCount);
    f32 *tempWeights = allocate_array(f32, multCount * colTrans);
    
    matrix_transpose(colTrans, rowTrans, transposer, tempArray);
    matrix_multiply_matrix(multCount, 1, colTrans, multiplier, tempArray, tempWeights);
    matrix_add_matrix(multCount, colTrans, tempWeights, weights);
    
    deallocate(tempWeights);
    deallocate(tempArray);
    //deallocate_array(multCount * colTrans, tempWeights);
    //deallocate_array(transpCount, tempArray);
}

internal void
add_weights(u32 multCount, u32 transpCount, f32 *multiplier, f32 *transposer, f32 *weights)
{
    // weights += multiplier * transpose(transposer) (matrix multiply of 2 vectors)
    
    for (u32 row = 0; row < multCount; ++row)
    {
        f32 rowM = multiplier[row];
        f32 *rowW = weights + row * transpCount;
        for (u32 col = 0; col < transpCount; ++col)
        {
            f32 colT = transposer[col];
            rowW[col] += rowM * colT;
        }
    }
}

internal void
transpose_multiply_old(u32 outputCount, u32 inputCount, f32 *transposedM, f32 *current, f32 *next)
{
    u32 rowTrans = inputCount;
    u32 colTrans = outputCount;
    
    //f32 *tempArray = allocate_array(f32, Count);
    f32 *tempWeights = allocate_array(f32, rowTrans * colTrans);
    matrix_transpose(rowTrans, colTrans, transposedM, tempWeights);
            matrix_multiply_matrix(colTrans, rowTrans, 1, tempWeights, current, next);
    
    deallocate(tempWeights);
    //deallocate_array(rowTrans * colTrans, tempWeights);
}

internal void
transpose_multiply(u32 outputCount, u32 inputCount, f32 *transposedM, f32 *current, f32 *next)
{
    // next = transpose(transposedM) * current (transposedM = weightsMatrix)
    
    f32 *colC = current;
    for (u32 row = 0; row < outputCount; ++row)
    {
        f32 *rowT = transposedM + row;
        f32 *rowN = next + row;
        *rowN = 0.0f;
        for (u32 s = 0; s < inputCount; ++s)
        {
            *rowN += rowT[s * outputCount] * colC[s];
        }
    }
}

int main(int argc, char **argv)
{
    f32 inputs[3] = {0.1f, 0.4f, 0.2f};
    u32 inputCount = 3;
    
    f32 weights[3 * 2] = {0.2f, 0.1f, 0.1f, 0.2f, 0.3f, 0.4f};
    
    f32 outputs[2] = {0.0f, 0.0f};
    u32 outputCount = 2;
    
    f32 bias[2] = {-0.2f, 0.1f};
    
    predict_layer_old(inputCount, outputCount, inputs, weights, bias, outputs);
    f32 a = outputs[0];
    
    predict_layer(inputCount, outputCount, inputs, weights, bias, outputs);
    f32 b = outputs[0];
    
    fprintf(stdout, "Predict orig: %f, new: %f\n", a, b);
    
f32 errors[2] = {0.1f, -0.2f};
    f32 gradientOld[2] = {};
    f32 gradientNew[2] = {};
    calculate_gradient_old(outputCount, outputs, errors, gradientOld, 0.1f);
    calculate_gradient(outputCount, outputs, errors, gradientNew, 0.1f);
    
    fprintf(stdout, "Gradient orig: %f, new: %f\n", gradientOld[0], gradientNew[0]);
    
    f32 weightsOld[3 * 2] = 
    {
        0.2f, 0.1f, 0.3f,
        0.4f, 0.5f, 0.4f,
    };
    f32 weightsNew[3 * 2] = 
    {
        0.2f, 0.1f, 0.3f,
        0.4f, 0.5f, 0.4f,
    };
    add_weights_old(outputCount, inputCount, gradientOld, inputs, weightsOld);
    add_weights(outputCount, inputCount, gradientNew, inputs, weightsNew);
    
    fprintf(stdout, "Weights orig: | %f %f %f |, new: | %f %f %f |\n", 
            weightsOld[0], weightsOld[1], weightsOld[2],
            weightsNew[0], weightsNew[1], weightsNew[2]);
    fprintf(stdout, "Weights orig: | %f %f %f |, new: | %f %f %f |\n", 
            weightsOld[3], weightsOld[4], weightsOld[5],
            weightsNew[3], weightsNew[4], weightsNew[5]);
    
    f32 errorsOld[3] = {};
    f32 errorsNew[3] = {};
    transpose_multiply_old(inputCount, outputCount, weightsOld, errors, errorsOld);
    transpose_multiply(inputCount, outputCount, weightsNew, errors, errorsNew);
    
    fprintf(stdout, "Errors orig: | %f %f %f |, new: | %f %f %f |\n", 
            errorsOld[0], errorsOld[1], errorsOld[2],
            errorsNew[0], errorsNew[1], errorsNew[2]);
    
    }