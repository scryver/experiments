#include "interface.h"
DRAW_IMAGE(draw_image);

#include "main.cpp"

#include "../libberdip/drawing.cpp"

enum GameStatus
{
    GameStatus_Serve,
    GameStatus_Playing,
    GameStatus_BallLost,
    GameStatus_GameOver,
};

struct GameState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;

    GameStatus status;

    Rectangle2 gameField;
    Rectangle2 paddle;
    Rectangle2 ball;
    f32 paddleSpeed;
    v2 ballSpeed;
};

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(GameState) <= state->memorySize);

    v2 size = V2((f32)image->width, (f32)image->height);

    GameState *gameState = (GameState *)state->memory;
    if (!state->initialized)
    {
        // gameState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        gameState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);

        state->initialized = true;

        gameState->gameField = rect_min_dim(V2(10, 10), size - V2(20, 20));
        gameState->paddle = rect_center_dim(V2(0.5f * image->width, image->height - 40), V2(80, 10));
        gameState->ball = rect_center_dim(V2(0.5f * image->width, image->height - 40 - 5 - 5), V2(10, 10));
    }

    if (gameState->status < GameStatus_GameOver)
    {
        f32 paddleAccel = 0.0f;
        if (is_down(keyboard, Key_Left)) {
            paddleAccel -= 0.75f * image->width;
        }
        if (is_down(keyboard, Key_Right)) {
            paddleAccel += 0.75f * image->width;
        }
        gameState->paddleSpeed += paddleAccel * dt;
        gameState->paddleSpeed = clamp(-(f32)image->width, gameState->paddleSpeed, (f32)image->width);
        f32 moveX = gameState->paddleSpeed * dt;
        gameState->paddle.min.x += moveX;
        gameState->paddle.max.x += moveX;
        if (gameState->paddle.min.x < gameState->gameField.min.x)
        {
            f32 diff = gameState->gameField.min.x - gameState->paddle.min.x;
            gameState->paddle.min.x += diff;
            gameState->paddle.max.x += diff;
            gameState->paddleSpeed = -gameState->paddleSpeed;
        }
        if (gameState->paddle.max.x > gameState->gameField.max.x)
        {
            f32 diff = gameState->gameField.max.x - gameState->paddle.max.x;
            gameState->paddle.min.x += diff;
            gameState->paddle.max.x += diff;
            gameState->paddleSpeed = -gameState->paddleSpeed;
        }
        gameState->paddleSpeed = 0.99f * gameState->paddleSpeed;
    }

    if (gameState->status == GameStatus_Serve)
    {
        gameState->ball = rect_center_dim(get_center(gameState->paddle) - V2(0, 10), V2(10, 10));
        gameState->ballSpeed = V2(0, 0);

        if (is_down(keyboard, Key_Space))
        {
            gameState->status = GameStatus_Playing;
            //gameState->ballSpeed = V2(2.0f * random_bilateral(&gameState->randomizer), -1.0f);
            gameState->ballSpeed = V2(gameState->paddleSpeed / (0.25f * image->width), -1.0f);
            gameState->ballSpeed = 300.0f * normalize(gameState->ballSpeed);
        }
    }
    else if (gameState->status == GameStatus_Playing)
    {
        gameState->ball.min += gameState->ballSpeed * dt;
        gameState->ball.max += gameState->ballSpeed * dt;
        // NOTE(michiel): Bounce on paddle
        if ((gameState->ball.max.y > gameState->paddle.min.y) &&
            (((gameState->ball.min.x < gameState->paddle.max.x) && (gameState->ball.min.x > gameState->paddle.min.x)) ||
             ((gameState->ball.max.x > gameState->paddle.min.x) && (gameState->ball.max.x < gameState->paddle.max.x))))
        {
            f32 diff = gameState->paddle.min.y - gameState->ball.max.y;
            gameState->ball.min.y += diff;
            gameState->ball.max.y += diff;
            gameState->ballSpeed = V2(4.0f * ((get_center(gameState->ball).x - get_center(gameState->paddle).x) / (0.5f * get_dim(gameState->paddle).x)), -1.0f);
            gameState->ballSpeed = 300.0f * normalize(gameState->ballSpeed);
            //gameState->ballSpeed.y = -gameState->ballSpeed.y;
            //gameState->ballSpeed.x = gameState->
        }

        // NOTE(michiel): Bounce on wall
        if (gameState->ball.min.x < gameState->gameField.min.x)
        {
            f32 diff = gameState->gameField.min.x - gameState->ball.min.x;
            gameState->ball.min.x += diff;
            gameState->ball.max.x += diff;
            gameState->ballSpeed.x = -gameState->ballSpeed.x;
        }
        if (gameState->ball.min.y < gameState->gameField.min.y)
        {
            f32 diff = gameState->gameField.min.y - gameState->ball.min.y;
            gameState->ball.min.y += diff;
            gameState->ball.max.y += diff;
            gameState->ballSpeed.y = -gameState->ballSpeed.y;
        }
        if (gameState->ball.max.x > gameState->gameField.max.x)
        {
            f32 diff = gameState->gameField.max.x - gameState->ball.max.x;
            gameState->ball.min.x += diff;
            gameState->ball.max.x += diff;
            gameState->ballSpeed.x = -gameState->ballSpeed.x;
        }
        if (gameState->ball.max.y > gameState->gameField.max.y)
        {
            f32 diff = gameState->gameField.max.y - gameState->ball.max.y;
            gameState->ball.min.y += diff;
            gameState->ball.max.y += diff;
            gameState->status = GameStatus_BallLost;
        }
    }
    else if (gameState->status == GameStatus_BallLost)
    {
        gameState->status = GameStatus_Serve;
    }

    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));

    outline_rectangle(image, gameState->gameField, V4(1, 1, 1, 1));
    fill_rectangle(image, gameState->paddle, V4(0, 1, 0, 1));
    fill_rectangle(image, gameState->ball, V4(1, 1, 0, 1));

    gameState->seconds += dt;
    ++gameState->ticks;
    if (gameState->seconds > 1.0f)
    {
        gameState->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", gameState->ticks,
                1000.0f / (f32)gameState->ticks);
        gameState->ticks = 0;
    }
}
