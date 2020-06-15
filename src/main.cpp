#ifndef MAIN_WINDOW_WIDTH
#define MAIN_WINDOW_WIDTH 800
#endif

#ifndef MAIN_WINDOW_HEIGHT
#define MAIN_WINDOW_HEIGHT 600
#endif

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

internal void
print_error(char *message)
{
    fprintf(stderr, "ERROR: %s\n", message);
}

// TODO(michiel): Swap out different libs and reload, so one executable for all the programs
// maybe with text file to say which one, or F5/F7 for forward/backward through the libs

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

internal struct timespec
get_wall_clock(void)
{
    struct timespec clock;
    clock_gettime(CLOCK_MONOTONIC, &clock);
    return clock;
}

internal f32
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
        
        u8 *threadStack = (u8 *)allocate_size(THREAD_STACK_SIZE);
        int threadId = clone(thread_process, threadStack + THREAD_STACK_SIZE,
                             CLONE_THREAD|CLONE_SIGHAND|CLONE_FS|CLONE_VM|CLONE_FILES|CLONE_PTRACE,
                             queue);
        fprintf(stdout, "Thread: %d\n", threadId);
    }
}

internal void
reset_keyboard_state(Keyboard *keyboard)
{
    keyboard->lastInput.size = 0;
    keyboard->lastInput.data = 0;
    for (u32 i = 0; i < array_count(keyboard->keys); ++i) {
        Key *key = keyboard->keys + i;
        key->isPressed = false;
        key->isReleased = false;
        key->edgeCount = 0;
    }
}

internal inline void
process_key(Keyboard *keyboard, Keys key, b32 isPressed)
{
    Key *k = keyboard->keys + (u32)key;
    if (isPressed && (!k->isDown))
    {
        k->isDown = true;
        k->isPressed = true;
        ++k->edgeCount;
    }
    else if (!isPressed && (k->isDown))
    {
        k->isDown = false;
        k->isReleased = true;
        ++k->edgeCount;
    }
}
global Keys gX11toKeys[] = {
    [XK_BackSpace] = Key_Backspace,
    [XK_Tab] = Key_Tab,
    [XK_Return] = Key_Enter,
    [XK_Scroll_Lock] = Key_ScrollLock,
    [XK_Escape] = Key_Escape,
    [XK_Delete] = Key_Delete,
    [XK_Home] = Key_Home,
    [XK_Left] = Key_Left,
    [XK_Up] = Key_Up,
    [XK_Right] = Key_Right,
    [XK_Down] = Key_Down,
    [XK_Page_Up] = Key_PageUp,
    [XK_Page_Down] = Key_PageDown,
    [XK_End] = Key_End,
    [XK_Insert] = Key_Insert,
    [XK_Num_Lock] = Key_NumLock,
    [XK_KP_Enter] = Key_NumEnter,
    [XK_KP_Home] = Key_Num7,
    [XK_KP_Left] = Key_Num4,
    [XK_KP_Up] = Key_Num8,
    [XK_KP_Right] = Key_Num6,
    [XK_KP_Down] = Key_Num2,
    [XK_KP_Page_Up] = Key_Num9,
    [XK_KP_Page_Down] = Key_Num3,
    [XK_KP_End] = Key_Num1,
    [XK_KP_Insert] = Key_Num0,
    [XK_KP_Delete] = Key_NumDot,
    [XK_KP_Equal] = Key_NumAdd,
    [XK_KP_Multiply] = Key_NumMultiply,
    [XK_KP_Add] = Key_NumAdd,
    [XK_KP_Subtract] = Key_NumSubtract,
    [XK_KP_Decimal] = Key_NumDot,
    [XK_KP_Divide] = Key_NumDivide,
    [XK_KP_0] = Key_Num0,
    [XK_KP_1] = Key_Num1,
    [XK_KP_2] = Key_Num2,
    [XK_KP_3] = Key_Num3,
    [XK_KP_4] = Key_Num4,
    [XK_KP_5] = Key_Num5,
    [XK_KP_6] = Key_Num6,
    [XK_KP_7] = Key_Num7,
    [XK_KP_8] = Key_Num8,
    [XK_KP_9] = Key_Num9,
    [XK_F1] = Key_F1,
    [XK_F2] = Key_F2,
    [XK_F3] = Key_F3,
    [XK_F4] = Key_F4,
    [XK_F5] = Key_F5,
    [XK_F6] = Key_F6,
    [XK_F7] = Key_F7,
    [XK_F8] = Key_F8,
    [XK_F9] = Key_F9,
    [XK_F10] = Key_F10,
    [XK_F11] = Key_F11,
    [XK_F12] = Key_F12,
    [XK_Caps_Lock] = Key_CapsLock,
    [XK_space] = Key_Space,
    [XK_exclam] = Key_Bang,
    [XK_quotedbl] = Key_DoubleQuote,
    [XK_numbersign] = Key_Hash,
    [XK_dollar] = Key_Dollar,
    [XK_percent] = Key_Percent,
    [XK_ampersand] = Key_Ampersand,
    [XK_apostrophe] = Key_Quote,
    [XK_parenleft] = Key_LeftParen,
    [XK_parenright] = Key_RightParen,
    [XK_asterisk] = Key_Asterisk,
    [XK_plus] = Key_Plus,
    [XK_comma] = Key_Comma,
    [XK_minus] = Key_Minus,
    [XK_period] = Key_Dot,
    [XK_slash] = Key_Slash,
    [XK_0] = Key_0,
    [XK_1] = Key_1,
    [XK_2] = Key_2,
    [XK_3] = Key_3,
    [XK_4] = Key_4,
    [XK_5] = Key_5,
    [XK_6] = Key_6,
    [XK_7] = Key_7,
    [XK_8] = Key_8,
    [XK_9] = Key_9,
    [XK_colon] = Key_Colon,
    [XK_semicolon] = Key_SemiColon,
    [XK_less] = Key_Less,
    [XK_equal] = Key_Equal,
    [XK_greater] = Key_Greater,
    [XK_question] = Key_Question,
    [XK_at] = Key_At,
    [XK_A] = Key_A,
    [XK_B] = Key_B,
    [XK_C] = Key_C,
    [XK_D] = Key_D,
    [XK_E] = Key_E,
    [XK_F] = Key_F,
    [XK_G] = Key_G,
    [XK_H] = Key_H,
    [XK_I] = Key_I,
    [XK_J] = Key_J,
    [XK_K] = Key_K,
    [XK_L] = Key_L,
    [XK_M] = Key_M,
    [XK_N] = Key_N,
    [XK_O] = Key_O,
    [XK_P] = Key_P,
    [XK_Q] = Key_Q,
    [XK_R] = Key_R,
    [XK_S] = Key_S,
    [XK_T] = Key_T,
    [XK_U] = Key_U,
    [XK_V] = Key_V,
    [XK_W] = Key_W,
    [XK_X] = Key_X,
    [XK_Y] = Key_Y,
    [XK_Z] = Key_Z,
    [XK_bracketleft] = Key_LeftBracket,
    [XK_backslash] = Key_BackSlash,
    [XK_bracketright] = Key_RightBracket,
    [XK_asciicircum] = Key_RoofIcon,
    [XK_underscore] = Key_Underscore,
    [XK_grave] = Key_Grave,
    [XK_a] = Key_A,
    [XK_b] = Key_B,
    [XK_c] = Key_C,
    [XK_d] = Key_D,
    [XK_e] = Key_E,
    [XK_f] = Key_F,
    [XK_g] = Key_G,
    [XK_h] = Key_H,
    [XK_i] = Key_I,
    [XK_j] = Key_J,
    [XK_k] = Key_K,
    [XK_l] = Key_L,
    [XK_m] = Key_M,
    [XK_n] = Key_N,
    [XK_o] = Key_O,
    [XK_p] = Key_P,
    [XK_q] = Key_Q,
    [XK_r] = Key_R,
    [XK_s] = Key_S,
    [XK_t] = Key_T,
    [XK_u] = Key_U,
    [XK_v] = Key_V,
    [XK_w] = Key_W,
    [XK_x] = Key_X,
    [XK_y] = Key_Y,
    [XK_z] = Key_Z,
    [XK_braceleft] = Key_LeftBrace,
    [XK_bar] = Key_Pipe,
    [XK_braceright] = Key_RightBrace,
    [XK_asciitilde] = Key_Tilde,
};

internal void
process_keyboard(Keyboard *keyboard, XEvent *event)
{
    persist char keyBuffer[1024];
    keyBuffer[0] = 0;
    
    KeySym sym;
    
    if (event && keyboard)
    {
        
        s32 count = XLookupString((XKeyEvent *)event, keyBuffer, array_count(keyBuffer),
                                  &sym, NULL);
        keyBuffer[count] = 0;
        if (count == 0)
        {
            char *tmp = XKeysymToString(sym);
            if (tmp)
            {
                copy(string_length(tmp), tmp, keyBuffer);
            }
        }
        
        keyboard->lastInput = string_fmt(array_count(keyboard->lastInputData),
                                         keyboard->lastInputData, "%.*s%s",
                                         STR_FMT(keyboard->lastInput), keyBuffer);
        
        b32 isPressed = event->type == KeyPress;
        switch (event->xkey.keycode)
        {
            // NOTE(michiel): Raw scancodes
            case 25: { process_key(keyboard, Key_GameUp, isPressed); } break;
            case 39: { process_key(keyboard, Key_GameDown, isPressed); } break;
            case 38: { process_key(keyboard, Key_GameLeft, isPressed); } break;
            case 40: { process_key(keyboard, Key_GameRight, isPressed); } break;
            case 24: { process_key(keyboard, Key_GameLeftUp, isPressed); } break;
            case 26: { process_key(keyboard, Key_GameRightUp, isPressed); } break;
        }
        
        switch (sym)
        {
            // NOTE(michiel): Interpreted by X11, don't know if I like doing it like this...
            case XK_Shift_L: {
                if (isPressed) {
                    keyboard->modifiers |= KeyMod_LeftShift;
                } else {
                    keyboard->modifiers &= ~KeyMod_LeftShift;
                }
            } break;
            case XK_Shift_R: {
                if (isPressed) {
                    keyboard->modifiers |= KeyMod_RightShift;
                } else {
                    keyboard->modifiers &= ~KeyMod_RightShift;
                }
            } break;
            
            case XK_Control_L: {
                if (isPressed) {
                    keyboard->modifiers |= KeyMod_LeftCtrl;
                } else {
                    keyboard->modifiers &= ~KeyMod_LeftCtrl;
                }
            } break;
            case XK_Control_R: {
                if (isPressed) {
                    keyboard->modifiers |= KeyMod_RightCtrl;
                } else {
                    keyboard->modifiers &= ~KeyMod_RightCtrl;
                }
            } break;
            
            case XK_Alt_L: {
                if (isPressed) {
                    keyboard->modifiers |= KeyMod_LeftAlt;
                } else {
                    keyboard->modifiers &= ~KeyMod_LeftAlt;
                }
            } break;
            case XK_Alt_R: {
                if (isPressed) {
                    keyboard->modifiers |= KeyMod_RightAlt;
                } else {
                    keyboard->modifiers &= ~KeyMod_RightAlt;
                }
            } break;
            
            default: {
                Keys key = gX11toKeys[sym];
                if (key != Key_None) {
                    process_key(keyboard, key, isPressed);
                } else {
                    fprintf(stdout, "Unhandled key: %u\n", event->xkey.keycode);
                }
            } break;
        }
    }
}

int main(int argc, char **argv)
{
    int windowWidth = MAIN_WINDOW_WIDTH;
    int windowHeight = MAIN_WINDOW_HEIGHT;
    
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
                           0, 0, windowWidth, windowHeight,
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
    //image->rowPitch = windowWidth;
    image->pixels = allocate_array(u32, windowWidth * windowHeight);
    
    State *state = allocate_struct(State);
    state->initialized = false;
    state->closeProgram = false;
    state->memorySize = 64 * 1024 * 1024;
    state->memory = (u8 *)mmap(0, state->memorySize, PROT_READ|PROT_WRITE,
                               MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    
    state->workQueue = allocate_struct(PlatformWorkQueue);
    init_work_queue(state->workQueue, 4);
    
    Mouse mouse = {};
    Keyboard keyboard = {};
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
            mouse.pixelPosition.x = winX;
            mouse.pixelPosition.y = winY;
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
    
    if (glXSwapIntervalEXT)
    {
        glXSwapIntervalEXT(display, window, 1);
    }
    
    bool isRunning = true;
    while (isRunning)
    {
        reset_keyboard_state(&keyboard);
        
        while (XPending(display))
        {
            XEvent event;
            XNextEvent(display, &event);
            
            // NOTE(michiel): Skip the auto repeat key
            if ((event.type == ButtonRelease) &&
                (XEventsQueued(display, QueuedAfterReading)))
            {
                XEvent nevent;
                XPeekEvent(display, &nevent);
                if ((nevent.type == ButtonPress) &&
                    (nevent.xbutton.time == event.xbutton.time) &&
                    (nevent.xbutton.button == event.xbutton.button))
                {
                    continue;
                }
            }
            
            switch (event.type)
            {
                case ConfigureNotify:
                {
                    windowWidth = event.xconfigure.width;
                    windowHeight = event.xconfigure.height;
                    mouse.relativePosition.x = (f32)mouse.pixelPosition.x / (f32)windowWidth;
                    mouse.relativePosition.y = (f32)mouse.pixelPosition.y / (f32)windowHeight;
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
                    mouse.pixelPosition.x = event.xmotion.x;
                    mouse.pixelPosition.y = event.xmotion.y;
                    mouse.relativePosition.x = (f32)mouse.pixelPosition.x / (f32)windowWidth;
                    mouse.relativePosition.y = (f32)mouse.pixelPosition.y / (f32)windowHeight;
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
                        else if (event.xbutton.button == 4)
                        {
                            ++mouse.scroll;
                        }
                        else if (event.xbutton.button == 5)
                        {
                            --mouse.scroll;
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
                    process_keyboard(&keyboard, &event);
                } break;
                
                default: break;
            }
        }
        
        struct timespec now = get_wall_clock();
        f32 secondsElapsed = get_seconds_elapsed(lastTime, now);
        lastTime = now;
        
        draw_image(state, image, mouse, &keyboard, secondsElapsed);
        isRunning = !state->closeProgram;
        
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
                     GL_BGRA, GL_UNSIGNED_BYTE, (void *)image->pixels);
        
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
