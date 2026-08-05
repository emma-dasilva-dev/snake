
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <ncurses.h>

#define TBool            int
#define True             1
#define False            0
#define SHAPE_FOOD       '@'  // Food
#define SHAPE_SNAKE      '#'  // Snake body
#define GAMEWIN_YLEN     15
#define GAMEWIN_XLEN     60
#define LOGWIN_YLEN      7
#define LOGWIN_XLEN      (GAMEWIN_XLEN)
#define LOGBUF_NUM       (LOGWIN_YLEN-2)
#define LOGBUF_LEN       (GAMEWIN_XLEN-2)
#define MAXLEVEL         12

#define GetSnakeTail(s)  ((s)->head->front)

WINDOW *logwin; // Declare a log window
#define INITRUNLOG()     logwin = newlogw() // Create a log window by calling the custom function newlogw()
#define RUNLOG(str)      runlog(logwin, str) // Run the log window to display game prompts
#define DESTROYRUNLOG()  delwin(logwin)

int g_level; // Player level, a global variable

enum TDirection {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
};

struct TFood {
    int y;
    int x;
};

struct TSnakeNode {
    int y;
    int x;
    struct TSnakeNode *front;
};

struct TSnake {
    int    length;
    struct TSnakeNode *head;
    enum   TDirection  dir;
};

int refreshgamew(WINDOW *win, struct TSnake *psnake);
void movesnake(struct TSnake *psnake);
int checksnake(struct TFood *pfood, struct TSnake *psnake);
void snakegrowup(struct TFood *pfood, struct TSnake *psnake);
void gameover(WINDOW *win, char *str);
struct TSnakeNode *newsnakenode(struct TSnakeNode **ppsnode, int y, int x);
WINDOW* newgamew();
struct TSnake* initsnake();
void destroysnake(struct TSnake *psnake);
void drawsnakew(WINDOW *win, struct TSnake *psnake);
void drawfoodw(WINDOW *win, struct TFood *pfood, struct TSnake *psnake);
TBool checkfood(struct TFood *pfood, struct TSnake *psnake);
WINDOW* newlogw();
void runlog(WINDOW *win, char *str);
void cleanline(WINDOW *win, int y, int x);
