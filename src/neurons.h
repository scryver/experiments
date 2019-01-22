#define NEURON_MUTATE(name) f32 name(f32 a, void *user)
typedef NEURON_MUTATE(NeuronMutate);

#define NEURON_ACTIVATE(name) f32 name(f32 a, void *user)
typedef NEURON_ACTIVATE(NeuronActivate);
#define NEURON_DELTA_ACTIVATE(name) f32 name(f32 a, void *user)
typedef NEURON_MUTATE(NeuronDeltaActivate);

#define NEURON_EVALUATE(name) u32 name(u32 count, f32 *targets, f32 *outputs)
typedef NEURON_EVALUATE(NeuronEvalutation);

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
    Arena arena;
    
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
    u32 totalWeights;
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
    network->layerCount.u = arena_allocate_array(&network->arena, u32, layerCount);
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
    network->layerSizes = arena_allocate_array(&network->arena, u32, network->layerCount);
    network->totalNeurons = 0;
    for (u32 hIndex = 0; hIndex < hiddenDepth; ++hIndex)
    {
        u32 count = hiddenCounts[hIndex];
        network->layerSizes[hIndex] = count;
        network->totalNeurons += count;
    }
    network->layerSizes[hiddenDepth] = outputCount;
    network->totalNeurons += outputCount;
    
    network->hidden = arena_allocate_array(&network->arena, f32, network->totalNeurons);
    network->biases = arena_allocate_array(&network->arena, f32, network->totalNeurons);
    
    u32 maxErrorLen = 0; // network->layerSizes[0];
    network->totalWeights = 0;
    u32 prevCount = network->inputCount;
    for (u32 i = 0; i < network->layerCount; ++i)
    {
        u32 count = network->layerSizes[i];
        network->totalWeights += count * prevCount;
        prevCount = count;
        
        if (maxErrorLen < count)
        {
            maxErrorLen = count;
        }
    }
    network->weights = arena_allocate_array(&network->arena, f32, network->totalWeights);
    
    network->outputs = network->hidden + network->totalNeurons - outputCount;
    
    network->trainErrorsA = arena_allocate_array(&network->arena, f32, maxErrorLen);
    network->trainErrorsB = arena_allocate_array(&network->arena, f32, maxErrorLen);
    network->trainGradient = arena_allocate_array(&network->arena, f32, maxErrorLen);
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
randomize_weights(RandomSeriesPCG *random, Neural *network, b32 compensateWeights = true)
{
    u32 prevCount = network->inputCount;
    f32 *weights = network->weights;
    for (u32 l = 0; l < network->layerCount; ++l)
    {
        u32 count = network->layerSizes[l];
        f32 oneOverSqrtX = 1.0f;
        if (compensateWeights)
        {
            oneOverSqrtX /= sqrt(prevCount);
        }
        for (u32 i = 0; i < count * prevCount; ++i)
        {
            weights[i] = random_bilateral(random) * oneOverSqrtX;
        }
        prevCount = count;
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
    for (u32 i = 0; i < network->totalWeights; ++i)
    {
        network->weights[i] = mutateFunc(network->weights[i], user);
    }
    for (u32 hIndex = 0; hIndex < network->totalNeurons; ++hIndex)
    {
        network->biases[hIndex] = mutateFunc(network->biases[hIndex], user);
    }
}

internal inline
NEURON_ACTIVATE(activation_function)
{
    return 1.0f / (1.0f + exp(-a));
}

internal inline 
NEURON_DELTA_ACTIVATE(delta_activation_function)
{
    return a * (1.0f - a);
}

internal void
predict_layer(u32 inCount, u32 outCount, f32 *inputs, f32 *weights, f32 *bias, f32 *outputs,
              b32 softmax = false,
              NeuronActivate *activation = activation_function, void *user = 0)
{
    // O = sigmoid(W * I + B);
    f32 *W = weights;
    f32 sum = 0.0f;
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
        
        if (softmax)
        {
            result = exp(result);
            sum += result;
            outputs[row] = result;
        }
        else
        {
            // NOTE(michiel): Sigmoid and store
            outputs[row] = activation(result, user);
        }
    }
    
    if (softmax)
    {
        f32 oneOverSum = 1.0f / sum;
        for (u32 row = 0; row < outCount; ++row)
        {
            outputs[row] *= oneOverSum;
        }
    }
}

internal void
predict(u32 inputCount, f32 *inputs, u32 layerCount, u32 *layerSizes,
        f32 *hidden, f32 *weights, f32 *biases, b32 softmax = false,
        NeuronActivate *activation = activation_function, void *user = 0)
{
    u32 hiddenOffset = 0;
    u32 h2hOffset = 0;
    for (u32 layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
        u32 inCount = 0;
        u32 outCount = 0;
        f32 *in = hidden + hiddenOffset;
        f32 *w = weights + h2hOffset;
        f32 *bias = 0;
        f32 *out = 0;
        b32 layerSoftmax = false;
        
        if (layerIndex == 0)
        {
            // H[0] = map(WIH * I + HB[0]);
            inCount = inputCount;
            outCount = layerSizes[layerIndex];
            in = inputs;
        }
        else
        {
            // H[1] = map(WHH[0] * H[0] + HB[1]);
            // H[2] = map(WHH[1] * H[1] + HB[2]);
            // H[d-1] = map(WHH[d-2] * H[d-2] + HB[d-1]);
            inCount = layerSizes[layerIndex - 1];
            outCount = layerSizes[layerIndex];
            
            hiddenOffset += inCount;
            
            if (layerIndex == layerCount - 1)
            {
                // NOTE(michiel): Output
                layerSoftmax = softmax;
            }
        }
        bias = biases + hiddenOffset;
        out = hidden + hiddenOffset;
        
        h2hOffset += inCount * outCount;
        
        predict_layer(inCount, outCount, in, w, bias, out, layerSoftmax, activation, user);
    }
}

internal void
predict(Neural *network, u32 inputCount, f32 *inputs, b32 softmax = false,
        NeuronActivate *activation = activation_function, void *user = 0)
{
    i_expect(inputCount == network->inputCount);
    
    predict(inputCount, inputs, network->layerCount, network->layerSizes,
            network->hidden, network->weights, network->biases, softmax,
            activation, user);
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
calculate_gradient(u32 count, f32 *inputs, f32 *errors, f32 *result, f32 learningRate,
                   NeuronDeltaActivate *delta_activate = delta_activation_function,
                   void *user = 0)
{
    for (u32 index = 0; index < count; ++index)
    {
        result[index] = errors[index] * delta_activate(inputs[index], user) * learningRate;
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
      f32 learningRate = 0.1f, NeuronDeltaActivate *delta_activate = delta_activation_function,
      void *user = 0)
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
                           learningRate, delta_activate, user);
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

internal inline f32
delta_function(f32 a)
{
    return a * (1.0f - a);
}

#if 1
internal inline f32
cost_function(f32 a)
{
    return 1.0f;
}
#else
internal inline f32
cost_function(f32 a)
{
    return delta_function(a);
}
#endif

internal void
back_propagate(Arena *memoryArena,
               u32 layerCount, u32 *layerSizes, f32 *hidden, f32 *weights, f32 *biases,
               u32 totalNeurons, u32 totalWeights, 
               Training *training, f32 *deltaWeights, f32 *deltaBiases, b32 softmax = false,
               NeuronActivate *activate = activation_function, void *user = 0)
{
    predict(training->inputCount, training->inputs,
            layerCount, layerSizes, hidden, weights, biases, 
            softmax, activate, user);
    
    TempMemory tempMem = temporary_memory(memoryArena);
    
    // NOTE(michiel): Backward pass
    
    u32 offset = totalNeurons - layerSizes[layerCount - 1];
    f32 *activation = hidden + offset;
    
    f32 *w = weights + totalWeights;
    
    f32 *dnb = deltaBiases + totalNeurons;
    f32 *dnw = deltaWeights + totalWeights;
    f32 *temp = arena_allocate_array(memoryArena, f32, totalNeurons);
    f32 *temp2 = arena_allocate_array(memoryArena, f32, totalNeurons);
    
    for (s32 layerIdx = layerCount - 1; layerIdx >= 0; --layerIdx)
    {
        u32 count = layerSizes[layerIdx];
        u32 prevCount = training->inputCount;
        if (layerIdx > 0)
        {
            prevCount = layerSizes[layerIdx - 1];
        }
        
        dnb -= count;
        dnw -= count * prevCount;
        
        if (layerIdx == layerCount - 1)
        {
            subtract_array(count, activation, training->outputs, temp);
            
            for (u32 bIdx = 0; bIdx < count; ++bIdx)
            {
                temp[bIdx] *= cost_function(activation[bIdx]);
            }
        }
        else
        {
            u32 nextCount = layerSizes[layerIdx + 1];
            
            f32 *colC = temp2;
            for (u32 row = 0; row < count; ++row)
            {
                f32 *rowW = w + row;
                f32 *rowT = temp + row;
                *rowT = 0.0f;
                for (u32 s = 0; s < nextCount; ++s)
                {
                    *rowT += rowW[s * count] * colC[s];
                }
                *rowT *= delta_function(activation[row]);
            }
        }
        
        w -= count * prevCount;
        
        for (u32 bIdx = 0; bIdx < count; ++bIdx)
        {
            dnb[bIdx] += temp[bIdx];
        }
        
        activation -= prevCount;
        if (activation < hidden)
        {
            activation = training->inputs;
        }
        
        add_weights(count, prevCount, temp, activation, dnw);
        
        prevCount = count;
        
        f32 *tempP = temp;
        temp = temp2;
        temp2 = tempP;
    }
    
    i_expect(dnb == deltaBiases);
    i_expect(dnw == deltaWeights);
    
    destroy_temporary(tempMem);
}

internal void
back_propagate(Neural *network, Training *training, f32 *deltaWeights, f32 *deltaBiases,
               b32 softmax = false,
               NeuronActivate *activation = activation_function, void *user = 0)
{
    i_expect(training->inputCount == network->inputCount);
    i_expect(training->outputCount == network->outputCount);
    
    back_propagate(&network->arena, network->layerCount, network->layerSizes, 
                   network->hidden, network->weights, network->biases,
                   network->totalNeurons, network->totalWeights, 
                   training, deltaWeights, deltaBiases, softmax,
                   activation, user);
}

internal void
update_mini_batch(Arena *memoryArena, u32 batchSize, Training *training, f32 learningRate, 
                  f32 lambda, u32 totalTrainingCount,
                  u32 layerCount, u32 *layerSizes, f32 *hidden, f32 *weights, f32 *biases,
                  u32 totalNeurons, u32 totalWeights, b32 softmax = false,
                  NeuronActivate *activation = activation_function, void *user = 0)
{
    TempMemory temp = temporary_memory(memoryArena);
    
    f32 *nw = arena_allocate_array(memoryArena, f32, totalWeights);
    f32 *nb = arena_allocate_array(memoryArena, f32, totalNeurons);
    
    for (u32 item = 0; item < batchSize; ++item)
    {
        Training *sample = training + item;
        back_propagate(memoryArena, layerCount, layerSizes, hidden, weights, biases, 
                       totalNeurons, totalWeights, sample, nw, nb, softmax,
                       activation, user);
    }
    
    f32 mod = learningRate / (f32)batchSize;
    f32 modReg = 1.0f - learningRate * (lambda / (f32)totalTrainingCount);
    for (u32 i = 0; i < totalWeights; ++i)
    {
        weights[i] = modReg * weights[i] - mod * nw[i];
    }
    for (u32 i = 0; i < totalNeurons; ++i)
    {
        biases[i] -= mod * nb[i];
    }
    
    destroy_temporary(temp);
}

internal void
update_mini_batch(Neural *network, u32 batchSize, Training *training, f32 learningRate,
                  f32 lambda, u32 totalTrainingCount, b32 softmax = false,
                  NeuronActivate *activation = activation_function, void *user = 0)
{
    update_mini_batch(&network->arena, batchSize, training, learningRate, 
                      lambda, totalTrainingCount,
                      network->layerCount, network->layerSizes, network->hidden,
                      network->weights, network->biases, network->totalNeurons,
                      network->totalWeights, softmax, activation, user);
}

internal void
update_mini_batch(Neural *network, TrainingSet training, f32 learningRate,
                  f32 lambda, u32 totalTrainingCount, b32 softmax = false,
                  NeuronActivate *activation = activation_function, void *user = 0)
{
    update_mini_batch(&network->arena, training.count, training.set, learningRate, 
                      lambda, totalTrainingCount,
                      network->layerCount, network->layerSizes, network->hidden,
                      network->weights, network->biases, network->totalNeurons,
                      network->totalWeights, softmax, activation, user);
}

internal
NEURON_EVALUATE(eval_neurons)
{
    u32 maxInt = U32_MAX;
    f32 max = 0.0f;
    u32 targetInt = U32_MAX;
    
    for (u32 o = 0; o < count; ++o)
    {
        f32 out = outputs[o];
        if (max < out)
        {
            max = out;
            maxInt = o;
        }
        
        if (targets[o] == 1.0f)
        {
            targetInt = o;
        }
    }
    
    if (maxInt == targetInt)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

internal
NEURON_EVALUATE(eval_single)
{
    u32 correct = 0;
    for (u32 i = 0; i < count; ++i)
    {
        if (((outputs[i] >= 0.5f) &&
             (targets[i] == 1.0f)) ||
            ((outputs[i] < 0.5f) &&
             (targets[i] == 0.0f)))
        {
            ++correct;
        }
    }
    
    return (correct > 0) ? 1 : 0;
}

internal inline u32
evaluate(Neural *network, TrainingSet tests,
         NeuronEvalutation *evaluation = eval_neurons)
{
    u32 correct = 0;
    
    for (u32 item = 0; item < tests.count; ++item)
    {
        Training *test = tests.set + item;
        predict(network, test->inputCount, test->inputs);
        
        correct += evaluation(test->outputCount, test->outputs, network->outputs);
    }
    
    return correct;
}

internal void
stochastic_gradient_descent(RandomSeriesPCG *random, Neural *network, 
                            u32 epochs, u32 miniBatchSize, f32 learningRate,
                            TrainingSet training, f32 lambda = 0.0f,
                            b32 softmax = false,
                            TrainingSet validation = {},
                            NeuronEvalutation *validateEval = eval_neurons,
                            b32 info = false)
{
    for (u32 epoch = 0; epoch < epochs; ++epoch)
    {
        shuffle(random, training.count, training.set);
        
        for (u32 batchAt = 0; 
             batchAt <= (training.count - miniBatchSize); 
             batchAt += miniBatchSize)
        {
            TrainingSet batch = {};
            batch.count = miniBatchSize;
            batch.set = training.set + batchAt;
            update_mini_batch(network, batch, learningRate, lambda, training.count, softmax);
        }
        
        if (validation.set)
        {
            u32 correct = evaluate(network, validation, validateEval);
            fprintf(stdout, "Epoch %u completed. (%u / %u)\n", epoch + 1, 
                    correct, validation.count);
        }
        else if (info)
        {
            fprintf(stdout, "Epoch %u completed.\n", epoch + 1);
        }
    }
}

//
// NOTE(michiel): Print functions
//

internal void
print_hidden(u32 layerCount, u32 *layerSizes, f32 *hidden,
             char *spacing = "  ")
{
    // NOTE(michiel): Can be used for biases as well
    
    /*
    * ------  ------  ------
    * | 0.1|  |-0.4|  | 0.6|
    * |-0.3|  | 0.5|  ------
    * | 0.2|  ------
    * ------
*/
    u32 maxRows = 0;
    for (u32 layerIdx = 0; layerIdx < layerCount; ++layerIdx)
    {
        u32 count = layerSizes[layerIdx];
        if (maxRows < count)
        {
            maxRows = count;
        }
        fprintf(stdout, "-------------%s", layerIdx < (layerCount - 1) ? spacing : "\n");
    }
    
    for (u32 r = 0; r < maxRows + 1; ++r)
    {
        f32 *h = hidden + r;
        
        for (u32 layerIdx = 0; layerIdx < layerCount; ++layerIdx)
        {
            u32 count = layerSizes[layerIdx];
            if (r < count)
            {
                fprintf(stdout, "|% .8f|%s", *h, layerIdx < (layerCount - 1) ? spacing : "\n");
            }
            else if (r == count)
            {
                fprintf(stdout, "-------------%s", layerIdx < (layerCount - 1) ? spacing : "\n");
            }
            else
            {
                fprintf(stdout, "             %s", layerIdx < (layerCount - 1) ? spacing : "\n");
            }
            h += count;
        }
    }
}

internal void
print_weights(u32 inputCount, u32 layerCount, u32 *layerSizes, f32 *weights,
              char *spacing = "  ")
{
    /*
    * -----------  ----------------  -----------
    * | 0.3  0.6|  |-1.0  0.4 -2.0|  |-1.2  0.3|
    * |-1.0  0.7|  | 3.0  0.5 -1.0|  -----------
    * | 0.7  0.3|  ----------------
    * -----------
    */
    u32 prevCount = inputCount;
    u32 maxRows = 0;
    for (u32 layerIdx = 0; layerIdx < layerCount; ++layerIdx)
    {
        u32 count = layerSizes[layerIdx];
        if (maxRows < count)
        {
            maxRows = count;
        }
        fprintf(stdout, "-");
        for (u32 p = 0; p < prevCount; ++p)
        {
            fprintf(stdout, "------------");
        }
        fprintf(stdout, "%s", layerIdx < (layerCount - 1) ? spacing : "\n");
        prevCount = count;
    }
    
    f32 *w = weights;
    
    for (u32 r = 0; r < maxRows + 1; ++r)
    {
        prevCount = inputCount;
        w = weights;
        
        for (u32 layerIdx = 0; layerIdx < layerCount; ++layerIdx)
        {
            u32 count = layerSizes[layerIdx];
            if (r < count)
            {
                f32 *wInner = w + r * prevCount;
                fprintf(stdout, "|");
                for (u32 p = 0; p < prevCount; ++p)
                {
                    fprintf(stdout, "% .8f%s", *wInner++, p < (prevCount - 1) ? "," : "");
                }
                fprintf(stdout, "|%s", layerIdx < (layerCount - 1) ? spacing : "\n");
                
                w += prevCount * count;
            }
            else if (r == count)
            {
                fprintf(stdout, "-");
                for (u32 p = 0; p < prevCount; ++p)
                {
                    fprintf(stdout, "------------");
                }
                fprintf(stdout, "%s", layerIdx < (layerCount - 1) ? spacing : "\n");
            }
            else
            {
                for (u32 p = 0; p < prevCount; ++p)
                {
                    fprintf(stdout, "          %s", p < (prevCount - 1) ? " " : "");
                }
                fprintf(stdout, "%s", layerIdx < (layerCount - 1) ? spacing : "\n");
            }
            prevCount = count;
        }
    }
}
