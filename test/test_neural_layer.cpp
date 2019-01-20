#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>     // PROT_*, MAP_*, munmap

#include "../libberdip/platform.h"
#include "../libberdip/std_file.c"
#include "../src/interface.h"
#include "test_interface.h"

#include "../libberdip/random.h"
#define MATRIX_TEST 1
#include "../src/matrix.h"
#include "../src/aitraining.h"
#include "../src/neuronlayer.h"

int main(int argc, char **argv)
{
    Training test = {};
    test.inputCount = 9;
    test.outputCount = 4;
    test.inputs = allocate_array(f32, test.inputCount);
    test.outputs = allocate_array(f32, test.outputCount);
    
    NeuralLayer layer = {};
    
    layer.kind = Neural_FeatureMap;
    layer.inputCount = test.inputCount;
    layer.inputs = test.inputs;
    layer.outputCount = test.outputCount;
    layer.outputs = allocate_array(f32, test.outputCount);
    
    layer.featureMap.mapCount = 1;
    layer.featureMap.inputWidth = 3;
    layer.featureMap.inputHeight = 3;
    layer.featureMap.featureWidth = 2;
    layer.featureMap.featureHeight = 2;
    
    layer.featureMap.weights = allocate_array(f32, 2 * 2);
    layer.featureMap.biases = allocate_array(f32, 1);
    
    // NOTE(michiel): Inputs
    test.inputs[0] = 0.2f;
    test.inputs[1] = 0.3f;
    test.inputs[2] = 0.4f;
    test.inputs[3] = -0.1f;
    test.inputs[4] = -0.2f;
    test.inputs[5] = -0.3f;
    test.inputs[6] = 0.5f;
    test.inputs[7] = 0.6f;
    test.inputs[8] = 0.1f;
    
    test.outputs[0] = 0.2f;
    test.outputs[1] = 0.3f;
    test.outputs[2] = 0.4f;
    test.outputs[3] = 0.6f;
    
    layer.featureMap.weights[0] = 0.11f;
    layer.featureMap.weights[1] = 0.22f;
    layer.featureMap.weights[2] = 0.33f;
    layer.featureMap.weights[3] = 0.44f;
    
    layer.featureMap.biases[0] = 0.0f;
    
    predict_layer(&layer);
    
    i_expect(test.inputs[0] == 0.2f);
    i_expect(test.inputs[1] == 0.3f);
    i_expect(test.inputs[2] == 0.4f);
    i_expect(test.inputs[3] == -0.1f);
    i_expect(test.inputs[4] == -0.2f);
    i_expect(test.inputs[5] == -0.3f);
    i_expect(test.inputs[6] == 0.5f);
    i_expect(test.inputs[7] == 0.6f);
    i_expect(test.inputs[8] == 0.1f);
    
    i_expect(test.outputs[0] == 0.2f);
    i_expect(test.outputs[1] == 0.3f);
    i_expect(test.outputs[2] == 0.4f);
    i_expect(test.outputs[3] == 0.6f);
    
    i_expect(layer.featureMap.weights[0] == 0.11f);
    i_expect(layer.featureMap.weights[1] == 0.22f);
    i_expect(layer.featureMap.weights[2] == 0.33f);
    i_expect(layer.featureMap.weights[3] == 0.44f);
    
    i_expect(layer.featureMap.biases[0] == 0.0f);
    
    f32 o0 = 0.11f *  0.2f + 0.22f *  0.3f + 0.33f * -0.1f + 0.44f * -0.2f;
    f32 o1 = 0.11f *  0.3f + 0.22f *  0.4f + 0.33f * -0.2f + 0.44f * -0.3f;
    f32 o2 = 0.11f * -0.1f + 0.22f * -0.2f + 0.33f *  0.5f + 0.44f *  0.6f;
    f32 o3 = 0.11f * -0.2f + 0.22f * -0.3f + 0.33f *  0.6f + 0.44f *  0.1f;
    i_expect(layer.outputs[0] == activate_neuron(o0));
    i_expect(layer.outputs[1] == activate_neuron(o1));
             i_expect(layer.outputs[2] == activate_neuron(o2));
                      i_expect(layer.outputs[3] == activate_neuron(o3));
    i_expect(almost_equal(layer.outputs[0], activate_neuron(-0.033f)));
    i_expect(almost_equal(layer.outputs[1], activate_neuron(-0.077f)));
             i_expect(almost_equal(layer.outputs[2], activate_neuron( 0.374f)));
    i_expect(almost_equal(layer.outputs[3], activate_neuron( 0.154f)));
    
    fprintf(stdout,
            "Outputs:\n"
            "[ [% .5f, % .5f],\n"
            "  [% .5f, % .5f] ]\n\n", layer.outputs[0], layer.outputs[1],
            layer.outputs[2], layer.outputs[3]);
    
    f32 *prevError = allocate_array(f32, 3 * 3);
    f32 *error = allocate_array(f32, 3 * 3);
    prevError[0] = layer.outputs[0] - test.outputs[0];
    prevError[1] = layer.outputs[1] - test.outputs[1];
    prevError[2] = layer.outputs[2] - test.outputs[2];
    prevError[3] = layer.outputs[3] - test.outputs[3];

    f32 *deltaw = allocate_array(f32, 2 * 2);
    f32 *deltab = allocate_array(f32, 1);
    
    f32 *dw = deltaw + 2 * 2;
    f32 *db = deltab + 1;
    back_propagate_feature_map(&layer, &test, false, &dw, &db, prevError, error);
    
    fprintf(stdout,
            "DB: % .5f, DW:\n"
            "[ [% .5f, % .5f],\n"
            "  [% .5f, % .5f] ]\n\n", deltab[0],
            deltaw[0], deltaw[1], deltaw[2], deltaw[3]);
    
    
    return 0;
}