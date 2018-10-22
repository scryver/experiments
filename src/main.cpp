#include <errno.h>
#include <sys/stat.h>     // stat
#include <sys/mman.h>     // PROT_*, MAP_*, munmap
#include <dlfcn.h>        // dlopen, dlsym, dlclose
#include <unistd.h>       // usleep
#include <time.h>
//#include <linux/futex.h> // TODO(michiel): futex
#include <semaphore.h>
//#include <pthread.h>
#include <sched.h>

#include <stdio.h>        // fprintf
#include <stdlib.h>
#include <string.h>       // memset

// NOTE(michiel): X11 Windowing
#include <X11/Xlib.h>
#include <X11/Xatom.h>

// NOTE(michiel): OpenGL and OpenGL for X11
#include <GL/gl.h>
#include <GL/glx.h>

#define KEYCODE_ESCAPE 9

struct MemBlock
{
    u32 maxSize;
    u32 size;
    u8 *memory;
    
    // TODO(michiel): More memory
    //struct MemBlock *next;
};

global MemBlock memory;

struct TempMemory
{
    u32 origSize;
};

internal inline TempMemory
temporary_memory(void)
{
    TempMemory result = {};
    result.origSize = memory.size;
    return result;
}

internal inline void
destroy_temporary(TempMemory temp)
{
    memory.size = temp.origSize;
}

#define allocate_struct(type) (type *)allocate_size(sizeof(type))
#define allocate_array(type, count) (type *)allocate_size(sizeof(type) * (count))
internal u8 *
allocate_size(umm size)
{
    if (memory.maxSize == 0)
    {
        memory.maxSize = megabytes(512);
        memory.memory = (u8 *)mmap(0, memory.maxSize, PROT_READ|PROT_WRITE,
                                   MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    }
    
    i_expect(memory.size + size <= memory.maxSize);
    u8 *data = memory.memory + memory.size;
    memory.size += size;
    memset(data, 0, size);
    return data;
}

internal void
print_error(char *message)
{
    fprintf(stderr, "ERROR: %s\n", message);
}

struct ReadFile
{
    u32 size;
    u8 *data;
};

internal ReadFile
read_entire_file(char *filename)
{
    ReadFile result = {};
    
    FILE *f = fopen(filename, "rb");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        u64 size = ftell(f);
        fseek(f, 0, SEEK_SET);
        i_expect(size < U32_MAX);
        
        result.data = (u8 *)allocate_size(size);
        i_expect(result.data);
        result.size = size;
        
        fread(result.data, size, 1, f);
        fclose(f);
    }
    
    return result;
}

//
// NOTE(michiel): WINDOW RENDERING MAIN STUFF
//

#define OPENGL_GLOBAL_FUNCTION(name) global type_##name *name;
#define GET_GL_FUNCTION(name) name = (type_##name *)glXGetProcAddress((GLubyte *) #name)

typedef void   type_glXSwapIntervalEXT(Display *display, GLXDrawable drawable, int interval);

typedef GLXContext type_glXCreateContextAttribsARB(Display *dpy, GLXFBConfig config,
                                                   GLXContext shareContext,
                                                   Bool direct, const int *attribList);

typedef void   type_glGenBuffers(GLsizei n, GLuint *buffers);
typedef void   type_glBindBuffer(GLenum target, GLuint buffer);
typedef void   type_glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void   type_glDrawBuffers(GLsizei n, const GLenum *bufs);

typedef void   type_glGenVertexArrays(GLsizei n, GLuint *arrays);
typedef void   type_glBindVertexArray(GLuint array);

typedef GLuint type_glCreateShader(GLenum type);
typedef void   type_glShaderSource(GLuint shader, GLsizei count, GLchar **string, GLint *length);
typedef void   type_glCompileShader(GLuint shader);
typedef void   type_glAttachShader(GLuint program, GLuint shader);
typedef void   type_glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void   type_glDeleteShader(GLuint shader);

typedef GLuint type_glCreateProgram(void);
typedef void   type_glLinkProgram(GLuint program);
typedef void   type_glValidateProgram(GLuint program);
typedef void   type_glGetProgramiv(GLuint program, GLenum pname, GLint *params);
typedef void   type_glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void   type_glUseProgram(GLuint program);
typedef void   type_glDeleteProgram(GLuint program);

typedef GLint type_glGetAttribLocation(GLuint program, const GLchar *name);
typedef void type_glEnableVertexAttribArray(GLuint index);
typedef void type_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *offset);
typedef void type_glDisableVertexAttribArray(GLuint index);

typedef GLint  type_glGetUniformLocation(GLuint program, const GLchar *name);
typedef void   type_glUniform1i(GLint location, GLint v0);

OPENGL_GLOBAL_FUNCTION(glXCreateContextAttribsARB);
OPENGL_GLOBAL_FUNCTION(glXSwapIntervalEXT);

OPENGL_GLOBAL_FUNCTION(glGenBuffers);
OPENGL_GLOBAL_FUNCTION(glBindBuffer);
OPENGL_GLOBAL_FUNCTION(glBufferData);
OPENGL_GLOBAL_FUNCTION(glDrawBuffers);

OPENGL_GLOBAL_FUNCTION(glGenVertexArrays);
OPENGL_GLOBAL_FUNCTION(glBindVertexArray);

OPENGL_GLOBAL_FUNCTION(glCreateShader);
OPENGL_GLOBAL_FUNCTION(glShaderSource);
OPENGL_GLOBAL_FUNCTION(glCompileShader);
OPENGL_GLOBAL_FUNCTION(glAttachShader);
OPENGL_GLOBAL_FUNCTION(glGetShaderInfoLog);
OPENGL_GLOBAL_FUNCTION(glDeleteShader);

OPENGL_GLOBAL_FUNCTION(glCreateProgram);
OPENGL_GLOBAL_FUNCTION(glLinkProgram);
OPENGL_GLOBAL_FUNCTION(glValidateProgram);
OPENGL_GLOBAL_FUNCTION(glGetProgramiv);
OPENGL_GLOBAL_FUNCTION(glGetProgramInfoLog);
OPENGL_GLOBAL_FUNCTION(glUseProgram);
OPENGL_GLOBAL_FUNCTION(glDeleteProgram);

OPENGL_GLOBAL_FUNCTION(glGetAttribLocation);
OPENGL_GLOBAL_FUNCTION(glEnableVertexAttribArray);
OPENGL_GLOBAL_FUNCTION(glVertexAttribPointer);
OPENGL_GLOBAL_FUNCTION(glDisableVertexAttribArray);

OPENGL_GLOBAL_FUNCTION(glGetUniformLocation);
OPENGL_GLOBAL_FUNCTION(glUniform1i);

internal int
xlib_error_callback(Display *display, XErrorEvent *event)
{
    char errorMessage[256];
    errorMessage[0] = 0;
    XGetErrorText(display, event->error_code, errorMessage, sizeof(errorMessage));
    char *errorName;
    switch (event->error_code)
    {
        case BadAccess:         { errorName = "BadAccess"; } break;
        case BadAlloc:          { errorName = "BadAlloc"; } break;
        case BadAtom:           { errorName = "BadAtom"; } break;
        case BadColor:          { errorName = "BadColor"; } break;
        case BadCursor:         { errorName = "BadCursor"; } break;
        case BadDrawable:       { errorName = "BadDrawable"; } break;
        case BadFont:           { errorName = "BadFont"; } break;
        case BadGC:             { errorName = "BadGC"; } break;
        case BadIDChoice:       { errorName = "BadIDChoice"; } break;
        case BadImplementation: { errorName = "BadImplementation"; } break;
        case BadLength:         { errorName = "BadLength"; } break;
        case BadMatch:          { errorName = "BadMatch"; } break;
        case BadName:           { errorName = "BadName"; } break;
        case BadPixmap:         { errorName = "BadPixmap"; } break;
        case BadRequest:        { errorName = "BadRequest"; } break;
        case BadValue:          { errorName = "BadValue"; } break;
        case BadWindow:         { errorName = "BadWindow"; } break;
        default:                { errorName = "UNKNOWN"; } break;
    }
    print_error(errorMessage);
    return True;
}

struct Vertex 
{
    v2 pos;
    v2 uv;
};

internal inline struct timespec
get_wall_clock(void)
{
    struct timespec clock;
    clock_gettime(CLOCK_MONOTONIC, &clock);
    return clock;
}

internal inline f32
get_seconds_elapsed(struct timespec start, struct timespec end)
{
    return ((f32)(end.tv_sec - start.tv_sec)
            + ((f32)(end.tv_nsec - start.tv_nsec) * 1e-9f));
}

//
// NOTE(michiel): Multi threading
//

struct PlatformWorkQueue
{
    u32 volatile        completionGoal;
    u32 volatile        completionCount;
    
    u32 volatile        nextEntryToWrite;
    u32 volatile        nextEntryToRead;
    //s32 volatile        semaphore;
    sem_t               semaphoreHandle;
    
    PlatformWorkQueueEntry entries[MAX_WORK_QUEUE_ENTRIES];
};

internal void
platform_add_entry(PlatformWorkQueue *queue, PlatformWorkQueueCallback *callback,
                   void *data)
{
    u32 newNextEntryToWrite = (queue->nextEntryToWrite + 1) % MAX_WORK_QUEUE_ENTRIES;
    i_expect(newNextEntryToWrite != queue->nextEntryToRead);
    PlatformWorkQueueEntry *entry = queue->entries + queue->nextEntryToWrite;
    entry->callback = callback;
    entry->data = data;
    ++queue->completionGoal;
    
    asm volatile("" ::: "memory");
    
    queue->nextEntryToWrite = newNextEntryToWrite;
    sem_post(&queue->semaphoreHandle);
//__sync_fetch_and_add(&queue->semaphore, 1);
    // TODO(michiel): Complete random max processes!
       //futex(&queue->semaphore, FUTEX_WAKE, 8, 0, 0, 0);
    //i_expect(result == 1);
    }

internal b32
do_next_work_queue_entry(PlatformWorkQueue *queue)
{
    b32 weShouldSleep = false;
    
    u32 origNextEntryToRead = queue->nextEntryToRead;
    u32 newNextEntryToRead = (queue->nextEntryToRead + 1) % MAX_WORK_QUEUE_ENTRIES;
    if (origNextEntryToRead != queue->nextEntryToWrite)
    {
        u32 index = __sync_val_compare_and_swap(&queue->nextEntryToRead,
                                                origNextEntryToRead,
                                                newNextEntryToRead);
        if (index == origNextEntryToRead)
        {
            PlatformWorkQueueEntry entry = queue->entries[index];
            entry.callback(queue, entry.data);
            __sync_fetch_and_add(&queue->completionCount, 1);
        }
    }
    else
    {
        weShouldSleep = true;
    }
    
    return weShouldSleep;
}

internal void
platform_complete_all_work(PlatformWorkQueue *queue)
{
    while (queue->completionGoal != queue->completionCount)
    {
        do_next_work_queue_entry(queue);
    }
    
    queue->completionGoal = 0;
    queue->completionCount = 0;
}

int
thread_process(void *parameter)
{
    PlatformWorkQueue *queue = (PlatformWorkQueue *)parameter;
    
    for (;;)
    {
        if (do_next_work_queue_entry(queue))
        {
            //s32 origSamValue = queue->semaphore;
            //while (futex(&queue->semaphore, FUTEX_WAIT, origSamValue, NULL, 0, 0))
            //{
                //
            //}
            while (sem_wait(&queue->semaphoreHandle) == EINTR)
            {
                
            }
        }
    }
}

internal void
init_work_queue(PlatformWorkQueue *queue, u32 threadCount)
{
    queue->completionGoal = 0;
    queue->completionCount = 0;
    
    queue->nextEntryToWrite = 0;
    queue->nextEntryToRead = 0;
    
    s32 initialCount = -1;
    sem_init(&queue->semaphoreHandle, 0, initialCount);
    
    char name[16] = "Thread nr ";
    for (u32 threadIndex = 0; threadIndex < threadCount; ++threadIndex)
    {
        name[10] = '0' + threadIndex;
        name[11] = '\0';
        
         u8 *threadStack = allocate_size(THREAD_STACK_SIZE);
        int threadId = clone(thread_process, threadStack + THREAD_STACK_SIZE,
                             CLONE_THREAD|CLONE_SIGHAND|CLONE_FS|CLONE_VM|CLONE_FILES|CLONE_PTRACE,
                             queue);
        fprintf(stdout, "Thread: %d\n", threadId);
    }
}

int main(int argc, char **argv)
{
    int windowWidth = 800;
    int windowHeight = 600;
    
    Display *display;
    Window window;
    
    XInitThreads();
    
    display = XOpenDisplay(":0.0");
    if (!display)
    {
        print_error("Can't open display");
        return 1;
    }
    
    XSetErrorHandler(xlib_error_callback);
    
    GET_GL_FUNCTION(glXCreateContextAttribsARB);
    GET_GL_FUNCTION(glXSwapIntervalEXT);
    
    GET_GL_FUNCTION(glGenBuffers);
    GET_GL_FUNCTION(glBindBuffer);
    GET_GL_FUNCTION(glBufferData);
    GET_GL_FUNCTION(glDrawBuffers);
    
    GET_GL_FUNCTION(glGenVertexArrays);
    GET_GL_FUNCTION(glBindVertexArray);
    
    GET_GL_FUNCTION(glCreateShader);
    GET_GL_FUNCTION(glShaderSource);
    GET_GL_FUNCTION(glCompileShader);
    GET_GL_FUNCTION(glAttachShader);
    GET_GL_FUNCTION(glGetShaderInfoLog);
    GET_GL_FUNCTION(glDeleteShader);
    
    GET_GL_FUNCTION(glCreateProgram);
    GET_GL_FUNCTION(glLinkProgram);
    GET_GL_FUNCTION(glValidateProgram);
    GET_GL_FUNCTION(glGetProgramiv);
    GET_GL_FUNCTION(glGetProgramInfoLog);
    GET_GL_FUNCTION(glUseProgram);
    GET_GL_FUNCTION(glDeleteProgram);
    
    GET_GL_FUNCTION(glGetAttribLocation);
    GET_GL_FUNCTION(glEnableVertexAttribArray);
    GET_GL_FUNCTION(glVertexAttribPointer);
    GET_GL_FUNCTION(glDisableVertexAttribArray);
    
    GET_GL_FUNCTION(glGetUniformLocation);
    GET_GL_FUNCTION(glUniform1i);
    
    int visualAttribs[] =
    {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        GLX_DOUBLEBUFFER, True,
        None
    };
    
    int fbCount;
    GLXFBConfig *fbc = glXChooseFBConfig(display, DefaultScreen(display), visualAttribs, &fbCount);
    if (fbCount < 1)
    {
        print_error("Failed to get some frame buffer configs");
        return -1;
    }
    
    GLXFBConfig framebufferConfig = fbc[0];
    
    XFree(fbc);
    XVisualInfo *pvi = glXGetVisualFromFBConfig(display, framebufferConfig);
    if (!pvi)
    {
        print_error("Failed to choose a valid visual");
        return -1;
    }
    
    pvi->screen = DefaultScreen(display);
    
    XSetWindowAttributes attr = {};
    Window rootWindow = RootWindow(display, pvi->screen);
    attr.colormap = XCreateColormap(display, rootWindow, pvi->visual, AllocNone);
    attr.border_pixel = 0;
    
    attr.event_mask = (StructureNotifyMask | PropertyChangeMask |
                       PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
                       KeyPressMask | KeyReleaseMask);
    
    window = XCreateWindow(display, rootWindow,
                           20, 20, windowWidth, windowHeight,
                           0, pvi->depth, InputOutput, pvi->visual,
                           CWBorderPixel | CWColormap | CWEventMask, &attr);
    if (!window)
    {
        print_error("GL Window creation failed");
        return -1;
    }
    
    Atom wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteWindow, 1);
    
    // NOTE(michiel): And display the window on the screen
    XMapRaised(display, window);
    
    int openGLAttribs[] =
    {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB|GLX_CONTEXT_DEBUG_BIT_ARB,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        0,
    };
    
    GLXContext openGLContext =
        glXCreateContextAttribsARB(display, framebufferConfig, 0, true, openGLAttribs);
    if (!openGLContext)
    {
        print_error("No modern OpenGL support");
        return -1;
    }
    
    if (!glXMakeCurrent(display, window, openGLContext))
    {
        print_error("Couldn't set the OpenGL context as the current context");
        return -1;
    }
    
    Image *image = allocate_struct(Image);
    image->width = windowWidth;
    image->height = windowHeight;
    image->pixels = allocate_array(u32, windowWidth * windowHeight);
    
    State *state = allocate_struct(State);
    state->initialized = false;
    state->memorySize = 64 * 1024 * 1024;
    state->memory = (u8 *)mmap(0, state->memorySize, PROT_READ|PROT_WRITE,
                               MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    
    state->workQueue = allocate_struct(PlatformWorkQueue);
    init_work_queue(state->workQueue, 1);
    
    Mouse mouse = {};
    {
    Window retRoot;
    Window retChild;
    s32 rootX, rootY, winX, winY;
    u32 mask;
    if(XQueryPointer(display, window, &retRoot, &retChild, &rootX, &rootY,
					 &winX, &winY, &mask))
    {
        winX = maximum(0, winX);
        winY = maximum(0, winY);
            mouse.pixelPosition.x = (f32)winX;
            mouse.pixelPosition.y = (f32)winY;
    }
    }
    
    Vertex vertices[4] = 
    {
        // X      Y       U     V
        {{-1.0f, -1.0f}, {0.0f, 1.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 0.0f}},
        {{-1.0f,  1.0f}, {0.0f, 0.0f}},
    };
    u8 indices[6] = {0, 1, 2, 0, 2, 3};
    
    GLuint screenVBO;
    glGenBuffers(1, &screenVBO);
    glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    GLuint textureBuf;
    glGenTextures(1, &textureBuf);
    
    GLuint vertexArray;
    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);
    
    GLuint programID;
    GLuint vertexP;
    GLuint vertexUV;
    GLuint sampler;
    {
        GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
        GLchar *vertexShaderCode = R"DOO(#version 330
        in vec2 vertP;
        in vec2 vertUV;
        
        smooth out vec2 fragUV;
        
        void main(void)
        {
             gl_Position = vec4(vertP, 0.0, 1.0);
             
             fragUV = vertUV;
}
        )DOO";
        glShaderSource(vertexShaderID, 1, &vertexShaderCode, 0);
        glCompileShader(vertexShaderID);
        
        GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
        GLchar *fragmentShaderCode = R"DOO(#version 330
        //todo: uniform float brightness
        uniform sampler2D textureSampler;
        
        smooth in vec2 fragUV;
        
        out vec4 pixelColour;
        
        void main(void)
        {
        vec4 textureColour = texture(textureSampler, fragUV);
        pixelColour = textureColour;
}
        )DOO";
        glShaderSource(fragmentShaderID, 1, &fragmentShaderCode, 0);
        glCompileShader(fragmentShaderID);
        
        programID = glCreateProgram();
        glAttachShader(programID, vertexShaderID);
        glAttachShader(programID, fragmentShaderID);
        glLinkProgram(programID);
        
        glValidateProgram(programID);
        GLint linked = false;
        glGetProgramiv(programID, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            fprintf(stderr, "Error in linking shader code\n");
            
            GLsizei ignored;
            char vertexErrors[4096];
            char fragmentErrors[4096];
            char programErrors[4096];
            vertexErrors[0] = 0;
            fragmentErrors[0] = 0;
            programErrors[0] = 0;
            glGetShaderInfoLog(vertexShaderID, sizeof(vertexErrors), &ignored, vertexErrors);
            glGetShaderInfoLog(fragmentShaderID, sizeof(fragmentErrors), &ignored, fragmentErrors);
            glGetProgramInfoLog(programID, sizeof(programErrors), &ignored, programErrors);
            
            fprintf(stderr, "VERTEX:\n%s\n\n", vertexErrors);
            fprintf(stderr, "FRAGMENT:\n%s\n\n", fragmentErrors);
            fprintf(stderr, "PROGRAM:\n%s\n\n", programErrors);
            print_error("Shader validation failed");
            return 1;
        }
        
        glDeleteShader(vertexShaderID);
        glDeleteShader(fragmentShaderID);
        
        vertexP = glGetAttribLocation(programID, "vertP");
        vertexUV = glGetAttribLocation(programID, "vertUV");
        sampler = glGetUniformLocation(programID, "textureSampler");
    }
    
    glUseProgram(programID);
    
    struct timespec lastTime = get_wall_clock();
    
    glXSwapIntervalEXT(display, window, 1);
    
    bool isRunning = true;
    while (isRunning)
    {
        while (XPending(display))
        {
            XEvent event;
            XNextEvent(display, &event);
            
            switch (event.type)
            {
                case ConfigureNotify:
                {
                    windowWidth = event.xconfigure.width;
                    windowHeight = event.xconfigure.height;
                } break;
                
                case DestroyNotify:
                {
                    isRunning = false;
                } break;
                
                case ClientMessage:
                {
                    if ((Atom)event.xclient.data.l[0] == wmDeleteWindow)
                    {
                        isRunning = false;
                    }
                } break;
                
                case MotionNotify:
                {
                    // NOTE(michiel): Mouse update event.xmotion.x/y
                    mouse.pixelPosition.x = (f32)event.xmotion.x;
                    mouse.pixelPosition.y = (f32)event.xmotion.y;
                } break;
                
                case ButtonPress:
                case ButtonRelease:
                {
                    if (event.type == ButtonPress)
                    {
                        if (event.xbutton.button == 1)
                        {
                            mouse.mouseDowns |= Mouse_Left;
                        }
                        else if (event.xbutton.button == 2) // TODO(michiel): Fix mouse: 2)
                        {
                            mouse.mouseDowns |= Mouse_Middle;
                        }
                        else if (event.xbutton.button == 3)
                        {
                            mouse.mouseDowns |= Mouse_Right;
                        }
                        else if (event.xbutton.button == 8)
                        {
                            mouse.mouseDowns |= Mouse_Extended1;
                        }
                        else if (event.xbutton.button == 9)
                        {
                            mouse.mouseDowns |= Mouse_Extended2;
                        }
                    }
                    else
                    {
                        if (event.xbutton.button == 1)
                        {
                            mouse.mouseDowns &= ~Mouse_Left;
                        }
                        else if (event.xbutton.button == 2)
                        {
                            mouse.mouseDowns &= ~Mouse_Middle;
                        }
                        else if (event.xbutton.button == 3)
                        {
                            mouse.mouseDowns &= ~Mouse_Right;
                        }
                        else if (event.xbutton.button == 8)
                        {
                            mouse.mouseDowns &= ~Mouse_Extended1;
                        }
                        else if (event.xbutton.button == 9)
                        {
                            mouse.mouseDowns &= ~Mouse_Extended2;
                        }
                    }
                    // NOTE(michiel): Mouse press/release event.xbutton.button
                } break;
                
                case KeyPress:
                case KeyRelease:
                {
                    // NOTE(michiel): Key press/release event.xkey.keycode
                    if ((event.type == KeyPress) &&
                        (event.xkey.keycode == KEYCODE_ESCAPE))
                    {
                        isRunning = false;
                    }
                } break;
                
                default: break;
            }
        }
        
        struct timespec now = get_wall_clock();
        f32 secondsElapsed = get_seconds_elapsed(lastTime, now);
        lastTime = now;
        
        draw_image(state, image, mouse, secondsElapsed);
        
        glViewport(0, 0, windowWidth, windowHeight);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(programID);
        glEnableVertexAttribArray(vertexP);
        glVertexAttribPointer(vertexP, 2, GL_FLOAT, false, sizeof(Vertex), (void *)0);
        glEnableVertexAttribArray(vertexUV);
        glVertexAttribPointer(vertexUV, 2, GL_FLOAT, false, sizeof(Vertex), (void *)(2 * sizeof(f32)));
        glUniform1i(sampler, 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureBuf);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image->width, image->height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, (void *)image->pixels);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        
        glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
        glXSwapBuffers(display, window);
    }
    
    return 0;
}
