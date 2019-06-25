#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

// repeat 36 [lt 10 pu fd 1 pd repeat 120 [fd 2 rt 3]]
struct Turtle
{
    v2 pos;
    v2 dir;
    b32 penDown;
};

internal v2
move_forward(Turtle *turtle, f32 amount)
{
    v2 newPos = turtle->pos + amount * turtle->dir;
    return newPos;
}

internal v2
move_backward(Turtle *turtle, f32 amount)
{
    return move_forward(turtle, -amount);
}

internal v2
turn_left(Turtle *turtle, f32 angleInDegree)
{
    f32 radians = deg2rad(angleInDegree);
    // [ cos(t) sin(t)] [dir.x]  = [cos(t)*dir.x + sin(t)*dir.y]
    // [-sin(t) cos(t)] [dir.y]    [-sin(t)*dir.x + cos(t)*dir.y]
    
    f32 cR = cos(radians);
    f32 sR = sin(radians);
    v2 newDir = V2( cR * turtle->dir.x + sR * turtle->dir.y,
                   -sR * turtle->dir.x + cR * turtle->dir.y);
    return newDir;
}

internal v2
turn_right(Turtle *turtle, f32 angleInDegree)
{
    return turn_left(turtle, -angleInDegree);
}

struct Parser
{
    String remaining;
    u32 index;
};

struct Command
{
    String name;
    f32 arg;
    f32 moreArg; // TODO(michiel): More generic please...
    u32 commandCount;
    Command *commands;
};

struct LogoState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    String commandInput;
    Turtle turtle;
    Parser parser;
    
    u32 maxCommandCount;
    u32 commandCount;
    Command *commands;
};

internal s32
parse_number(String number)
{
    s32 result = 0;
    b32 negative = number.data[0] == '-';
    if (negative)
    {
        ++number.data;
        --number.size;
    }
    
    while (number.size &&
           (number.data[0] >= '0') &&
           (number.data[0] <= '9'))
    {
        result *= 10;
        result += number.data[0] - '0';
        ++number.data;
        --number.size;
    }
    
    return negative ? -result : result;
}

internal String
get_next_token(Parser *parser)
{
    String result = {};
    if (parser->index < parser->remaining.size)
    {
        while ((parser->index < parser->remaining.size) &&
               is_whitespace(parser->remaining.data[parser->index++]))
        {
            
        }
        result.data = parser->remaining.data + parser->index - 1;
        ++result.size;
        if ((result.data[0] != '[') && (result.data[0] != ']'))
        {
            while ((parser->index < parser->remaining.size) &&
                   (parser->remaining.data[parser->index] != ']') &&
                   !is_whitespace(parser->remaining.data[parser->index++]))
            {
                ++result.size;
            }
        }
    }
    
    return result;
}

global const String forward = string("fd");
global const String backward = string("bd");
global const String leftTurn = string("lt");
global const String rightTurn = string("rt");
global const String penUp = string("pu");
global const String penDown = string("pd");
global const String repeat = string("repeat");
global const String bracketOpen = string("[");
global const String bracketClose = string("]");
global const String setPos = string("setpos");

internal u32
parse_commands(Parser *parser, u32 maxCommands, Command *commands)
{
    String token = get_next_token(parser);
    
    u32 commandCount = 0;
    
    while (token.size && (commandCount < maxCommands))
    {
        if ((token == forward) ||
            (token == backward) ||
            (token == leftTurn) ||
            (token == rightTurn))
        {
            Command *command = commands + commandCount++;
            command->name = token;
            command->arg = (f32)parse_number(get_next_token(parser));
        }
        else if ((token == penUp) ||
                 (token == penDown))
        {
            Command *command = commands + commandCount++;
            command->name = token;
        }
        else if (token == repeat)
        {
            Command *command = commands + commandCount++;
            command->name = token;
            command->arg = (f32)parse_number(get_next_token(parser));
            token = get_next_token(parser);
            i_expect(token == bracketOpen);
            command->commands = commands + commandCount;
            command->commandCount = parse_commands(parser, maxCommands - commandCount, command->commands);
            commandCount += command->commandCount;
        }
        else if (token == bracketClose)
        {
            break;
        }
        else if (token == setPos)
        {
            Command *command = commands + commandCount++;
            command->name = token;
            token = get_next_token(parser);
            i_expect(token == bracketOpen);
            command->arg = (f32)parse_number(get_next_token(parser));
            command->moreArg = (f32)parse_number(get_next_token(parser));
            token = get_next_token(parser);
            i_expect(token == bracketClose);
        }
        
        token = get_next_token(parser);
    }
    
    return commandCount;
}

internal void
execute_commands(Image *image, Turtle *turtle, u32 commandCount, Command *commands)
{
    for (u32 cIdx = 0; cIdx < commandCount; ++cIdx)
    {
        Command *command = commands + cIdx;
        
        if (command->name == forward)
        {
            v2 move = move_forward(turtle, command->arg);
            if (turtle->penDown)
            {
                draw_line(image, turtle->pos.x, turtle->pos.y, move.x, move.y, V4(1, 1, 1, 1));
            }
            turtle->pos = move;
        }
        else if (command->name == backward)
        {
            v2 move = move_backward(turtle, command->arg);
            if (turtle->penDown)
            {
                draw_line(image, turtle->pos.x, turtle->pos.y, move.x, move.y, V4(1, 1, 1, 1));
            }
            turtle->pos = move;
        }
        else if (command->name == leftTurn)
        {
            turtle->dir = turn_left(turtle, command->arg);
        }
        else if (command->name == rightTurn)
        {
            turtle->dir = turn_right(turtle, command->arg);
        }
        else if (command->name == penUp)
        {
            turtle->penDown = false;
        }
        else if (command->name == penDown)
        {
            turtle->penDown = true;
        }
        else if (command->name == repeat)
        {
            for (u32 loopIdx = 0; loopIdx < command->arg; ++loopIdx)
            {
                execute_commands(image, turtle, command->commandCount, command->commands);
            }
            cIdx += command->commandCount;
        }
        else if (command->name == setPos)
        {
            v2 pos = V2(command->arg, command->moreArg);
            turtle->pos = pos;
        }
    }
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(LogoState) <= state->memorySize);
    
    v2 size = V2((f32)image->width, (f32)image->height);
    
    LogoState *logo = (LogoState *)state->memory;
    if (!state->initialized)
    {
        // logo->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        logo->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        logo->commandInput.data = (u8 *)"setpos [400 500] repeat 10 [fd 100 lt 36]"; // "repeat 36 [lt 10 repeat 120 [fd 5 rt 3]]";
        logo->commandInput.size = string_length((char *)logo->commandInput.data);
        
        logo->turtle.pos = 0.5f * size;
        logo->turtle.dir = V2(1, 0);
        logo->turtle.penDown = true;
        
        logo->parser.remaining = logo->commandInput;
        logo->parser.index = 0;
        
        logo->maxCommandCount = 256;
        logo->commandCount = 0;
        logo->commands = allocate_array(Command, logo->maxCommandCount);
        
        logo->commandCount = parse_commands(&logo->parser, logo->maxCommandCount, logo->commands);
        
        state->initialized = true;
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    Turtle turtle = {};
    turtle.pos = 0.5f * size;
    turtle.dir = V2(1, 0);
    turtle.penDown = true;
    
    execute_commands(image, &turtle, logo->commandCount, logo->commands);
    
    logo->prevMouseDown = mouse.mouseDowns;
    logo->seconds += dt;
    ++logo->ticks;
    if (logo->seconds > 1.0f)
    {
        logo->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", logo->ticks,
                1000.0f / (f32)logo->ticks);
        logo->ticks = 0;
    }
}
