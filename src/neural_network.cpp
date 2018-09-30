#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "random.h"
#include "matrix.h"
#include "drawing.cpp"

struct Matrix
{
    u32 rows;
    u32 columns;
    f32 *m;
};

internal void
init_matrix(Matrix *matrix, u32 rows, u32 columns)
{
    matrix->rows = rows;
    matrix->columns = columns;
    i_expect(matrix->m == 0);
    matrix->m = allocate_array(f32, rows * columns);
}

internal Matrix
matrix_from_array(u32 rows, u32 columns, f32 *data)
{
    Matrix result = {};
    result.rows = rows;
    result.columns = columns;
    result.m = data;
    return result;
}

internal void
destroy_matrix(Matrix *matrix)
{
    if (matrix->m)
    {
    deallocate_array(matrix->rows * matrix->columns, matrix->m);
    }
}

internal void
randomize_matrix(RandomSeriesPCG *random, Matrix *matrix)
{
    for (u32 row = 0; row < matrix->rows; ++row)
    {
        f32 *r = matrix->m + row * matrix->columns;
        for (u32 col = 0; col < matrix->columns; ++col)
        {
            r[col] = random_bilateral(random);
        }
    }
}

internal inline Matrix &
operator +=(Matrix &m, f32 s)
{
    matrix_add_scalar(m.rows, m.columns, m.m, s);
    return m;
}

internal inline Matrix &
operator *=(Matrix &m, f32 s)
{
    matrix_multiply_scalar(m.rows, m.columns, m.m, s);
    return m;
}

internal inline Matrix &
operator +=(Matrix &a, Matrix b)
{
    i_expect(a.rows == b.rows);
    i_expect(a.columns == b.columns);
    matrix_add_matrix(a.rows, a.columns, b.m, a.m);
    return a;
}

internal inline Matrix
operator -(Matrix a, Matrix b)
{
    i_expect(a.rows == b.rows);
    i_expect(a.columns == b.columns);
    Matrix result = {};
    init_matrix(&result, a.rows, a.columns);
    matrix_copy(result.rows, result.columns, a.m, result.m);
    matrix_sub_matrix(result.rows, result.columns, b.m, result.m);
    return result;
}

internal inline Matrix
operator *(Matrix a, Matrix b)
{
    i_expect(a.columns == b.rows);
    Matrix result = {};
    init_matrix(&result, a.rows, b.columns);
    matrix_multiply_matrix(a.rows, a.columns, b.columns, a.m, b.m, result.m);
    return result;
}

internal inline void
hadamard(Matrix factor, Matrix *dest)
{
    i_expect(factor.rows == dest->rows);
    i_expect(factor.columns == dest->columns);
    matrix_hadamard_matrix(dest->rows, dest->columns, factor.m, dest->m);
}

internal inline Matrix
transpose(Matrix m)
{
    Matrix result = {};
    init_matrix(&result, m.columns, m.rows);
    matrix_transpose(m.rows, m.columns, m.m, result.m);
    return result;
}

internal void
map(Matrix *m, MatrixMap *map)
{
    matrix_map(m->rows, m->columns, m->m, map);
}

MATRIX_MAP(signoid)
{
    f32 result = 0;
    result = 1.0f / (1.0f + exp(-a));
    return result;
}

struct NeuralNetwork
{
    u32 inputCount;
    u32 hiddenCount;
    u32 outputCount;
    
    f32 learningRate;
    
    Matrix inputToHiddenWeights;
    Matrix hiddenToOutputWeights;
    Matrix hiddenBias;
    Matrix outputBias;
    
    Matrix hidden;
    Matrix outputs;
    };

internal void
init_neural_network(RandomSeriesPCG *random, NeuralNetwork *network, 
                    u32 inputs, u32 hidden, u32 outputs, f32 learningRate = 1.0f)
{
    network->inputCount = inputs;
    network->hiddenCount = hidden;
    network->outputCount = outputs;
    
    network->learningRate = learningRate;
    
    i_expect(network->inputToHiddenWeights.m == 0);
    i_expect(network->hiddenToOutputWeights.m == 0);
    i_expect(network->hiddenBias.m == 0);
    i_expect(network->outputBias.m == 0);
    i_expect(network->hidden.m == 0);
    i_expect(network->outputs.m == 0);
    
    init_matrix(&network->inputToHiddenWeights, hidden, inputs);
    init_matrix(&network->hiddenToOutputWeights, outputs, hidden);
    randomize_matrix(random, &network->inputToHiddenWeights);
    randomize_matrix(random, &network->hiddenToOutputWeights);
    
    init_matrix(&network->hiddenBias, hidden, 1);
    init_matrix(&network->outputBias, outputs, 1);
    randomize_matrix(random, &network->hiddenBias);
    randomize_matrix(random, &network->outputBias);
    
    init_matrix(&network->hidden, hidden, 1);
    init_matrix(&network->outputs, outputs, 1);
    }

internal void
feed_forward(NeuralNetwork *network, u32 inputCount, f32 *inputs)
{
    i_expect(inputCount == network->inputCount);
    Matrix input = matrix_from_array(inputCount, 1, inputs);
    
    destroy_matrix(&network->hidden);
     network->hidden = network->inputToHiddenWeights * input;
    network->hidden += network->hiddenBias;
    map(&network->hidden, signoid);
    
        destroy_matrix(&network->outputs);
    network->outputs = network->hiddenToOutputWeights * network->hidden;
    network->outputs += network->outputBias;
    map(&network->outputs, signoid);
    
    i_expect(network->outputs.rows == network->outputCount);
    i_expect(network->outputs.columns == 1);
}

MATRIX_MAP(dsigmoid)
{
    f32 result = 0;
    result = a * (1.0f - a);
    return result;
}

internal void
train(NeuralNetwork *network, u32 inputCount, f32 *inputs,
      u32 answerCount, f32 *answers)
{
    i_expect(network->inputCount == inputCount);
    i_expect(network->outputCount == answerCount);
    
    feed_forward(network, inputCount, inputs);
    
    Matrix answer = matrix_from_array(answerCount, 1, answers);
    
    // NOTE(michiel): Calculate errors
    Matrix outputErrors = answer - network->outputs;
    i_expect(outputErrors.columns == 1);
    
#if 0    
    // NOTE(michiel): Blame inputs
    Matrix weightsITranspose = transpose(network->inputToHiddenWeights);
    Matrix inputErrors = weightsITranspose * hiddenErrors;
    #endif

    // NOTE(michiel): Calculate output gradient
    Matrix gradients = {};
    init_matrix(&gradients, network->outputs.rows, network->outputs.columns);
    matrix_copy(gradients.rows, gradients.columns, network->outputs.m, gradients.m);
    map(&gradients, dsigmoid);
    hadamard(outputErrors, &gradients);
    gradients *= network->learningRate;
    
    // NOTE(michiel): Calculate hidden weight deltas
    Matrix hiddenT = transpose(network->hidden);
    Matrix weightsHDeltas = gradients * hiddenT;
    
    // NOTE(michiel): Add the deltas
    network->outputBias += gradients;
    network->hiddenToOutputWeights += weightsHDeltas;
    
    
    // NOTE(michiel): Blame hidden layers
    Matrix weightsHTranspose = transpose(network->hiddenToOutputWeights);
    Matrix hiddenErrors = weightsHTranspose * outputErrors;
    i_expect(hiddenErrors.columns == 1);
    
    // NOTE(michiel): Calculate hidden gradient
    Matrix hiddenGradients = {};
    init_matrix(&hiddenGradients, network->hidden.rows, network->hidden.columns);
    matrix_copy(hiddenGradients.rows, hiddenGradients.columns, network->hidden.m,
                hiddenGradients.m);
    map(&hiddenGradients, dsigmoid);
    hadamard(hiddenErrors, &hiddenGradients);
    hiddenGradients *= network->learningRate;
    
    // NOTE(michiel): Calculate hidden weight deltas
    Matrix inputT = transpose(matrix_from_array(inputCount, 1, inputs));
    Matrix weightsIDeltas = hiddenGradients * inputT;
    
    // NOTE(michiel): Add the deltas
    network->hiddenBias += hiddenGradients;
    network->inputToHiddenWeights += weightsIDeltas;
    
    
    destroy_matrix(&weightsIDeltas);
    destroy_matrix(&inputT);
    destroy_matrix(&hiddenGradients);
    
    destroy_matrix(&weightsHDeltas);
    destroy_matrix(&hiddenT);
    destroy_matrix(&gradients);

#if 0    
    destroy_matrix(&inputErrors);
    destroy_matrix(&weightsITranspose);
    #endif

    destroy_matrix(&hiddenErrors);
    destroy_matrix(&weightsHTranspose);
    
    destroy_matrix(&outputErrors);
}

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
