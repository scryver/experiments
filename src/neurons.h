#define NEURON_MUTATE(name) f32 name(f32 a, void *user)
typedef NEURON_MUTATE(NeuronMutate);

enum ArrayKind
{
    Array_Float,
    Array_UInt,
};
struct Array
{
    enum32(ArrayKind) kind;
    u32 count;
    union {
        f32 *f;
        u32 *u;
    };
};

struct Neural
{
    u32 inputCount;
    
#if 0    
    Array layerCount;
    Array layers;
    Array biases;
    Array weights;
#endif
    
    u32 hiddenTotal;
    u32 hiddenDepth;
    u32 *hiddenCount;
    f32 *hidden;
    f32 *hiddenBias;
    f32 *i2hWeights;
    f32 *h2hWeights;
    
    u32 outputCount;
    f32 *outputs;
    f32 *outputBias;
    f32 *h2oWeights;
    
    f32 *trainErrorsA;
    f32 *trainErrorsB;
    f32 *trainGradient;
};

internal void
init_neural_network(Neural *network, u32 inputCount, 
                    u32 hiddenDepth, u32 *hiddenCounts, u32 outputCount)
{
    network->inputCount = inputCount; // NOTE(michiel): For verification mostly
    
#if 0    
    u32 layerCount = 1 + hiddenDepth + 1; // NOTE(michiel): input + hidden + output
    network->layerCount.kind = Array_UInt;
    network->layerCount.size = layerCount;
    i_expect(network->layerCount.u == 0);
    network->layerCount.u = allocate_array(u32, layerCount);
    for (u32 layerIndex = 0; layerIndex < network->layerCount.size; ++layerIndex)
    {
        if (layerIndex == 0)
        {
            network->layerCount.u[layerIndex] = inputCount;
        }
        else if (layerIndex == network->layerCount.size - 1)
        {
            network->layerCount.u[layerIndex] = outputCount;
        }
        else
        {
            network->layerCount.u[layerIndex] = hiddenCounts[layerIndex - 1];
        }
    }
#endif
    
    i_expect(network->hiddenCount == 0);
    i_expect(network->hidden == 0);
    i_expect(network->hiddenBias == 0);
    i_expect(network->i2hWeights == 0);
    i_expect(network->outputs == 0);
    i_expect(network->outputBias == 0);
    i_expect(network->h2oWeights == 0);
    
    network->hiddenDepth = hiddenDepth;
    network->hiddenCount = allocate_array(u32, hiddenDepth);
    network->hiddenTotal = 0;
    for (u32 hIndex = 0; hIndex < hiddenDepth; ++hIndex)
    {
        u32 count = hiddenCounts[hIndex];
        network->hiddenCount[hIndex] = count;
        network->hiddenTotal += count;
    }
    network->hidden = allocate_array(f32, network->hiddenTotal);
    network->hiddenBias = allocate_array(f32, network->hiddenTotal);
    network->i2hWeights = allocate_array(f32, network->hiddenCount[0] * inputCount);
    
    u32 maxErrorLen = network->hiddenCount[0];
    u32 h2hWeightLen = 0;
    for (u32 i = 1; i < hiddenDepth; ++i)
    {
        h2hWeightLen += network->hiddenCount[i] * network->hiddenCount[i - 1];
        if (maxErrorLen < network->hiddenCount[i])
        {
            maxErrorLen = network->hiddenCount[i];
        }
    }
    network->h2hWeights = allocate_array(f32, h2hWeightLen);
    
    network->outputCount = outputCount;
    network->outputs = allocate_array(f32, outputCount);
    network->outputBias = allocate_array(f32, outputCount);
    network->h2oWeights = allocate_array(f32, outputCount * network->hiddenCount[hiddenDepth - 1]);
    
    if (maxErrorLen < outputCount)
    {
        maxErrorLen = outputCount;
    }
    network->trainErrorsA = allocate_array(f32, maxErrorLen);
    network->trainErrorsB = allocate_array(f32, maxErrorLen);
    network->trainGradient = allocate_array(f32, maxErrorLen);
}

internal void
neural_copy(Neural *source, Neural *dest)
{
    i_expect(source->inputCount == dest->inputCount);
    i_expect(source->hiddenDepth == dest->hiddenDepth);
    i_expect(source->hiddenCount[0] == dest->hiddenCount[0]);
    i_expect(source->outputCount == dest->outputCount);
    
    for (u32 hIndex = 0; hIndex < source->hiddenCount[0] * source->inputCount; ++hIndex)
    {
        dest->i2hWeights[hIndex] = source->i2hWeights[hIndex];
    }
    u32 h2hOffset = 0;
    for (u32 i = 1; i < source->hiddenDepth; ++i)
    {
        i_expect(source->hiddenCount[i] == dest->hiddenCount[i]);
        u32 h2hWidth = source->hiddenCount[i] * source->hiddenCount[i - 1];
        for (u32 hIndex = 0; hIndex < h2hWidth; ++hIndex)
        {
            dest->h2hWeights[hIndex + h2hOffset] = source->h2hWeights[hIndex + h2hOffset];
        }
        h2hOffset += h2hWidth;
    }
    for (u32 hIndex = 0; hIndex < source->hiddenTotal; ++hIndex)
    {
        dest->hiddenBias[hIndex] = source->hiddenBias[hIndex];
    }
    for (u32 oIndex = 0;
         oIndex < source->outputCount * source->hiddenCount[source->hiddenDepth - 1];
         ++oIndex)
    {
        dest->h2oWeights[oIndex] = source->h2oWeights[oIndex];
    }
    for (u32 oIndex = 0; oIndex < source->outputCount; ++oIndex)
    {
        dest->outputBias[oIndex] = source->outputBias[oIndex];
    }
}

internal void
randomize_weights(RandomSeriesPCG *random, Neural *network)
{
    for (u32 hIndex = 0; hIndex < network->hiddenCount[0] * network->inputCount; ++hIndex)
    {
        network->i2hWeights[hIndex] = random_bilateral(random);
    }
    u32 h2hOffset = 0;
        for (u32 i = 1; i < network->hiddenDepth; ++i)
    {
        u32 h2hWidth = network->hiddenCount[i] * network->hiddenCount[i - 1];
        for (u32 hIndex = 0; hIndex < h2hWidth; ++hIndex)
        {
            network->h2hWeights[hIndex + h2hOffset] = random_bilateral(random);
        }
        h2hOffset += h2hWidth;
    }
    for (u32 hIndex = 0; hIndex < network->hiddenTotal; ++hIndex)
    {
        network->hiddenBias[hIndex] = random_bilateral(random);
    }
    for (u32 oIndex = 0;
         oIndex < network->outputCount * network->hiddenCount[network->hiddenDepth - 1];
         ++oIndex)
    {
        network->h2oWeights[oIndex] = random_bilateral(random);
    }
    for (u32 oIndex = 0; oIndex < network->outputCount; ++oIndex)
    {
        network->outputBias[oIndex] = random_bilateral(random);
    }
}

internal void
neural_mixin(RandomSeriesPCG *random, Neural *source, Neural *dest, f32 rate = 0.5f)
{
    i_expect(source->inputCount == dest->inputCount);
    i_expect(source->hiddenDepth == dest->hiddenDepth);
    i_expect(source->hiddenCount[0] == dest->hiddenCount[0]);
    i_expect(source->outputCount == dest->outputCount);
    
    for (u32 hIndex = 0; hIndex < source->hiddenCount[0] * source->inputCount; ++hIndex)
    {
        if (random_unilateral(random) < rate)
        {
        dest->i2hWeights[hIndex] = source->i2hWeights[hIndex];
        }
    }
    u32 h2hOffset = 0;
    for (u32 i = 1; i < source->hiddenDepth; ++i)
    {
        i_expect(source->hiddenCount[i] == dest->hiddenCount[i]);
        u32 h2hWidth = source->hiddenCount[i] * source->hiddenCount[i - 1];
        for (u32 hIndex = 0; hIndex < h2hWidth; ++hIndex)
        {
            if (random_unilateral(random) < rate)
            {
            dest->h2hWeights[hIndex + h2hOffset] = source->h2hWeights[hIndex + h2hOffset];
            }
        }
        h2hOffset += h2hWidth;
    }
    for (u32 hIndex = 0; hIndex < source->hiddenTotal; ++hIndex)
    {
        if (random_unilateral(random) < rate)
        {
        dest->hiddenBias[hIndex] = source->hiddenBias[hIndex];
        }
    }
    for (u32 oIndex = 0;
         oIndex < source->outputCount * source->hiddenCount[source->hiddenDepth - 1];
         ++oIndex)
    {
        if (random_unilateral(random) < rate)
        {
        dest->h2oWeights[oIndex] = source->h2oWeights[oIndex];
        }
    }
    for (u32 oIndex = 0; oIndex < source->outputCount; ++oIndex)
    {
        if (random_unilateral(random) < rate)
        {
        dest->outputBias[oIndex] = source->outputBias[oIndex];
        }
    }
}

internal void
neural_mutate(RandomSeriesPCG *random, Neural *network, NeuronMutate *mutateFunc,
              void *user = 0)
{
    for (u32 hIndex = 0; hIndex < network->hiddenCount[0] * network->inputCount; ++hIndex)
    {
            network->i2hWeights[hIndex] = mutateFunc(network->i2hWeights[hIndex], user);
    }
    u32 h2hOffset = 0;
    for (u32 i = 1; i < network->hiddenDepth; ++i)
    {
        u32 h2hWidth = network->hiddenCount[i] * network->hiddenCount[i - 1];
        for (u32 hIndex = 0; hIndex < h2hWidth; ++hIndex)
        {
                network->h2hWeights[hIndex] = mutateFunc(network->h2hWeights[hIndex], user);
            }
        h2hOffset += h2hWidth;
    }
    for (u32 hIndex = 0; hIndex < network->hiddenTotal; ++hIndex)
    {
        network->hiddenBias[hIndex] = mutateFunc(network->hiddenBias[hIndex], user);
    }
    for (u32 oIndex = 0;
         oIndex < network->outputCount * network->hiddenCount[network->hiddenDepth - 1];
         ++oIndex)
    {
        network->h2oWeights[oIndex] = mutateFunc(network->h2oWeights[oIndex], user);
    }
    for (u32 oIndex = 0; oIndex < network->outputCount; ++oIndex)
    {
        network->outputBias[oIndex] = mutateFunc(network->outputBias[oIndex], user);
    }
}

internal inline f32
activation_function(f32 inp)
{
    return 1.0f / (1.0f + exp(-inp));
}

internal inline f32
delta_activation_function(f32 inp)
{
    return inp * (1.0f - inp);
}

internal void
predict_layer(u32 inCount, u32 outCount, f32 *inputs, f32 *weights, f32 *bias, f32 *outputs)
{
    // O = sigmoid(W * I + B);
    f32 *W = weights;
    for (u32 row = 0; row < outCount; ++row)
    {
        f32 result = 0.0f;
        // NOTE(michiel): Weights
        for (u32 col = 0; col < inCount; ++col)
        {
            result += (*W++) * inputs[col];
        }
        // NOTE(michiel): Bias
        result += bias[row];
        
        // NOTE(michiel): Sigmoid and store
        outputs[row] = activation_function(result);
        }
}

internal void
predict(Neural *network, u32 inputCount, f32 *inputs)
{
    i_expect(inputCount == network->inputCount);
    
    u32 hiddenOffset = 0;
    u32 h2hOffset = 0;
    for (u32 layerIndex = 0; layerIndex < network->hiddenDepth + 1; ++layerIndex)
    {
        u32 inCount = 0;
        u32 outCount = 0;
        f32 *in = 0;
        f32 *weights = 0;
        f32 *bias = 0;
        f32 *out = 0;
        if (layerIndex == 0)
        {
            // H[0] = map(WIH * I + HB[0]);
            inCount = network->inputCount;
            outCount = network->hiddenCount[0];
            in = inputs;
            weights = network->i2hWeights;
            bias = network->hiddenBias;
            out = network->hidden;
        }
        else if (layerIndex == network->hiddenDepth)
        {
            // O = map(WHO * H[d-1] + OB);
            inCount = network->hiddenCount[layerIndex - 1];
            outCount = network->outputCount;
            in = network->hidden + hiddenOffset;
            weights = network->h2oWeights;
            bias = network->outputBias;
            out = network->outputs;
        }
        else
        {
            // H[1] = map(WHH[0] * H[0] + HB[1]);
            // H[2] = map(WHH[1] * H[1] + HB[2]);
            // H[d-1] = map(WHH[d-2] * H[d-2] + HB[d-1]);
            inCount = network->hiddenCount[layerIndex - 1];
            outCount = network->hiddenCount[layerIndex];
            in = network->hidden + hiddenOffset;
            weights = network->h2hWeights + h2hOffset;
            hiddenOffset += inCount;
            h2hOffset += inCount * outCount;
            bias = network->hiddenBias + hiddenOffset;
            out = network->hidden + hiddenOffset;
        }
        
        predict_layer(inCount, outCount, in, weights, bias, out);
    }
}

internal void
add_to_array(u32 count, f32 *a, f32 *result)
{
    // TODO(michiel): Better naming
    for (u32 index = 0; index < count; ++index)
    {
        result[index] += a[index];
    }
}

internal void
subtract_array(u32 count, f32 *a, f32 *b, f32 *result)
{
    for (u32 index = 0; index < count; ++index)
    {
        result[index] = a[index] - b[index];
    }
}

internal void
calculate_gradient(u32 count, f32 *inputs, f32 *errors, f32 *result, f32 learningRate)
{
    for (u32 index = 0; index < count; ++index)
    {
        result[index] = errors[index] * delta_activation_function(inputs[index]) * learningRate;
    }
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

#if 0
internal void
teach_layer(u32 inputCount, u32 outputCount, f32 *inputs, f32 *errors,)
{
    // G = hadamard(1 / (1 - exp(-O)), T - O) * learningRate;
    // G = hadamard(O + (1 - O), T - O) * learningRate;
    // OB += G;
    // W += G * transpose(I);
}
#endif

internal void
train(Neural *network, u32 inputCount, f32 *inputs, u32 targetCount, f32 *targets,
      f32 learningRate = 0.1f)
{
    i_expect(network->inputCount == inputCount);
    i_expect(network->outputCount == targetCount);
    
    predict(network, inputCount, inputs);
    
    u32 hiddenDepthIndex = network->hiddenDepth - 1;
    u32 hiddenIndex = 0;
    u32 h2hIndex = 0;
    for (u32 i = 0; i < network->hiddenDepth - 1; ++i)
    {
        hiddenIndex += network->hiddenCount[i];
        if (i > 0)
        {
            h2hIndex += network->hiddenCount[i] * network->hiddenCount[i - 1];
        }
    }
    
    f32 *currentErrors = network->trainErrorsA;
    f32 *nextErrors = network->trainErrorsB;
    
    // E = T - O;
    subtract_array(network->outputCount, targets, network->outputs, currentErrors);

#if 0    
    for (u32 err = 0; err < network->outputCount; ++err)
    {
        f32 val = currentErrors[err];
        currentErrors[err] = val * currentErrors[err] * 0.5f;
    }
    #endif

    // G = hadamard(1 / (1 - exp(-O)), E) * learningRate;
    calculate_gradient(network->outputCount, network->outputs, currentErrors,
                       network->trainGradient, learningRate);
    // OB += G
    add_to_array(network->outputCount, network->trainGradient, network->outputBias);
    // OW += G * transpose(H[-1]);
    add_weights(network->outputCount, network->hiddenCount[hiddenDepthIndex], 
                network->trainGradient, network->hidden + hiddenIndex, network->h2oWeights);
    
    f32 *weights = network->h2oWeights;
    u32 currentCount = network->outputCount;

for (; hiddenDepthIndex > 0; --hiddenDepthIndex)
    {
        u32 layerCount = network->hiddenCount[hiddenDepthIndex];
        // E = transpose(weights) * E;
        transpose_multiply(layerCount, currentCount, weights, currentErrors, nextErrors);
        
        // G = hadamard(1 / (1 - exp(-H[d])), E) * learningRate;
        calculate_gradient(layerCount, network->hidden + hiddenIndex,
                           nextErrors, network->trainGradient, learningRate);
        // HB[d] += G;
        add_to_array(layerCount, network->trainGradient, network->hiddenBias + hiddenIndex);
        
        u32 prevCount = network->hiddenCount[hiddenDepthIndex - 1];
        hiddenIndex -= prevCount;
        
        // weights = HW[d-1];
        // HW[d-1] += G * transpose(H[d-1]);
        weights = network->h2hWeights + h2hIndex;
        add_weights(layerCount, prevCount, network->trainGradient,
                    network->hidden + hiddenIndex, weights);
        
        if (hiddenDepthIndex > 1)
        {
            h2hIndex -= prevCount * network->hiddenCount[hiddenDepthIndex - 2];;
        }
        
        f32 *tempError = currentErrors;
        currentErrors = nextErrors;
        nextErrors = tempError;
        currentCount = layerCount;
    }
    
    i_expect(hiddenDepthIndex == 0);
    i_expect(hiddenIndex == 0);
    i_expect(h2hIndex == 0);
    // E = transpose(weights) * E;
    transpose_multiply(network->hiddenCount[0], currentCount, weights, 
                       currentErrors, nextErrors);
    // G = hadamard(1 / (1 - exp(-H[0])), E) * learningRate;
    calculate_gradient(network->hiddenCount[0], network->hidden, nextErrors,
                       network->trainGradient, learningRate);
    // HB[0] += G;
    add_to_array(network->hiddenCount[0], network->trainGradient, network->hiddenBias);
    // IW += G * transpose(I);
    add_weights(network->hiddenCount[0], network->inputCount, network->trainGradient,
                inputs, network->i2hWeights);
}
