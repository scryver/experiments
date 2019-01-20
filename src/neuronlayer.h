struct FullyConnected
{
    b32 softMax;
    
    f32 *weights;
    f32 *biases;
    };

struct FeatureMap
{
    u32 mapCount;
    
    u32 inputWidth;
    u32 inputHeight; // Implicit by inputCount / inputWidth
    u32 featureWidth;
    u32 featureHeight;
    //u32 stride;
    u32 poolWidth; // NOTE(michiel): If 0 no pooling
    u32 poolHeight;
    
    u32 *poolIndices; // NOTE(michiel): Backprop cache
    f32 *prePool;
    f32 *weights;
    f32 *biases;
};

enum NeuralLayers
{
    Neural_FullyConnected,
    Neural_FeatureMap,
};
struct NeuralLayer
{
    NeuralLayers kind;
    
    u32 inputCount;
    u32 outputCount;
    f32 *inputs;
    f32 *outputs;
    
    union
    {
        FullyConnected fullyConnected;
        FeatureMap     featureMap;
    };
};

struct NeuralCake
{
    Arena arena;
    
    // NOTE(michiel): Real state
    u32 layerCount;
    NeuralLayer *layers;
    
    f32 *outputs;
    
    u32 totalWeights;
    u32 totalBiases;
    
    u32 layersFilled;
};

internal void
init_neural_network(NeuralCake *network, u32 layers)
{
    i_expect(network->layers == 0);
    network->layerCount = layers;
    network->layers = arena_allocate_array(&network->arena, NeuralLayer, layers);
    network->layersFilled = 0;
}

internal void
add_fully_connected_layer(NeuralCake *network, u32 inputCount, u32 outputCount, 
                          b32 softMax = false)
{
    i_expect(network->layersFilled < network->layerCount);
    i_expect(inputCount);
    i_expect(outputCount);
    
    NeuralLayer *layer = network->layers + network->layersFilled++;
    layer->kind = Neural_FullyConnected;
    if (network->layersFilled > 1)
    {
        i_expect((layer - 1)->outputCount == inputCount);
    }
    layer->inputCount = inputCount;
    layer->outputCount = outputCount;
    
    layer->fullyConnected.softMax = softMax;
    
    layer->fullyConnected.weights = 0;
    layer->fullyConnected.biases  = 0;
}

internal void
add_fully_connected_layers(NeuralCake *network, u32 counts, u32 *dataSizes)
{
    i_expect(counts > 1);
    i_expect((network->layersFilled + counts - 1) <= network->layerCount);
    u32 inputCount = dataSizes[0];
    for (u32 c = 1; c < counts; ++c)
    {
        u32 outputCount = dataSizes[c];
        add_fully_connected_layer(network, inputCount, outputCount);
        inputCount = outputCount;
    }
}

internal void
add_feature_map_layer(NeuralCake *network, u32 maps,
                      u32 inputWidth, u32 inputHeight, u32 featureWidth, u32 featureHeight,
                      u32 stride = 1)
{
    i_expect(network->layersFilled < network->layerCount);
    i_expect(inputWidth);
    i_expect(inputHeight);
    i_expect(featureWidth);
    i_expect(featureHeight);
    i_expect(stride);
    i_expect(inputWidth > featureWidth);
    i_expect(inputHeight > featureHeight);
    i_expect(stride < featureWidth);
    
    NeuralLayer *layer = network->layers + network->layersFilled++;
    layer->kind = Neural_FeatureMap;
    layer->inputCount = inputWidth * inputHeight;
    if (network->layersFilled > 1)
    {
        i_expect((layer - 1)->outputCount == layer->inputCount);
    }
    layer->outputCount = (inputWidth - featureWidth + 1) * (inputHeight - featureHeight + 1) * maps;
    
    layer->featureMap.mapCount = maps;
    
    layer->featureMap.inputWidth = inputWidth;
    layer->featureMap.inputHeight = inputHeight;
    layer->featureMap.featureWidth = featureWidth;
    layer->featureMap.featureHeight = featureHeight;
    layer->featureMap.poolWidth = 0;
    layer->featureMap.poolHeight = 0;
    //layer->featureMap.stride = stride;
    
    layer->featureMap.poolIndices = 0;
    layer->featureMap.weights = 0;
    layer->featureMap.biases = 0;
}

internal void
add_max_pooling_feature_map_layer(NeuralCake *network, u32 maps,
                                  u32 inputWidth, u32 inputHeight,
                                  u32 featureWidth, u32 featureHeight, 
                                  u32 poolWidth, u32 poolHeight, u32 stride = 1)
{
    i_expect(network->layersFilled < network->layerCount);
    i_expect(inputWidth);
    i_expect(inputHeight);
    i_expect(featureWidth);
    i_expect(featureHeight);
    i_expect(poolWidth);
    i_expect(poolHeight);
    i_expect(stride);
    i_expect(inputWidth > featureWidth);
    i_expect(inputHeight > featureHeight);
    i_expect(stride < featureWidth);
    
    u32 outW = (inputWidth - featureWidth + 1);
    u32 outH = (inputHeight - featureHeight + 1);
    i_expect(outW > poolWidth);
    i_expect(outH > poolHeight);
    i_expect((outW % poolWidth) == 0);
    i_expect((outH % poolHeight) == 0);
    
    NeuralLayer *layer = network->layers + network->layersFilled++;
    layer->kind = Neural_FeatureMap;
    layer->inputCount = inputWidth * inputHeight;
    if (network->layersFilled > 1)
    {
        i_expect((layer - 1)->outputCount == layer->inputCount);
    }
    layer->outputCount = (outW / poolWidth) * (outH / poolHeight) * maps;
    
    layer->featureMap.mapCount = maps;
    
    layer->featureMap.inputWidth = inputWidth;
    layer->featureMap.inputHeight = inputHeight;
    layer->featureMap.featureWidth = featureWidth;
    layer->featureMap.featureHeight = featureHeight;
    layer->featureMap.poolWidth = poolWidth;
    layer->featureMap.poolHeight = poolHeight;
    //layer->featureMap.stride = stride;
    
    layer->featureMap.poolIndices = 0;
    layer->featureMap.weights = 0;
    layer->featureMap.biases = 0;
    }

internal void
finish_network(NeuralCake *network)
{
    i_expect(network->layersFilled == network->layerCount);
    
    u32 neuronCount = 0;
    u32 weightCount = 0;
    u32 biasCount = 0;
    u32 poolIdxCount = 0;
    for (u32 l = 0; l < network->layerCount; ++l)
    {
        NeuralLayer *layer = network->layers + l;
        switch(layer->kind)
        {
            case Neural_FullyConnected:
            {
                u32 inputCount = layer->inputCount;
                u32 outputCount = layer->outputCount;
                
                neuronCount += outputCount;
                weightCount += outputCount * inputCount;
                biasCount += outputCount;
            } break;
            
            case Neural_FeatureMap:
            {
                u32 outputCount = layer->outputCount;
                
                neuronCount += outputCount;
                weightCount += (layer->featureMap.featureWidth * layer->featureMap.featureHeight) * layer->featureMap.mapCount;
                biasCount += layer->featureMap.mapCount;
                
                if (layer->featureMap.poolWidth)
                {
                    i_expect(layer->featureMap.poolHeight);
                    neuronCount +=
                        (layer->featureMap.inputWidth - layer->featureMap.featureWidth + 1) *
                        (layer->featureMap.inputHeight - layer->featureMap.featureHeight + 1) *
                        layer->featureMap.mapCount;
                    poolIdxCount += layer->outputCount;
                }
            } break;
            
            INVALID_DEFAULT_CASE;
        }
        
        //layer->fullyConnected.weights = arena_allocate_array(&network->arena, f32, inputCount * outputCount);
        //layer->fullyConnected.biases  = arena_allocate_array(&network->arena, f32, outputCount);
        
        //layer->featureMap.weights = arena_allocate_array(&network->arena, f32, featureWidth * featureHeight)
    }
    
    network->totalWeights = weightCount;
    network->totalBiases = biasCount;
    
    f32 *allTheValues = arena_allocate_array(&network->arena, f32,
                                             neuronCount + weightCount + biasCount + poolIdxCount);
    
    f32 *neurons = allTheValues;
    f32 *weights = neurons + neuronCount;
    f32 *biases = weights + weightCount;
    u32 *poolIndices = (u32 *)(biases + biasCount);
    
    f32 *n = neurons;
    f32 *w = weights;
    f32 *b = biases;
    u32 *p = poolIndices;
    
    for (u32 l = 0; l < network->layerCount; ++l)
    {
        NeuralLayer *layer = network->layers + l;
        if (l > 0)
        {
            layer->inputs = n - layer->inputCount;
        }
        else
        {
            layer->inputs = 0;
        }
        
        switch(layer->kind)
        {
            case Neural_FullyConnected:
            {
                u32 inputCount = layer->inputCount;
                u32 outputCount = layer->outputCount;
                
                layer->fullyConnected.weights = w;
                w += inputCount * outputCount;
                layer->fullyConnected.biases = b;
                b += outputCount;
            } break;
            
            case Neural_FeatureMap:
            {
                u32 maps = layer->featureMap.mapCount;
                u32 inW = layer->featureMap.inputWidth;
                u32 inH = layer->featureMap.inputHeight;
                u32 fW = layer->featureMap.featureWidth;
                u32 fH = layer->featureMap.featureHeight;
                u32 pW = layer->featureMap.poolWidth;
                u32 pH = layer->featureMap.poolHeight;
                
                if (pW)
                {
                    i_expect(pH);
                    layer->featureMap.poolIndices = p;
                    p += layer->outputCount;
                    layer->featureMap.prePool = n;
                    n += (inW - fW + 1) * (inH - fH + 1) * maps;
                }
                
                layer->featureMap.weights = w;
                w += fW * fH * maps;
                layer->featureMap.biases = b;
                b += maps;
            } break;
            
            INVALID_DEFAULT_CASE;
        }
        
        layer->outputs = n;
        n += layer->outputCount;
        
        if (l == network->layerCount - 1)
        {
            network->outputs = layer->outputs;
        }
    }
    
    //fprintf(stderr, "n-N: %lu | w-W: %lu | b-B: %lu | nC %u | wC %u | bC %u\n",
            //        (n - neurons), (w - weights), (b - biases), neuronCount, weightCount, biasCount);
    i_expect((n - neurons) == neuronCount);
    i_expect((w - weights) == weightCount);
    i_expect((b - biases) == biasCount);
    i_expect(n == weights);
    i_expect(w == biases);
}

internal void
randomize_weights(RandomSeriesPCG *random, NeuralCake *network, b32 compensateWeights = true)
{
    i_expect(network->layersFilled == network->layerCount);
    
    for (u32 l = 0; l < network->layerCount; ++l)
    {
        NeuralLayer *layer = network->layers + l;
         switch (layer->kind)
        {
            case Neural_FullyConnected:
            {
                u32 inputCount = layer->inputCount;
                u32 outputCount = layer->outputCount;
                
                f32 oneOverSqrtX = 1.0f;
                if (compensateWeights)
                {
                    oneOverSqrtX /= sqrt(inputCount);
                }
                for (u32 w = 0; w < inputCount * outputCount; ++w)
                {
                    layer->fullyConnected.weights[w] = random_bilateral(random);
                }
                for (u32 b = 0; b < outputCount; ++b)
                {
                    layer->fullyConnected.biases[b] = random_bilateral(random);
                }
            } break;
            
            case Neural_FeatureMap:
            {
                u32 size = layer->featureMap.featureWidth * layer->featureMap.featureHeight * layer->featureMap.mapCount;
                for (u32 w = 0; w < size; ++w)
                {
                        layer->featureMap.weights[w] = random_bilateral(random);
                }
                for (u32 b = 0; b < layer->featureMap.mapCount; ++b)
                {
                    layer->featureMap.biases[b] = 0.0f; // random_bilateral(random);
                }
                } break;
            
            INVALID_DEFAULT_CASE;
        }
    }
}

internal inline f32
activate_neuron(f32 a)
{
    return 1.0f / (1.0f + exp(-a));
}

internal inline f32
delta_neuron(f32 a)
{
    return a * (1.0f - a);
}

internal inline f32
log_likely_cost_neuron(f32 a)
{
    return 1.0f;
}

internal inline f32
square_cost_neuron(f32 a)
{
    return delta_neuron(a);
}

internal void
predict_layer(NeuralLayer *layer)
{
    // O = sigmoid(W * I + B);
    switch (layer->kind)
    {
        case Neural_FullyConnected:
        {
            f32 *W = layer->fullyConnected.weights;
            f32 sum = 0.0f;
            for (u32 row = 0; row < layer->outputCount; ++row)
            {
                f32 result = 0.0f;
                // NOTE(michiel): Weights
                for (u32 col = 0; col < layer->inputCount; ++col)
                {
                    result += (*W++) * layer->inputs[col];
                }
                
                // NOTE(michiel): Bias
                result += layer->fullyConnected.biases[row];
                
                    // NOTE(michiel): Sigmoid and store
                if (layer->fullyConnected.softMax)
                {
                    result = exp(result);
                    sum += result;
                    layer->outputs[row] = result;
                }
                else
                {
                    layer->outputs[row] = activate_neuron(result);
                }
            }
            
            if (layer->fullyConnected.softMax)
            {
                f32 oneOverSum = 1.0f / sum;
                for (u32 row = 0; row < layer->outputCount; ++row)
                {
                    layer->outputs[row] *= oneOverSum;
                }
            }
        } break;
        
        case Neural_FeatureMap:
        {
            u32 inW = layer->featureMap.inputWidth;
            u32 inH = layer->featureMap.inputHeight;
            u32 fW = layer->featureMap.featureWidth;
            u32 fH = layer->featureMap.featureHeight;
            u32 pW = layer->featureMap.poolWidth;
            u32 pH = layer->featureMap.poolHeight;
            u32 outW = inW - fW + 1;
            u32 outH = inH - fH + 1;
            
            b32 usePool = pW && pH;
            
            f32 *out = layer->outputs;
            if (usePool)
            {
                out = layer->featureMap.prePool;
            }
            
            f32 *in = layer->inputs;
            f32 *weights = layer->featureMap.weights;
            f32 *biases = layer->featureMap.biases;
            
            for (u32 m = 0; m < layer->featureMap.mapCount; ++m)
            {
            for (u32 y = 0; y < outH; ++y)
            {
                f32 *inRow = layer->inputs + y * inW;
                f32 *outRow = out + y * outW;
                for (u32 x = 0; x < outW; ++x)
                {
                    f32 val = 0.0f;
                    for (u32 fy = 0; fy < fH; ++fy)
                    {
                        f32 *inFRow = inRow + x + fy * inW;
                            f32 *wRow = weights + fy * fW;
                        for (u32 fx = 0; fx < fW; ++fx)
                        {
                                val += wRow[fx] * inFRow[fx];
                        }
                    }
                        outRow[x] = activate_neuron(val + biases[0]);
                }
            }
                out += outW * outH;
                weights += fW * fH;
                ++biases;
            }
            
            if (usePool)
            {
                out = layer->outputs;
                u32 *indxs = layer->featureMap.poolIndices;
                
                u32 outPW = outW / pW;
                u32 outPH = outH / pH;
                i_expect((outPW * outPH * layer->featureMap.mapCount) == layer->outputCount);
                
                 in = layer->featureMap.prePool;
                
                for (u32 m = 0; m < layer->featureMap.mapCount; ++m)
                {
                    for (u32 y = 0; y < outPH; ++y)
                    {
                        f32 *outRow = out + y * outPW;
                        f32 *inputRow = in + y * outW * pW;
                        for (u32 x = 0; x < outPW; ++x)
                        {
                            f32 max = F32_MIN;
                            u32 maxIdx = U32_MAX;
                            for (u32 py = 0; py < pH; ++py)
                            {
                                f32 *inputPRow = inputRow + py * outW;
                                for (u32 px = 0; px < pW; ++px)
                                {
                                    if (max < inputPRow[px])
                                    {
                                        max = inputPRow[px];
                                        maxIdx = (u64)(inputPRow + px - in);
                                    }
                                }
                            }
                            indxs[x] = maxIdx;
                        outRow[x] = max;
                            }
                    }
                    
                    in += outW * outH;
                    out += outPW * outPH;
                    indxs += outPW * outPH;
                }
            }
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}

internal void
predict(NeuralCake *network, u32 inputCount, f32 *inputs)
{
    i_expect(network->layers[0].inputCount == inputCount);
    network->layers[0].inputs = inputs;
    
    for (u32 layerIndex = 0; layerIndex < network->layerCount; ++layerIndex)
    {
        NeuralLayer *layer = network->layers + layerIndex;
        predict_layer(layer);
    }
}

internal void
predict(NeuralCake *network, Training *input)
{
    predict(network, input->inputCount, input->inputs);
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
back_propagate_fully_connected(NeuralLayer *layer, Training *training, b32 finalLayer,
                               f32 **dnw, f32 **dnb, f32 *prevError, f32 *error)
{
    u32 count = layer->outputCount;
    u32 prevCount = layer->inputCount;
    
    *dnb -= count;
    *dnw -= count * prevCount;
    
    if (finalLayer)
    {
        i_expect(training->outputCount == layer->outputCount);
        
        subtract_array(count, layer->outputs, training->outputs, error);
        
        for (u32 bIdx = 0; bIdx < count; ++bIdx)
        {
            error[bIdx] *= log_likely_cost_neuron(layer->outputs[bIdx]);
        }
    }
    else
    {
        NeuralLayer *nextLayer = layer + 1;
        if (nextLayer->kind == Neural_FullyConnected)
        {
            u32 nextCount = nextLayer->outputCount;
            
            f32 *colC = prevError;
            for (u32 row = 0; row < count; ++row)
            {
                f32 *rowW = nextLayer->fullyConnected.weights + row;
                f32 *rowT = error + row;
                f32 val = 0.0f;
                for (u32 s = 0; s < nextCount; ++s)
                {
                    val += rowW[s * count] * colC[s];
                }
                val *= delta_neuron(layer->outputs[row]);
                *rowT = val;
            }
        }
        else
        {
            INVALID_CODE_PATH;
        }
    }
    
    for (u32 bIdx = 0; bIdx < count; ++bIdx)
    {
        *dnb[bIdx] += error[bIdx];
    }
    
    add_weights(count, prevCount, error, layer->inputs, *dnw);
}

internal void
back_propagate_feature_map(NeuralLayer *layer, Training *training, b32 finalLayer,
                               f32 **dnw, f32 **dnb, f32 *prevError, f32 *error)
{
    i_expect(!finalLayer); // NOTE(michiel): Can't be the last layer
    
    u32 maps = layer->featureMap.mapCount;
    
    u32 inW = layer->featureMap.inputWidth;
    u32 inH = layer->featureMap.inputHeight;
    u32 fW = layer->featureMap.featureWidth;
    u32 fH = layer->featureMap.featureHeight;
    u32 pW = layer->featureMap.poolWidth;
    u32 pH = layer->featureMap.poolHeight;
    u32 outW = inW - fW + 1;
    u32 outH = inH - fH + 1;
    
    i_expect(fW <= outW);
    
    b32 usePool = pW && pH;
    
    if (usePool)
    {
        u32 outPW = outW / pW;
        u32 outPH = outH / pH;
        
        u32 *pIdxs = layer->featureMap.poolIndices;
        f32 *pErr = prevError;
        f32 *err = error;
        
        for (u32 map = 0; map < maps; ++map)
        {
            for (u32 y = 0; y < outH; ++y)
            {
                f32 *errRow = err + y * outW;
                for (u32 x = 0; x < outW; ++x)
                {
                    errRow[x] = 0.0f;
                }
            }
            
            for (u32 i = 0; i < outPW * outPH; ++i)
            {
                u32 index = pIdxs[i];
                err[index] = pErr[i];
            }
            
            pIdxs += outPW * outPH;
            pErr += outPW * outPH;
            err += outW * outH;
        }
        
        f32 *t = prevError;
        prevError = error;
        error = t;
    }
    
    for (u32 y = 0; y < inH; ++y)
    {
        for (u32 x = 0; x < inW; ++x)
        {
            error[y * inW + x] = 0.0f;
        }
    }
    
    f32 *output = layer->outputs;
    f32 *pErr = prevError;
    f32 *weights = layer->featureMap.weights;
    
    for (u32 map = 0; map < maps; ++map)
    {
        *dnw -= fW * fH;
        --(*dnb);
        
        for (u32 fY = 0; fY < fH; ++fY)
        {
            f32 *wRow = *dnw + fY * fW;
            f32 *inGRow = layer->inputs + fY * inW;
            for (u32 fX = 0; fX < fW; ++fX)
            {
                f32 val = 0.0f;
                for (u32 y = 0; y < outH; ++y)
                {
                    f32 *inRow = inGRow + fX + y * inW;
                    f32 *outRow = pErr + y * outW;
                    for (u32 x = 0; x < outW; ++x)
                    {
                        val += inRow[x] * delta_neuron(outRow[x]);
                    }
                }
                wRow[fX] += val;
            }
        }
        
        for (u32 inY = 0; inY < inH; ++inY)
        {
            f32 *inRow = error + inY * inW;
            for (u32 inX = 0; inX < inW; ++inX)
            {
                f32 val = 0.0f;
                for (u32 fy = 0; fy < fH; ++fy)
                {
                    s32 yOff = inY - fH + 1 + fy;
                    if ((yOff >= 0) && (yOff < outH))
                    {
                        f32 *wRow = weights + fy * fW;
                        f32 *outRow = pErr + inX + yOff;
                        for (u32 fx = 0; fx < fW; ++fx)
                        {
                            s32 xOff = inX - fW + 1 + fx;
                            if ((xOff >= 0) && (xOff < outW))
                            {
                                val += wRow[fx] * delta_neuron(outRow[xOff]);
                            }
                        }
                    }
                }
                
                inRow[inX] += val;
            }
        }
        
        weights += fW * fH;
        pErr += outW * outH;
        output += outW * outH;
    }
}

internal void
back_propagate(NeuralCake *network, Training *training, f32 *deltaWeights, f32 *deltaBiases)
{
    predict(network, training);
    
    TempMemory tempMem = temporary_memory(&network->arena);
    
    // NOTE(michiel): Backward pass
    f32 *dnw = deltaWeights + network->totalWeights;
    f32 *dnb = deltaBiases + network->totalBiases; 
    u32 maxNeurons = 0;
    
    for (u32 layerIdx = 0; layerIdx < network->layerCount; ++layerIdx)
    {
        u32 l = network->layerCount - layerIdx - 1;
        NeuralLayer *layer = network->layers + l;
        
        if (maxNeurons < layer->inputCount)
        {
            maxNeurons = layer->inputCount;
        }
        if (maxNeurons < layer->outputCount)
        {
            maxNeurons = layer->outputCount;
        }
        
        if (layer->kind == Neural_FeatureMap)
            {
                if (layer->featureMap.poolWidth && layer->featureMap.poolHeight)
                {
                    u32 interSize = layer->outputCount * layer->featureMap.poolWidth * layer->featureMap.poolHeight;
                    if (maxNeurons < interSize)
                    {
                        maxNeurons = interSize;
                    }
                }
            }
        }
    
    f32 *error = arena_allocate_array(&network->arena, f32, maxNeurons);
    f32 *prevError = arena_allocate_array(&network->arena, f32, maxNeurons);
    
    for (u32 layerIdx = 0; layerIdx < network->layerCount; ++layerIdx)
    {
        u32 l = network->layerCount - layerIdx - 1;
        NeuralLayer *layer = network->layers + l;
        
        switch (layer->kind)
        {
            case Neural_FullyConnected:
            {
                back_propagate_fully_connected(layer, training, layerIdx == 0,
                                               &dnw, &dnb, prevError, error);
            } break;
            
            case Neural_FeatureMap:
            {
                back_propagate_feature_map(layer, training, layerIdx == 0,
                                           &dnw, &dnb, prevError, error);
            } break;
            
            INVALID_DEFAULT_CASE;
        }
        
        f32 *t = prevError;
        prevError = error;
        error = t;
    }
    
    destroy_temporary(tempMem);
}

internal void
update_mini_batch(NeuralCake *network, TrainingSet training, f32 learningRate,
                  f32 lambda = 0.0f, u32 totalTrainingCount = 0)
{
    TempMemory temp = temporary_memory(&network->arena);
    
    f32 *nw = arena_allocate_array(&network->arena, f32, network->totalWeights);
    f32 *nb = arena_allocate_array(&network->arena, f32, network->totalBiases);
    
    for (u32 item = 0; item < training.count; ++item)
    {
        Training *sample = training.set + item;
        back_propagate(network, sample, nw, nb);
    }
    
    f32 mod = learningRate / (f32)training.count;
    f32 modReg = 1.0f;
    if (lambda > 0.0f)
    {
        i_expect(totalTrainingCount);
    modReg = 1.0f - learningRate * (lambda / (f32)totalTrainingCount);
    }
    
    for (u32 l = 0; l < network->layerCount; ++l)
    {
        NeuralLayer *layer = network->layers + l;
        switch (layer->kind)
        {
            case Neural_FullyConnected:
            {
                for (u32 i = 0; i < layer->inputCount * layer->outputCount; ++i)
                {
                    f32 weight = layer->fullyConnected.weights[i];
                    weight = modReg * weight - mod * (*nw++);
                    layer->fullyConnected.weights[i] = weight;
                }
                for (u32 i = 0; i < layer->outputCount; ++i)
                {
                    f32 bias = layer->fullyConnected.biases[i];
                    bias = bias - mod * (*nb++);
                    layer->fullyConnected.biases[i] = bias;
                }
            } break;
            
            case Neural_FeatureMap:
            {
                u32 featureSize = layer->featureMap.featureWidth * layer->featureMap.featureHeight * layer->featureMap.mapCount;
                
                for (u32 i = 0; i < featureSize; ++i)
                {
                    f32 weight = layer->featureMap.weights[i];
                    weight = modReg * weight - mod * (*nw++);
                    layer->featureMap.weights[i] = weight;
                }
                for (u32 i = 0; i < layer->featureMap.mapCount; ++i)
                {
                    f32 bias = layer->featureMap.biases[i];
                    bias = bias - mod * (*nb++);
                    layer->featureMap.biases[i] = bias;
                }
            } break;
            
            INVALID_DEFAULT_CASE;
        }
    }
    
    destroy_temporary(temp);
}

internal u32
eval_neuron(u32 count, f32 *targets, f32 *outputs)
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

internal u32
eval_neuron_single_value(u32 count, f32 *targets, f32 *outputs, f32 epsilon = 0.01f)
{
    u32 correct = 0;
    for (u32 i = 0; i < count; ++i)
    {
        if ((outputs[i] >= (targets[i] - epsilon)) &&
            (outputs[i] <= (targets[i] + epsilon)))
        {
            ++correct;
        }
    }
    
    return (correct > 0) ? 1 : 0;
}

internal u32
evaluate(NeuralCake *network, TrainingSet tests)
{
    u32 correct = 0;
    
    for (u32 item = 0; item < tests.count; ++item)
    {
        Training *test = tests.set + item;
        predict(network, test->inputCount, test->inputs);
        
        correct += eval_neuron(test->outputCount, test->outputs, network->outputs);
    }
    
    return correct;
}

internal void
stochastic_gradient_descent(RandomSeriesPCG *random, NeuralCake *network, 
                            u32 epochs, u32 miniBatchSize, f32 learningRate,
                              TrainingSet training, f32 lambda = 0.0f)
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
            update_mini_batch(network, batch, learningRate, lambda, training.count);
        }
    }
}
