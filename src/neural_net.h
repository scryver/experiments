// TODO(michiel): Ugly class, please remove as is (no hidden allocations!)
struct Matrix
{
    u32 rows;
    u32 columns;
    f32 *m;
};

internal Matrix
create_matrix(u32 rows, u32 columns)
{
    Matrix result;
    result.rows = rows;
    result.columns = columns;
    result.m = allocate_array(f32, rows * columns);
    return result;
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

internal inline Matrix &
operator -=(Matrix &a, Matrix b)
{
    i_expect(a.rows == b.rows);
    i_expect(a.columns == b.columns);
    matrix_sub_matrix(a.rows, a.columns, b.m, a.m);
    return a;
}

internal inline void
multiply(Matrix a, Matrix b, Matrix *result)
{
    i_expect(a.columns == b.rows);
    i_expect(a.rows == result->rows);
    i_expect(b.columns == result->columns);
    matrix_multiply_matrix(a.rows, a.columns, b.columns, a.m, b.m, result->m);
}

internal inline void
hadamard(Matrix factor, Matrix *dest)
{
    i_expect(factor.rows == dest->rows);
    i_expect(factor.columns == dest->columns);
    matrix_hadamard_matrix(dest->rows, dest->columns, factor.m, dest->m);
}

internal inline void
transpose(Matrix orig, Matrix *transposed)
{
    i_expect(orig.rows == transposed->columns);
    i_expect(orig.columns == transposed->rows);
    matrix_transpose(orig.rows, orig.columns, orig.m, transposed->m);
}

internal void
map(Matrix *m, MatrixMap *map)
{
    matrix_map(m->rows, m->columns, m->m, map);
}

MATRIX_MAP(sigmoid)
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
    
    network->inputToHiddenWeights = create_matrix(hidden, inputs);
    network->hiddenToOutputWeights = create_matrix(outputs, hidden);
    randomize_matrix(random, &network->inputToHiddenWeights);
    randomize_matrix(random, &network->hiddenToOutputWeights);
    
    network->hiddenBias = create_matrix(hidden, 1);
    network->outputBias = create_matrix(outputs, 1);
    randomize_matrix(random, &network->hiddenBias);
    randomize_matrix(random, &network->outputBias);
    
    network->hidden = create_matrix(hidden, 1);
    network->outputs = create_matrix(outputs, 1);
}

internal void
feed_forward(NeuralNetwork *network, u32 inputCount, f32 *inputs)
{
    TempMemory temp = temporary_memory();
    
    i_expect(inputCount == network->inputCount);
    Matrix input = matrix_from_array(inputCount, 1, inputs);
    
    multiply(network->inputToHiddenWeights, input, &network->hidden);
    network->hidden += network->hiddenBias;
    map(&network->hidden, sigmoid);
    
    multiply(network->hiddenToOutputWeights, network->hidden, &network->outputs);
    network->outputs += network->outputBias;
    map(&network->outputs, sigmoid);
    
    i_expect(network->outputs.rows == network->outputCount);
    i_expect(network->outputs.columns == 1);
    
    destroy_temporary(temp);
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
    TempMemory temp = temporary_memory();
    
    i_expect(network->inputCount == inputCount);
    i_expect(network->outputCount == answerCount);
    
    feed_forward(network, inputCount, inputs);
    
    Matrix answer = matrix_from_array(answerCount, 1, answers);
    
    // NOTE(michiel): Calculate errors
    Matrix outputErrors = create_matrix(answer.rows, answer.columns);
    matrix_copy(answer.rows, answer.columns, answer.m, outputErrors.m);
    outputErrors -= network->outputs;
    i_expect(outputErrors.columns == 1);
    
#if 0    
    // NOTE(michiel): Blame inputs
    Matrix weightsITranspose = transpose(network->inputToHiddenWeights);
    Matrix inputErrors = weightsITranspose * hiddenErrors;
#endif
    
    // NOTE(michiel): Calculate output gradient
    Matrix gradients = create_matrix(network->outputs.rows, network->outputs.columns);
    matrix_copy(gradients.rows, gradients.columns, network->outputs.m, gradients.m);
    map(&gradients, dsigmoid);
    hadamard(outputErrors, &gradients);
    gradients *= network->learningRate;
    
    // NOTE(michiel): Calculate hidden weight deltas
    Matrix hiddenT = create_matrix(network->hidden.columns, network->hidden.rows);
        transpose(network->hidden, &hiddenT);
    Matrix weightsHDeltas = create_matrix(gradients.rows, hiddenT.columns);
    multiply(gradients, hiddenT, &weightsHDeltas);
    
    // NOTE(michiel): Add the deltas
    network->outputBias += gradients;
    network->hiddenToOutputWeights += weightsHDeltas;
    
    
    // NOTE(michiel): Blame hidden layers
    Matrix weightsHTranspose = create_matrix(network->hiddenToOutputWeights.columns, network->hiddenToOutputWeights.rows);
    transpose(network->hiddenToOutputWeights, &weightsHTranspose);
    Matrix hiddenErrors = create_matrix(weightsHTranspose.rows, outputErrors.columns);
    multiply(weightsHTranspose, outputErrors, &hiddenErrors);
    i_expect(hiddenErrors.columns == 1);
    
    // NOTE(michiel): Calculate hidden gradient
    Matrix hiddenGradients = create_matrix(network->hidden.rows, network->hidden.columns);
    matrix_copy(hiddenGradients.rows, hiddenGradients.columns, network->hidden.m,
                hiddenGradients.m);
    map(&hiddenGradients, dsigmoid);
    hadamard(hiddenErrors, &hiddenGradients);
    hiddenGradients *= network->learningRate;
    
    // NOTE(michiel): Calculate hidden weight deltas
    Matrix inputT = create_matrix(1, inputCount);
    transpose(matrix_from_array(inputCount, 1, inputs), &inputT);
    Matrix weightsIDeltas = create_matrix(hiddenGradients.rows, inputT.columns);
    multiply(hiddenGradients, inputT, &weightsIDeltas);
    
    // NOTE(michiel): Add the deltas
    network->hiddenBias += hiddenGradients;
    network->inputToHiddenWeights += weightsIDeltas;
    
    destroy_temporary(temp);
}
