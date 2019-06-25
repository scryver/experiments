#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"
#include "../libberdip/std_file.c"

#include "aitraining.h"
#include "mnist.h"

struct MnistParser
{
    RandomSeriesPCG randomizer;
    u32 ticks;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(MnistParser) <= state->memorySize);
    
    MnistParser *mnist = (MnistParser *)state->memory;
    if (!state->initialized)
    {
        //mnist->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        mnist->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        MnistSet train = parse_mnist(static_string("data/train-labels-idx1-ubyte"),
                                     static_string("data/train-images-idx3-ubyte"));
        MnistSet test = parse_mnist(static_string("data/t10k-labels-idx1-ubyte"), 
                                    static_string("data/t10k-images-idx3-ubyte"));
        
        if (1)
        {
            // NOTE(michiel): Export to f32
            
            i_expect(train.count == 60000);
            MnistSet validation = split_mnist(&train, 50000);
            i_expect(train.count == 50000);
            i_expect(validation.count == 10000);
            
            TrainingSet training = mnist_to_training(&train);
            
            TrainingSet tests = mnist_to_training(&test);
            
            TrainingSet validations = mnist_to_training(&validation);
            
            save_training(training, "data/mnist-f32train");
            save_training(tests, "data/mnist-f32test");
            save_training(validations, "data/mnist-f32validation");
        }
        
        fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
        
        draw_mnist(image, 10, 10, train.images);
        draw_mnist(image, 40, 10, train.images + 1);
        fprintf(stdout, "%d %d\n", *train.labels, *(train.labels + 1));
        
        draw_mnist(image, 10, 50, test.images);
        draw_mnist(image, 40, 50, test.images + 1);
        fprintf(stdout, "%d %d\n", *test.labels, *(test.labels + 1));
        
        state->initialized = true;
    }
    
    ++mnist->ticks;
}
