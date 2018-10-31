struct Neurote
{
    u32 layerCount;
    u32 *layers;
    u32 *weightOffsets;
    u32 *biasOffsets;
    
    f32 *weights;
    f32 *biases;
    };

internal void
init_neural_network(Neurote *network, u32 layerCount, u32 *layers)
{
    i_expect(layerCount >= 2);
    
    i_expect(network->layers == 0);
    i_expect(network->weights == 0);
    i_expect(network->biases == 0);
    
    network->layerCount = layerCount;
    network->layers = allocate_array(u32, layerCount);
    network->weightOffsets = allocate_array(u32, layerCount - 1);
    network->biasOffsets = allocate_array(u32, layerCount - 1);
    
    u32 totalBiasCount = 0;
    u32 totalWeightCount = 0;
    
    u32 prevCount = 0;
    for (u32 layerIdx = 0; layerIdx < layerCount; ++layerIdx)
    {
        u32 count = layers[layerIdx];
        network->layers[layerIdx] = count;
        
        if (prevCount)
        {
            network->weightOffsets[layerIdx - 1] = totalWeightCount;
            network->biasOffsets[layerIdx - 1] = totalBiasCount;
            
            totalBiasCount += count;
        totalWeightCount += count * prevCount;
        }
        
        prevCount = count;
    }
    
    network->weights = allocate_array(f32, totalWeightCount);
    network->biases = allocate_array(f32, totalBiasCount);
}

internal void
randomize_weights(RandomSeriesPCG *random, Neurote *network)
{
    for (u32 layerIdx = 0; layerIdx < network->layerCount - 1; ++layerIdx)
    {
        u32 layerCount = network->layers[layerIdx + 1];
        u32 weightOffset = network->weightOffsets[layerIdx];
        u32 biasOffset = network->biasOffsets[layerIdx];
        
        for (u32 weightIdx = weightOffset;
             weightIdx < weightOffset + layerCount * network->layers[layerIdx];
             ++weightIdx)
        {
            network->weights[weightIdx] = slow_gaussian(random);
        }
    }
}