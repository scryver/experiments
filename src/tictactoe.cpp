#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "../libberdip/perlin.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

enum TicTacToePlayer
{
    Player_None,   // Uninitialized
    Player_X,      // X-player
    Player_O,      // O-player
    Player_Tie,    // Used for checkwinner return
};

enum TicTacToeState
{
    State_None,
    State_Started,
    State_PlayerInput,
    State_PlayerChange,
    State_EndOfGame,
};

struct GameState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    TicTacToePlayer currentPlayer;
    
    TicTacToeState state;
    u32 ticksToProceed;
    TicTacToeState nextState;
    
    u8 grid[9];
};

internal void
set_timeout(GameState *state, TicTacToeState toState, u32 timeoutTicks)
{
    state->ticksToProceed = timeoutTicks;
    state->nextState = toState;
}

internal TicTacToePlayer
get_tile(GameState *state, u32 row, u32 col)
{
    TicTacToePlayer result = (TicTacToePlayer)state->grid[row * 3 + col];
    return result;
}

internal b32
is_player(TicTacToePlayer player)
{
    b32 result = ((player == Player_X) ||
                  (player == Player_O));
    return result;
}

internal void
reset_game(GameState *state)
{
    copy_single(9, 0, state->grid);
    state->currentPlayer = Player_X;
}

internal b32
has_empty_spots(GameState *state)
{
    b32 result = false;
    for (u32 idx = 0; idx < 9; ++idx)
    {
        if (state->grid[idx] == Player_None) {
            result = true;
            break;
        }
    }
    return result;
}

internal TicTacToePlayer
check_winner(GameState *state)
{
    TicTacToePlayer result = Player_None;
    
    for (u32 idx = 0; idx < 3; ++idx)
    {
        TicTacToePlayer test = get_tile(state, idx, 0);
        if ((test == get_tile(state, idx, 1)) &&
            (test == get_tile(state, idx, 2)) &&
            is_player(test))
        {
            result = test;
        }
    }
    
    for (u32 idx = 0; idx < 3; ++idx)
    {
        TicTacToePlayer test = get_tile(state, 0, idx);
        if ((test == get_tile(state, 1, idx)) &&
            (test == get_tile(state, 2, idx)) &&
            is_player(test))
        {
            result = test;
        }
    }
    
    TicTacToePlayer test = get_tile(state, 1, 1);
    if (is_player(test) && 
        (((test == get_tile(state, 0, 0)) &&
          (test == get_tile(state, 2, 2))) ||
         ((test == get_tile(state, 0, 2)) &&
          (test == get_tile(state, 2, 0)))))
    {
        result = test;
    }
    
    if ((result == Player_None) && !has_empty_spots(state))
    {
        result = Player_Tie;
    }
    
    return result;
}

internal TicTacToePlayer
other_player(TicTacToePlayer current)
{
    TicTacToePlayer result = Player_None;
    switch (current)
    {
        case Player_X: { result = Player_O; } break;
        case Player_O: { result = Player_X; } break;
        INVALID_DEFAULT_CASE;
    }
    return result;
}

internal void
next_move_random(GameState *state)
{
    u8 spots[9];
    u32 spotCount = 0;
    for (u32 idx = 0; idx < 9; ++idx)
    {
        if (state->grid[idx] == Player_None) {
            spots[spotCount++] = idx;
        }
    }
    if (spotCount)
    {
        u32 choice = random_choice(&state->randomizer, spotCount);
        state->grid[spots[choice]] = state->currentPlayer;
    }
}

internal s32
get_score(TicTacToePlayer maximizer, TicTacToePlayer winner)
{
    s32 result = 0;
    if (is_player(winner))
    {
        result = winner == maximizer ? 1 : -1;
    }
    return result;
}

struct MiniMax
{
    s32 score;
    u32 index;
};

internal MiniMax
minimax(GameState *state, TicTacToePlayer current, b32 isMaximizing, u32 depth)
{
    MiniMax result = {};
    TicTacToePlayer winner = check_winner(state);
    if (winner != Player_None)
    {
        result.score = get_score(isMaximizing ? current : other_player(current), winner);
    }
    else
    {
        u32 bestIndex = U32_MAX;
        s32 bestScore = isMaximizing ? -2 : 2;
        for (u32 idx = 0; idx < 9; ++idx)
        {
            if (state->grid[idx] == Player_None)
            {
                state->grid[idx] = current;
                MiniMax score = minimax(state, other_player(current), !isMaximizing, depth + 1);
                state->grid[idx] = Player_None;
                if (isMaximizing)
                {
                    if (bestScore < score.score)
                    {
                        bestScore = score.score;
                        bestIndex = idx;
                    }
                }
                else
                {
                    if (bestScore > score.score)
                    {
                        bestScore = score.score;
                        bestIndex = idx;
                    }
                }
            }
        }
        if ((bestScore > -2) && (bestScore < 2) &&
            (bestIndex != U32_MAX))
        {
            result.score = bestScore;
            result.index = bestIndex;
        }
    }
    return result;
}

internal void
next_move_minimax(GameState *state)
{
    MiniMax nextMove = minimax(state, state->currentPlayer, true, 0);
    if (state->grid[nextMove.index] == Player_None)
    {
        state->grid[nextMove.index] = state->currentPlayer;
    }
    else
    {
        INVALID_CODE_PATH;
    }
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(GameState) <= state->memorySize);
    
    v2 size = V2((f32)image->width, (f32)image->height);
    
    GameState *game = (GameState *)state->memory;
    if (!state->initialized)
    {
        // game->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        game->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        game->nextState = State_Started;
        
        state->initialized = true;
    }
    
    //
    // NOTE(michiel): Update
    //
    
    if (game->ticksToProceed)
    {
        --game->ticksToProceed;
    }
    else
    {
        game->state = game->nextState;
        switch (game->state)
        {
            case State_Started: {
                reset_game(game);
                set_timeout(game, State_PlayerInput, 30);
            } break;
            
            case State_PlayerChange: {
                game->currentPlayer = other_player(game->currentPlayer);
                set_timeout(game, State_PlayerInput, 20);
            } break;
            
            case State_PlayerInput: {
                TicTacToePlayer winner = check_winner(game);
                if (winner != Player_None)
                {
                    set_timeout(game, State_EndOfGame, 120);
                }
                else
                {
                    if (random_unilateral(&game->randomizer) > 0.1f)
                    {
                        next_move_minimax(game);
                    }
                    else
                    {
                        next_move_random(game);
                    }
                    
                    set_timeout(game, State_PlayerChange, 20);
                }
            } break;
            
            case State_EndOfGame: {
                set_timeout(game, State_Started, 90);
            } break;
            
            INVALID_DEFAULT_CASE;
        }
    }
    
    //
    // NOTE(michiel): Render
    //
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    v2 tileSize;
    tileSize.x = tileSize.y = (size.height - 20.0f) / 3.0f;
    
    v2 start = {10, 10};
    v2 end = start + 3.0f * tileSize;
    Rectangle2 boardRect = rect_min_dim(start, 3.0f * tileSize);
    
    if (game->state == State_EndOfGame)
    {
        // NOTE(michiel): Draw board
        outline_rectangle(image, boardRect, V4(1, 1, 1, 0.01f));
        draw_line(image, start.x, start.y + tileSize.y, end.x, start.y + tileSize.y, V4(1, 1, 1, 0.1f));
        draw_line(image, start.x, start.y + 2.0f * tileSize.y, end.x, start.y + 2.0f * tileSize.y, V4(1, 1, 1, 0.1f));
        draw_line(image, start.x + tileSize.x, start.y, start.x + tileSize.x, end.y, V4(1, 1, 1, 0.1f));
        draw_line(image, start.x + 2.0f * tileSize.x, start.y, start.x + 2.0f * tileSize.x, end.y, V4(1, 1, 1, 0.1f));
        
        TicTacToePlayer winner = check_winner(game);
        switch (winner)
        {
            case Player_X:
            {
                draw_line(image, boardRect.min + 10.0f, boardRect.max - 10.0f, V4(1, 1, 0, 1));
                draw_line(image, boardRect.max.x - 10.0f, boardRect.min.y + 10.0f,
                          boardRect.min.x + 10.0f, boardRect.max.y - 10.0f, V4(1, 1, 0, 1));
            } break;
            
            case Player_O:
            {
                fill_circle(image, boardRect.min + 0.5f * get_dim(boardRect), 0.45f * 3.0f * tileSize.x, V4(1, 1, 0, 1));
                fill_circle(image, boardRect.min + 0.5f * get_dim(boardRect), 0.44f * 3.0f * tileSize.x, V4(0, 0, 0, 1));
            } break;
            
            case Player_Tie:
            {
                // NOTE(michiel): Draw players
                for (u32 row = 0; row < 3; ++row)
                {
                    for (u32 col = 0; col < 3; ++col)
                    {
                        TicTacToePlayer test = get_tile(game, row, col);
                        if (test == Player_X)
                        {
                            draw_line(image, start.x + col * tileSize.x + 10.0f, start.y + row * tileSize.y + 10.0f,
                                      start.x + (col + 1) * tileSize.x - 10.0f, start.y + (row + 1) * tileSize.y - 10.0f,
                                      V4(1, 1, 0, 1));
                            draw_line(image, start.x + (col + 1) * tileSize.x - 10.0f, start.y + row * tileSize.y + 10.0f,
                                      start.x + col * tileSize.x + 10.0f, start.y + (row + 1) * tileSize.y - 10.0f,
                                      V4(1, 1, 0, 1));
                        }
                        else if (test == Player_O)
                        {
                            fill_circle(image, start.x + (col + 0.5f) * tileSize.x, start.y + (row + 0.5f) * tileSize.y,
                                        0.45f * tileSize.x, V4(1, 1, 0, 1));
                            fill_circle(image, start.x + (col + 0.5f) * tileSize.x, start.y + (row + 0.5f) * tileSize.y,
                                        0.44f * tileSize.x, V4(0, 0, 0, 1));
                        }
                    }
                }
            } break;
            
            INVALID_DEFAULT_CASE;
        }
    }
    else
    {
        // NOTE(michiel): Draw board
        outline_rectangle(image, boardRect, V4(1, 1, 1, 0.2f));
        draw_line(image, start.x, start.y + tileSize.y, end.x, start.y + tileSize.y, V4(1, 1, 1, 1));
        draw_line(image, start.x, start.y + 2.0f * tileSize.y, end.x, start.y + 2.0f * tileSize.y, V4(1, 1, 1, 1));
        draw_line(image, start.x + tileSize.x, start.y, start.x + tileSize.x, end.y, V4(1, 1, 1, 1));
        draw_line(image, start.x + 2.0f * tileSize.x, start.y, start.x + 2.0f * tileSize.x, end.y, V4(1, 1, 1, 1));
        
        v4 colour = V4(1, 1, 0, 1);
        if ((game->nextState == State_EndOfGame) &&
            (game->seconds > 0.5f))
        {
            colour.a = 0.2f;
        }
        
        // NOTE(michiel): Draw players
        for (u32 row = 0; row < 3; ++row)
        {
            for (u32 col = 0; col < 3; ++col)
            {
                TicTacToePlayer test = get_tile(game, row, col);
                if (test == Player_X)
                {
                    draw_line(image, start.x + col * tileSize.x + 10.0f, start.y + row * tileSize.y + 10.0f,
                              start.x + (col + 1) * tileSize.x - 10.0f, start.y + (row + 1) * tileSize.y - 10.0f,
                              colour);
                    draw_line(image, start.x + (col + 1) * tileSize.x - 10.0f, start.y + row * tileSize.y + 10.0f,
                              start.x + col * tileSize.x + 10.0f, start.y + (row + 1) * tileSize.y - 10.0f,
                              colour);
                }
                else if (test == Player_O)
                {
                    fill_circle(image, start.x + (col + 0.5f) * tileSize.x, start.y + (row + 0.5f) * tileSize.y,
                                0.45f * tileSize.x, colour);
                    fill_circle(image, start.x + (col + 0.5f) * tileSize.x, start.y + (row + 0.5f) * tileSize.y,
                                0.44f * tileSize.x, V4(0, 0, 0, 1));
                }
            }
        }
    }
    
    game->prevMouseDown = mouse.mouseDowns;
    game->seconds += dt;
    ++game->ticks;
    if (game->seconds > 1.0f)
    {
        game->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", game->ticks,
                1000.0f / (f32)game->ticks);
        game->ticks = 0;
    }
}

