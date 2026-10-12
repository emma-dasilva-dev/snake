#include "raylib.h"
#include <stdbool.h>

#define COLS 30
#define ROWS 20
#define CELL 24
#define BOARD_X 32
#define BOARD_Y 108
#define WIDTH (BOARD_X * 2 + COLS * CELL)
#define HEIGHT (BOARD_Y + ROWS * CELL + 76)
#define MAX_CELLS (COLS * ROWS)
#define MAX_LEVEL 12

typedef enum { UP, DOWN, LEFT, RIGHT } Direction;
typedef enum { MENU, PLAYING, PAUSED, LOST, WON } Screen;

typedef struct {
    Vector2 body[MAX_CELLS];  // body[0] is the head; coordinates are grid cells.
    Vector2 food;
    int length, level, score;
    Direction direction, nextDirection;
    bool turnQueued;
    float accumulator;
    Screen screen;
} Game;

static bool same(Vector2 a, Vector2 b) { return a.x == b.x && a.y == b.y; }

static bool onSnake(const Game *g, Vector2 p, int count) {
    for (int i = 0; i < count; i++) if (same(g->body[i], p)) return true;
    return false;
}

static void spawnFood(Game *g) {
    // Select uniformly among free cells; never place food on the snake.
    int freeCells = MAX_CELLS - g->length;
    if (freeCells <= 0) { g->screen = WON; return; }
    int target = GetRandomValue(0, freeCells - 1);
    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++) {
            Vector2 p = {(float)x, (float)y};
            if (!onSnake(g, p, g->length) && target-- == 0) {
                g->food = p;
                return;
            }
        }
    }
}

static void reset(Game *g) {
    *g = (Game){0};
    g->length = 4;
    g->level = 1;
    g->direction = LEFT;
    g->nextDirection = LEFT;
    g->screen = MENU;
    for (int i = 0; i < g->length; i++)
        g->body[i] = (Vector2){(float)(COLS / 2 + i), (float)(ROWS / 2)};
    spawnFood(g);
}

static bool opposite(Direction a, Direction b) {
    return (a == UP && b == DOWN) || (a == DOWN && b == UP) ||
           (a == LEFT && b == RIGHT) || (a == RIGHT && b == LEFT);
}

static void input(Game *g) {
    if (g->screen == MENU || g->screen == LOST || g->screen == WON) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            reset(g);
            g->screen = PLAYING;
        }
        return;
    }
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE)) {
        g->screen = g->screen == PLAYING ? PAUSED : PLAYING;
        g->accumulator = 0.0f;
    }
    if (g->screen != PLAYING || g->turnQueued) return;
    Direction wanted = g->direction;
    bool pressed = true;
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) wanted = UP;
    else if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) wanted = DOWN;
    else if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) wanted = LEFT;
    else if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) wanted = RIGHT;
    else pressed = false;
    if (pressed && !opposite(g->direction, wanted) && wanted != g->direction) {
        g->nextDirection = wanted;
        g->turnQueued = true;
    }
}

static void step(Game *g) {
    g->direction = g->nextDirection;
    g->turnQueued = false;
    Vector2 head = g->body[0];
    switch (g->direction) {
        case UP: head.y--; break;
        case DOWN: head.y++; break;
        case LEFT: head.x--; break;
        case RIGHT: head.x++; break;
    }
    if (head.x < 0 || head.x >= COLS || head.y < 0 || head.y >= ROWS) {
        g->screen = LOST;
        return;
    }
    bool eating = same(head, g->food);
    // On an ordinary move the last tail cell becomes free.
    int occupied = g->length - (eating ? 0 : 1);
    if (onSnake(g, head, occupied)) {
        g->screen = LOST;
        return;
    }
    int newLength = g->length + (eating ? 1 : 0);
    for (int i = newLength - 1; i > 0; i--) g->body[i] = g->body[i - 1];
    g->body[0] = head;
    g->length = newLength;
    if (eating) {
        g->score += 10;
        g->level++;
        if (g->level > MAX_LEVEL || g->length == MAX_CELLS) {
            g->screen = WON;
        } else {
            spawnFood(g);
        }
    }
}

static void centered(const char *text, int y, int size, Color color) {
    DrawText(text, (WIDTH - MeasureText(text, size)) / 2, y, size, color);
}

static void draw(const Game *g) {
    Color background = {13, 19, 24, 255};
    Color board = {20, 31, 37, 255};
    Color grid = {30, 43, 49, 255};
    Color green = {160, 234, 103, 255};
    Color red = {255, 117, 101, 255};
    Color white = {238, 244, 239, 255};
    Color muted = {139, 161, 164, 255};
    ClearBackground(background);
    DrawText("SNAKE", BOARD_X, 25, 38, white);
    DrawText(TextFormat("SCORE  %03d", g->score), BOARD_X, 78, 20, muted);
    DrawText(TextFormat("LEVEL  %02d / %02d", g->level > MAX_LEVEL ? MAX_LEVEL : g->level, MAX_LEVEL),
             WIDTH - 225, 78, 20, muted);
    DrawRectangle(BOARD_X - 2, BOARD_Y - 2, COLS * CELL + 4, ROWS * CELL + 4, green);
    DrawRectangle(BOARD_X, BOARD_Y, COLS * CELL, ROWS * CELL, board);
    for (int x = 1; x < COLS; x++)
        DrawLine(BOARD_X + x * CELL, BOARD_Y, BOARD_X + x * CELL, BOARD_Y + ROWS * CELL, grid);
    for (int y = 1; y < ROWS; y++)
        DrawLine(BOARD_X, BOARD_Y + y * CELL, BOARD_X + COLS * CELL, BOARD_Y + y * CELL, grid);
    if (g->screen != WON || g->length < MAX_CELLS) {
        DrawCircle(BOARD_X + (int)g->food.x * CELL + CELL / 2,
                   BOARD_Y + (int)g->food.y * CELL + CELL / 2, 8, red);
    }
    for (int i = g->length - 1; i >= 0; i--) {
        Rectangle r = {BOARD_X + g->body[i].x * CELL + 2,
                       BOARD_Y + g->body[i].y * CELL + 2, CELL - 4, CELL - 4};
        DrawRectangleRounded(r, 0.3f, 4, i == 0 ? white : green);
    }
    centered("MOVE  WASD / ARROWS       PAUSE  P / SPACE", HEIGHT - 47, 18, muted);
    if (g->screen != PLAYING) {
        DrawRectangle(BOARD_X, BOARD_Y, COLS * CELL, ROWS * CELL, (Color){7, 14, 18, 225});
        const char *heading = g->screen == MENU ? "READY TO PLAY?" :
                              g->screen == PAUSED ? "PAUSED" :
                              g->screen == WON ? "YOU WIN!" : "GAME OVER";
        const char *detail = g->screen == PAUSED ? "P / SPACE TO RESUME" :
                             "ENTER / SPACE TO START";
        centered(heading, BOARD_Y + ROWS * CELL / 2 - 36, 36, white);
        centered(detail, BOARD_Y + ROWS * CELL / 2 + 18, 20, green);
    }
}

int main(void) {
    InitWindow(WIDTH, HEIGHT, "Snake | Raylib Desktop");
    SetExitKey(KEY_ESCAPE);
    SetTargetFPS(60);
    Game game;
    reset(&game);
    while (!WindowShouldClose()) {
        input(&game);
        if (game.screen == PLAYING) {
            // Fixed-step movement is independent of rendering frame rate.
            game.accumulator += GetFrameTime();
            if (game.accumulator > 0.5f) game.accumulator = 0.5f;
            float interval = 0.18f - ((game.level - 1) / 3) * 0.025f;
            if (game.accumulator >= interval) {
                game.accumulator -= interval;
                step(&game);
            }
        }
        BeginDrawing();
        draw(&game);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
