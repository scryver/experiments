#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "random.h"
#include "drawing.cpp"

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
        
        MnistSet set10k = parse_mnist("data/t10k-labels-idx1-ubyte", "data/t10k-images-idx3-ubyte");
        MnistSet setTrain = parse_mnist("data/train-labels-idx1-ubyte", "data/train-images-idx3-ubyte");
        
        fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
        
        draw_mnist(image, 10, 10, setTrain.images);
        draw_mnist(image, 40, 10, setTrain.images + 1);
        fprintf(stdout, "%d %d\n", *setTrain.labels, *(setTrain.labels + 1));
        
        draw_mnist(image, 10, 50, set10k.images);
        draw_mnist(image, 40, 50, set10k.images + 1);
        fprintf(stdout, "%d %d\n", *set10k.labels, *(set10k.labels + 1));
        
    state->initialized = true;
    }
    
    ++mnist->ticks;
}
