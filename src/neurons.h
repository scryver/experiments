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
    u32 outputCount;
    
#if 0    
    Array layerCount;
    Array layers;
    Array biases;
    Array weights;
#endif
    
    // NOTE(michiel): Real state
    u32 layerCount;
    u32 *layerSizes;
    f32 *biases;
    f32 *weights;
    
    // NOTE(michiel): Calculated state
    u32 totalNeurons;
    f32 *hidden;
    f32 *outputs;
    
    // NOTE(michiel): Temporary state
    f32 *trainErrorsA;
    f32 *trainErrorsB;
    f32 *trainGradient;
};

internal void
init_neural_network(Neural *network, u32 inputCount, 
                    u32 hiddenDepth, u32 *hiddenCounts, u32 outputCount)
{
    network->inputCount = inputCount; // NOTE(michiel): For verification, mostly
    network->outputCount = outputCount; // NOTE(michiel): For verification, mostly
    
    i_expect(network->layerSizes == 0);
    i_expect(network->hidden == 0);
    i_expect(network->biases == 0);
    i_expect(network->weights == 0);
    i_expect(network->outputs == 0);
    
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
    
    network->layerCount = hiddenDepth + 1;
    network->layerSizes = allocate_array(u32, network->layerCount);
    network->totalNeurons = 0;
    for (u32 hIndex = 0; hIndex < hiddenDepth; ++hIndex)
    {
        u32 count = hiddenCounts[hIndex];
        network->layerSizes[hIndex] = count;
        network->totalNeurons += count;
    }
    network->layerSizes[hiddenDepth] = outputCount;
    network->totalNeurons += outputCount;
    
    network->hidden = allocate_array(f32, network->totalNeurons);
    network->biases = allocate_array(f32, network->totalNeurons);
    
    u32 maxErrorLen = 0; // network->layerSizes[0];
    u32 h2hWeightLen = 0;
    u32 prevCount = network->inputCount;
    for (u32 i = 0; i < network->layerCount; ++i)
    {
        u32 count = network->layerSizes[i];
        h2hWeightLen += count * prevCount;
        prevCount = count;
        
        if (maxErrorLen < count)
        {
            maxErrorLen = count;
        }
    }
    network->weights = allocate_array(f32, h2hWeightLen);
    
    network->outputs = network->hidden + network->totalNeurons - outputCount;
    
    network->trainErrorsA = allocate_array(f32, maxErrorLen);
    network->trainErrorsB = allocate_array(f32, maxErrorLen);
    network->trainGradient = allocate_array(f32, maxErrorLen);
}

internal void
neural_copy(Neural *source, Neural *dest)
{
    i_expect(source->inputCount == dest->inputCount);
    i_expect(source->layerCount == dest->layerCount);
    
    u32 h2hOffset = 0;
    u32 prevCount = source->inputCount;
    for (u32 i = 0; i < source->layerCount; ++i)
    {
        i_expect(source->layerSizes[i] == dest->layerSizes[i]);
            u32 count = source->layerSizes[i];
        u32 h2hWidth = count * prevCount;
        prevCount = count;
        
        for (u32 hIndex = 0; hIndex < h2hWidth; ++hIndex)
        {
            dest->weights[hIndex + h2hOffset] = source->weights[hIndex + h2hOffset];
        }
        
        h2hOffset += h2hWidth;
    }
    for (u32 hIndex = 0; hIndex < source->totalNeurons; ++hIndex)
    {
        dest->biases[hIndex] = source->biases[hIndex];
    }
}

internal void
randomize_weights(RandomSeriesPCG *random, Neural *network)
{
    u32 h2hOffset = 0;
    u32 prevCount = network->inputCount;
        for (u32 i = 0; i < network->layerCount; ++i)
    {
        u32 h2hWidth;
        u32 count = network->layerSizes[i];
        h2hWidth = count * prevCount;
        prevCount = count;
        
        for (u32 hIndex = 0; hIndex < h2hWidth; ++hIndex)
        {
            network->weights[hIndex + h2hOffset] = random_bilateral(random);
        }
        h2hOffset += h2hWidth;
    }
    for (u32 hIndex = 0; hIndex < network->totalNeurons; ++hIndex)
    {
        network->biases[hIndex] = random_bilateral(random);
    }
}

internal void
neural_mixin(RandomSeriesPCG *random, Neural *source, Neural *dest, f32 rate = 0.5f)
{
    i_expect(source->inputCount == dest->inputCount);
    i_expect(source->layerCount == dest->layerCount);
    
    u32 h2hOffset = 0;
    u32 prevCount = source->inputCount;
    for (u32 i = 0; i < source->layerCount; ++i)
    {
        i_expect(source->layerSizes[i] == dest->layerSizes[i]);
            u32 count = source->layerSizes[i];
        u32 h2hWidth = count * prevCount;
        prevCount = count;
        
        for (u32 hIndex = 0; hIndex < h2hWidth; ++hIndex)
        {
            if (random_unilateral(random) < rate)
            {
            dest->weights[hIndex + h2hOffset] = source->weights[hIndex + h2hOffset];
            }
        }
        h2hOffset += h2hWidth;
    }
    for (u32 hIndex = 0; hIndex < source->totalNeurons; ++hIndex)
    {
        if (random_unilateral(random) < rate)
        {
        dest->biases[hIndex] = source->biases[hIndex];
        }
    }
}

internal void
neural_mutate(RandomSeriesPCG *random, Neural *network, NeuronMutate *mutateFunc,
              void *user = 0)
{
    u32 h2hOffset = 0;
    u32 prevCount = network->inputCount;
    for (u32 i = 0; i < network->layerCount + 1; ++i)
    {
        u32 count = network->layerSizes[i];
        u32 h2hWidth = count * prevCount;
        prevCount = count;
        
        for (u32 hIndex = 0; hIndex < h2hWidth; ++hIndex)
        {
            network->weights[hIndex + h2hOffset] = mutateFunc(network->weights[hIndex + h2hOffset], user);
            }
        h2hOffset += h2hWidth;
    }
    for (u32 hIndex = 0; hIndex < network->totalNeurons; ++hIndex)
    {
        network->biases[hIndex] = mutateFunc(network->biases[hIndex], user);
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

#if 1
        u32 inCount4 = inCount >> 2;
        u32 inCountR = inCount & 0x3;
        
        for (u32 col4 = 0; col4 < inCount4; ++col4)
        {
            result += (*W++) * inputs[(col4 << 2) + 0];
            result += (*W++) * inputs[(col4 << 2) + 1];
            result += (*W++) * inputs[(col4 << 2) + 2];
            result += (*W++) * inputs[(col4 << 2) + 3];
        }

        for (u32 colR = 0; colR < inCountR; ++colR)
        {
            result += (*W++) * inputs[(inCount4 << 2) + colR];
        }
        #else
        for (u32 col = 0; col < inCount; ++col)
        {
            result += (*W++) * inputs[col];
        }
        #endif
        
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
    for (u32 layerIndex = 0; layerIndex < network->layerCount; ++layerIndex)
    {
        u32 inCount = 0;
        u32 outCount = 0;
        f32 *in = network->hidden + hiddenOffset;
        f32 *weights = network->weights + h2hOffset;
        f32 *bias = 0;
        f32 *out = 0;
        
        if (layerIndex == 0)
        {
            // H[0] = map(WIH * I + HB[0]);
            inCount = network->inputCount;
            outCount = network->layerSizes[layerIndex];
            in = inputs;
        }
        else
        {
            // H[1] = map(WHH[0] * H[0] + HB[1]);
            // H[2] = map(WHH[1] * H[1] + HB[2]);
            // H[d-1] = map(WHH[d-2] * H[d-2] + HB[d-1]);
            inCount = network->layerSizes[layerIndex - 1];
                outCount = network->layerSizes[layerIndex];
            
            hiddenOffset += inCount;
        }
        bias = network->biases + hiddenOffset;
        out = network->hidden + hiddenOffset;
        
        h2hOffset += inCount * outCount;
        
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
    i_expect(network->layerSizes[network->layerCount - 1] == targetCount);
    
    predict(network, inputCount, inputs);
    
    s32 layerCountIndex = network->layerCount - 1;
    s32 hiddenIndex = 0;
    s32 h2hIndex = 0;
    for (u32 i = 0; i < network->layerCount - 1; ++i)
    {
        hiddenIndex += network->layerSizes[i];
        if (i == 0)
        {
            h2hIndex += network->layerSizes[i] * network->inputCount;
        }
        else
        {
            h2hIndex += network->layerSizes[i] * network->layerSizes[i - 1];
        }
    }
    
    f32 *currentErrors = network->trainErrorsA;
    f32 *nextErrors = network->trainErrorsB;
    
    f32 *weights = 0; // network->weights + h2hIndex;
    u32 currentCount = 0; // network->outputCount;
    
for (; layerCountIndex >= 0; --layerCountIndex)
    {
        u32 layerCount = network->layerSizes[layerCountIndex];
        f32 *neurons = network->hidden + hiddenIndex;
        f32 *bias = network->biases + hiddenIndex;
        
        if (layerCountIndex == network->layerCount - 1)
        {
            // NOTE(michiel): Output
            
            // E = T - O;
            subtract_array(layerCount, targets, neurons, nextErrors);
            #if 0
            for (u32 err = 0; err < layerCount; ++err)
            {
                f32 val = nextErrors[err];
                nextErrors[err] = val * val * 0.5f;
            }
#endif
        }
        else
        {
            // NOTE(michiel): Hidden layer
            
        // E = transpose(weights) * E;
        transpose_multiply(layerCount, currentCount, weights, currentErrors, nextErrors);
        }
        
        // G = hadamard(1 / (1 - exp(-H[d])), E) * learningRate;
        calculate_gradient(layerCount, neurons, nextErrors, network->trainGradient,
                           learningRate);
        // HB[d] += G;
        add_to_array(layerCount, network->trainGradient, bias);
        
        u32 prevCount;
        if (layerCountIndex == 0)
        {
            prevCount = network->inputCount;
        }
        else
        {
         prevCount = network->layerSizes[layerCountIndex - 1];
        }
        
        // weights = HW[d-1];
        weights = network->weights + h2hIndex;
        hiddenIndex -= prevCount;
        f32 *transposed = network->hidden + hiddenIndex;
        if (layerCountIndex == 0)
        {
            transposed = inputs;
        }
        
        // HW[d-1] += G * transpose(H[d-1]);
        add_weights(layerCount, prevCount, network->trainGradient, transposed, weights);
        
        if (layerCountIndex == 1)
        {
            h2hIndex -= prevCount * network->inputCount;
        }
        else if (layerCountIndex > 1)
        {
            h2hIndex -= prevCount * network->layerSizes[layerCountIndex - 2];
        }
        
        f32 *tempError = currentErrors;
        currentErrors = nextErrors;
        nextErrors = tempError;
        currentCount = layerCount;
    }
    
    i_expect(layerCountIndex == -1);
    i_expect(hiddenIndex < 0);
    i_expect(h2hIndex == 0);
}
