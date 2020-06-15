#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "../libberdip/perlin.h"
#include "../libberdip/complex.h"
#include "../libberdip/multilane.h"
//#include "complex.h"
#include "multithread.h"

struct State
{
    b32 initialized;
    u32 memorySize;
    u8 *memory;
    
    b32 closeProgram;
    
    PlatformWorkQueue *workQueue;
};

#define DRAW_IMAGE(name) void name(State *state, Image *image, Mouse mouse, Keyboard *keyboard, f32 dt)
typedef DRAW_IMAGE(DrawImage);
