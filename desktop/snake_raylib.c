#include "raylib.h"
#include <math.h>
#include <stdbool.h>

#define COLS 30
#define ROWS 20
#define CELL 24
#define BOARD_X 32
#define BOARD_Y 108
#define WIDTH (BOARD_X * 2 + COLS * CELL)
#define HEIGHT (BOARD_Y + ROWS * CELL + 76)
#define MAX_CELLS (COLS * ROWS)
#define BASE_INTERVAL_MS 150  // time per move at level 1
#define SPEED_STEP_MS 10      // interval reduction per speed tier
#define MIN_INTERVAL_MS 80    // fastest speed: 12.5 moves/s
#define LEVELS_PER_TIER 3
#define MAX_SPEED_TIER ((BASE_INTERVAL_MS - MIN_INTERVAL_MS) / SPEED_STEP_MS)  // 7, from level 22
#define SAMPLE_RATE 44100
#define SHIFT_SECONDS 0.35f  // color shift after eating
#define PULSE_SECONDS 0.40f  // ring pulse and fruit pop-in after eating

typedef enum { UP, DOWN, LEFT, RIGHT } Direction;
typedef enum { MENU, PLAYING, PAUSED, LOST, CLEARED } Screen;  // CLEARED: snake fills the board
enum { SFX_EAT = 1, SFX_LEVEL_UP = 2, SFX_GAME_OVER = 4 };

typedef struct {
    Vector2 body[MAX_CELLS];  // body[0] is the head; coordinates are grid cells.
    Vector2 food;
    int length, level, score;
    Direction direction, nextDirection;
    bool turnQueued;
    float accumulator;
    Screen screen;
} Game;

typedef struct { float from, to, seconds; } Tone;  // pitch glides from -> to (Hz)

typedef struct {
    Sound eat, levelUp, gameOver, start;
    bool ready, muted;
} Sfx;

typedef struct {
    Color background, board, grid, border, body, head, fruit, text, accent, title;
} Palette;

// Boards stay dark and fruits bright, so the fruit stands out in every palette
// and mid-transition. Each fruit uses a hue well away from its snake.
static const Palette PALETTES[] = {
    {   // Classic: the original look, used when effects are off
        {13, 19, 24, 255}, {20, 31, 37, 255}, {30, 43, 49, 255}, {160, 234, 103, 255},
        {160, 234, 103, 255}, {238, 244, 239, 255}, {255, 117, 101, 255},
        {139, 161, 164, 255}, {160, 234, 103, 255}, {238, 244, 239, 255}},
    {   // Ocean: cyan snake, amber fruit
        {8, 18, 32, 255}, {12, 28, 48, 255}, {22, 42, 66, 255}, {72, 202, 228, 255},
        {72, 202, 228, 255}, {224, 247, 255, 255}, {255, 196, 61, 255},
        {120, 156, 184, 255}, {72, 202, 228, 255}, {224, 247, 255, 255}},
    {   // Ember: orange snake, sky-blue fruit
        {28, 12, 18, 255}, {42, 18, 28, 255}, {62, 30, 42, 255}, {255, 140, 90, 255},
        {255, 140, 90, 255}, {255, 236, 214, 255}, {110, 214, 255, 255},
        {190, 140, 150, 255}, {255, 140, 90, 255}, {255, 236, 214, 255}},
    {   // Grape: violet snake, lime fruit
        {18, 12, 32, 255}, {28, 20, 48, 255}, {44, 34, 70, 255}, {180, 140, 255, 255},
        {180, 140, 255, 255}, {245, 238, 255, 255}, {170, 255, 110, 255},
        {160, 146, 196, 255}, {180, 140, 255, 255}, {245, 238, 255, 255}},
    {   // Mint: mint snake, pink fruit
        {10, 22, 18, 255}, {16, 34, 28, 255}, {28, 52, 43, 255}, {100, 230, 180, 255},
        {100, 230, 180, 255}, {230, 255, 245, 255}, {255, 110, 170, 255},
        {130, 170, 156, 255}, {100, 230, 180, 255}, {230, 255, 245, 255}},
    {   // Desert: gold snake, blue fruit
        {24, 20, 10, 255}, {38, 32, 16, 255}, {58, 50, 28, 255}, {240, 200, 80, 255},
        {240, 200, 80, 255}, {255, 248, 225, 255}, {110, 165, 255, 255},
        {180, 166, 128, 255}, {240, 200, 80, 255}, {255, 248, 225, 255}},
    {   // Neon: magenta snake, aqua fruit
        {20, 10, 26, 255}, {32, 14, 40, 255}, {52, 26, 62, 255}, {255, 90, 200, 255},
        {255, 90, 200, 255}, {255, 235, 250, 255}, {100, 255, 210, 255},
        {180, 140, 180, 255}, {255, 90, 200, 255}, {255, 235, 250, 255}},
    {   // Frost: ice snake, red fruit
        {14, 16, 22, 255}, {22, 26, 34, 255}, {38, 44, 56, 255}, {200, 214, 232, 255},
        {200, 214, 232, 255}, {255, 255, 255, 255}, {255, 84, 84, 255},
        {140, 150, 170, 255}, {200, 214, 232, 255}, {255, 255, 255, 255}},
};
#define PALETTE_COUNT ((int)(sizeof PALETTES / sizeof PALETTES[0]))

// Presentation only: nothing here feeds back into the game rules.
typedef struct {
    int current;       // palette being shown, or shifted towards
    Palette from;      // colors on screen when the current shift began
    float shift;       // shift progress, 0..1 (1 = settled)
    float pulse;       // eat pulse progress, 0..1 (1 = idle)
    Vector2 pulseCell; // grid cell where the food was eaten
    bool enabled;
} Theme;

static bool same(Vector2 a, Vector2 b) { return a.x == b.x && a.y == b.y; }

static bool onSnake(const Game *g, Vector2 p, int count) {
    for (int i = 0; i < count; i++) if (same(g->body[i], p)) return true;
    return false;
}

static void spawnFood(Game *g) {
    // Select uniformly among free cells; never place food on the snake.
    int freeCells = MAX_CELLS - g->length;
    if (freeCells <= 0) { g->screen = CLEARED; return; }
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

static void startGame(Game *g) {
    reset(g);
    g->screen = PLAYING;
}

static bool opposite(Direction a, Direction b) {
    return (a == UP && b == DOWN) || (a == DOWN && b == UP) ||
           (a == LEFT && b == RIGHT) || (a == RIGHT && b == LEFT);
}

static void input(Game *g) {
    if (g->screen == MENU || g->screen == LOST || g->screen == CLEARED) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) startGame(g);
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
        // Endless play: the run only stops early if no free cell is left for food.
        if (g->length == MAX_CELLS) g->screen = CLEARED;
        else spawnFood(g);
    }
}

// Speed increases every three levels until the cap is reached.
static int speedTier(int level) {
    int tier = (level - 1) / LEVELS_PER_TIER;
    return tier < MAX_SPEED_TIER ? tier : MAX_SPEED_TIER;
}

// Seconds per move; whole milliseconds keep the curve exact.
static float moveInterval(int level) {
    return (float)(BASE_INTERVAL_MS - speedTier(level) * SPEED_STEP_MS) / 1000.0f;
}

// Renders a sequence of soft triangle-wave tones into a Sound. Each tone fades
// in briefly and decays to silence (no clicks); `delay` adds leading silence.
static Sound makeSound(const Tone *tones, int count, float delay, float volume) {
    float seconds = delay;
    for (int i = 0; i < count; i++) seconds += tones[i].seconds;
    int frames = (int)(seconds * SAMPLE_RATE);
    short *samples = MemAlloc((unsigned int)frames * (unsigned int)sizeof(short));  // zero-filled
    if (!samples) return (Sound){0};
    int at = (int)(delay * SAMPLE_RATE);
    float phase = 0.0f;
    for (int i = 0; i < count; i++) {
        int n = (int)(tones[i].seconds * SAMPLE_RATE);
        for (int j = 0; j < n && at < frames; j++, at++) {
            float t = (float)j / n;
            phase += (tones[i].from + (tones[i].to - tones[i].from) * t) / SAMPLE_RATE;
            phase -= (int)phase;
            float triangle = 4.0f * fabsf(phase - 0.5f) - 1.0f;
            float envelope = fminf(1.0f, j / (0.004f * SAMPLE_RATE)) * (1.0f - t) * (1.0f - t);
            samples[at] = (short)(triangle * envelope * volume * 32767.0f);
        }
    }
    Wave wave = {(unsigned int)frames, SAMPLE_RATE, 16, 1, samples};
    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);  // the Sound keeps its own copy of the samples
    return sound;
}

static void loadSfx(Sfx *sfx) {
    *sfx = (Sfx){0};
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;  // no audio device: play silently
    const Tone eat[] = {{520.0f, 1040.0f, 0.09f}};
    const Tone levelUp[] = {{523.25f, 523.25f, 0.07f}, {659.25f, 659.25f, 0.07f},
                            {783.99f, 783.99f, 0.07f}, {1046.50f, 1046.50f, 0.18f}};
    const Tone gameOver[] = {{392.00f, 392.00f, 0.13f}, {311.13f, 311.13f, 0.13f},
                             {261.63f, 261.63f, 0.13f}, {196.00f, 147.00f, 0.40f}};
    const Tone start[] = {{659.25f, 659.25f, 0.05f}, {987.77f, 987.77f, 0.08f}};
    sfx->eat = makeSound(eat, 1, 0.0f, 0.30f);
    sfx->levelUp = makeSound(levelUp, 4, 0.08f, 0.24f);  // starts after the eat blip
    sfx->gameOver = makeSound(gameOver, 4, 0.0f, 0.30f);
    sfx->start = makeSound(start, 2, 0.0f, 0.16f);
    sfx->ready = true;
}

static void unloadSfx(Sfx *sfx) {
    if (!sfx->ready) return;
    UnloadSound(sfx->eat);
    UnloadSound(sfx->levelUp);
    UnloadSound(sfx->gameOver);
    UnloadSound(sfx->start);
    CloseAudioDevice();
}

static void play(const Sfx *sfx, Sound sound) {
    if (sfx->ready && !sfx->muted) PlaySound(sound);
}

// The SFX_* sounds one movement step should trigger. Evaluated once per step,
// never per frame, so each event sounds exactly once.
static int stepSounds(const Game *g, int previousLevel) {
    if (g->screen == LOST) return SFX_GAME_OVER;
    if (g->level == previousLevel) return 0;
    // Every food is a level, so the fanfare marks real speed-ups (none past the
    // cap) and a completed board.
    if (g->screen == CLEARED || speedTier(g->level) > speedTier(previousLevel))
        return SFX_EAT | SFX_LEVEL_UP;
    return SFX_EAT;
}

static void playSounds(const Sfx *sfx, int sounds) {
    if (sounds & SFX_EAT) play(sfx, sfx->eat);
    if (sounds & SFX_LEVEL_UP) play(sfx, sfx->levelUp);
    if (sounds & SFX_GAME_OVER) play(sfx, sfx->gameOver);
}

// A random palette other than `current`, so a palette never repeats back to back.
static int nextPalette(int current) {
    return (current + GetRandomValue(1, PALETTE_COUNT - 1)) % PALETTE_COUNT;
}

static float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

static Palette blendPalette(const Palette *a, const Palette *b, float t) {
    return (Palette){
        ColorLerp(a->background, b->background, t), ColorLerp(a->board, b->board, t),
        ColorLerp(a->grid, b->grid, t), ColorLerp(a->border, b->border, t),
        ColorLerp(a->body, b->body, t), ColorLerp(a->head, b->head, t),
        ColorLerp(a->fruit, b->fruit, t), ColorLerp(a->text, b->text, t),
        ColorLerp(a->accent, b->accent, t), ColorLerp(a->title, b->title, t)};
}

static void themeReset(Theme *theme, bool enabled) {
    *theme = (Theme){.from = PALETTES[0], .shift = 1.0f, .pulse = 1.0f, .enabled = enabled};
}

// The colors to draw this frame.
static Palette themeColors(const Theme *theme) {
    Palette shown = blendPalette(&theme->from, &PALETTES[theme->current], smooth(theme->shift));
    shown.fruit = PALETTES[theme->current].fruit;  // new fruit appears in the new palette
    return shown;
}

// Starts a shift from whatever is on screen now, so eating again mid-shift
// carries on smoothly instead of jumping.
static void themeEat(Theme *theme, Vector2 cell) {
    if (!theme->enabled) return;
    theme->from = themeColors(theme);
    theme->current = nextPalette(theme->current);
    theme->shift = 0.0f;
    theme->pulse = 0.0f;
    theme->pulseCell = cell;
}

static void themeUpdate(Theme *theme, float dt) {
    theme->shift = fminf(1.0f, theme->shift + dt / SHIFT_SECONDS);
    theme->pulse = fminf(1.0f, theme->pulse + dt / PULSE_SECONDS);
}

static void centered(const char *text, int y, int size, Color color) {
    DrawText(text, (WIDTH - MeasureText(text, size)) / 2, y, size, color);
}

static Vector2 cellCenter(Vector2 cell) {
    return (Vector2){BOARD_X + cell.x * CELL + CELL / 2.0f, BOARD_Y + cell.y * CELL + CELL / 2.0f};
}

// Snake shape, in pixels. Body plus outline is 21 px wide, so neighbouring
// runs of a tight coil keep a 3 px gap and their dark outlines between them.
#define BODY_RADIUS 9.0f
#define OUTLINE 1.5f
#define HEAD_RADIUS 10.5f    // back of the head, wider than the body
#define SNOUT_RADIUS 8.0f    // front of the head, set forward to form a bullet shape
#define HEAD_BACK 2.0f       // offsets of those two circles from the cell centre
#define SNOUT_FORWARD 4.0f
#define TAPER_SEGMENTS 3
#define TONGUE_PERIOD 2.6f   // seconds between tongue flicks
#define TONGUE_SECONDS 0.16f

static Vector2 heading(Direction d) {
    return d == UP ? (Vector2){0, -1} : d == DOWN ? (Vector2){0, 1} :
           d == LEFT ? (Vector2){-1, 0} : (Vector2){1, 0};
}

// Body half-width of segment i: full along the body, narrowing over the last
// few segments into a tail (fewer on a short snake, so it never looks like a cone).
static float segmentRadius(int i, int length) {
    int taper = (length - 1) / 2 < TAPER_SEGMENTS ? (length - 1) / 2 : TAPER_SEGMENTS;
    int fromTail = length - 1 - i;
    if (taper < 1 || fromTail >= taper) return BODY_RADIUS;
    return BODY_RADIUS * (0.4f + 0.6f * (float)fromTail / taper);
}

static Vector2 offset(Vector2 p, Vector2 dir, float distance) {
    return (Vector2){p.x + dir.x * distance, p.y + dir.y * distance};
}

static void eyePositions(Vector2 head, Direction d, Vector2 eyes[2]) {
    Vector2 f = heading(d), side = {-f.y, f.x};
    eyes[0] = offset(offset(head, f, 2.5f), side, 5.0f);
    eyes[1] = offset(offset(head, f, 2.5f), side, -5.0f);
}

// Filled quad with corners a-b-c-d in order, whichever way they wind.
static void drawQuad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color color) {
    float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (cross < 0) { DrawTriangle(a, b, c, color); DrawTriangle(a, c, d, color); }
    else { DrawTriangle(a, c, b, color); DrawTriangle(a, d, c, color); }
}

// Segment i needs a round joint where the body turns, and at the tail tip.
// Elsewhere the straight bridges already meet edge to edge.
static bool needsJoint(const Game *g, int i) {
    if (i == g->length - 1) return true;
    Vector2 behind = g->body[i + 1], here = g->body[i], ahead = g->body[i - 1];
    return behind.x - here.x != here.x - ahead.x || behind.y - here.y != here.y - ahead.y;
}

// The body as one shape, `grow` px fatter than normal: tapering bridges
// between grid cells, rounded at corners and at the tail.
static void drawBody(const Game *g, float grow, Color color) {
    for (int i = g->length - 1; i > 0; i--) {
        Vector2 a = cellCenter(g->body[i]), b = cellCenter(g->body[i - 1]);
        float ra = segmentRadius(i, g->length) + grow;
        float rb = (i > 1 ? segmentRadius(i - 1, g->length) : BODY_RADIUS) + grow;
        Vector2 n = {(a.y - b.y) / CELL, (b.x - a.x) / CELL};  // unit normal (cells are adjacent)
        drawQuad(offset(a, n, ra), offset(b, n, rb), offset(b, n, -rb), offset(a, n, -ra), color);
        if (needsJoint(g, i)) DrawCircleV(a, ra, color);
    }
}

static void drawHeadShape(Vector2 head, Vector2 f, float grow, Color color) {
    Vector2 back = offset(head, f, -HEAD_BACK), snout = offset(head, f, SNOUT_FORWARD);
    Vector2 side = {-f.y, f.x};
    DrawCircleV(back, HEAD_RADIUS + grow, color);
    DrawCircleV(snout, SNOUT_RADIUS + grow, color);
    // Straight cheeks between the two circles make a clean bullet outline.
    drawQuad(offset(back, side, HEAD_RADIUS + grow), offset(snout, side, SNOUT_RADIUS + grow),
             offset(snout, side, -SNOUT_RADIUS - grow), offset(back, side, -HEAD_RADIUS - grow), color);
}

// Bold arcade style: dark outlines, a light stripe along the back, and a
// bullet-shaped head with narrow, forward-looking eyes. All colors derive from
// the palette, so the snake follows every color shift.
static void drawSnake(const Game *g, const Palette *p, bool tongue) {
    Color outline = ColorLerp(p->body, p->background, 0.8f);
    Color stripe = ColorLerp(p->body, p->head, 0.35f);
    Color headColor = ColorLerp(p->body, p->head, 0.45f);
    Vector2 head = cellCenter(g->body[0]), f = heading(g->direction), side = {-f.y, f.x};

    drawBody(g, OUTLINE, outline);
    drawBody(g, 0.0f, p->body);
    for (int i = g->length - 1; i > 1; i--) {
        Vector2 a = cellCenter(g->body[i]), b = cellCenter(g->body[i - 1]);
        float w = segmentRadius(i, g->length) * 0.3f;
        DrawLineEx(a, b, 2.0f * w, stripe);
        if (needsJoint(g, i)) DrawCircleV(a, w, stripe);  // rounds the stripe through corners
    }

    if (tongue) {  // a quick forked flick, only when there is board ahead
        Vector2 ahead = {g->body[0].x + f.x, g->body[0].y + f.y};
        if (ahead.x >= 0 && ahead.x < COLS && ahead.y >= 0 && ahead.y < ROWS) {
            Color tongueColor = {236, 84, 112, 255};
            Vector2 root = offset(head, f, 11.0f), fork = offset(head, f, 16.0f);
            DrawLineEx(root, fork, 2.0f, tongueColor);
            DrawLineEx(fork, offset(offset(fork, f, 3.0f), side, 2.2f), 1.8f, tongueColor);
            DrawLineEx(fork, offset(offset(fork, f, 3.0f), side, -2.2f), 1.8f, tongueColor);
        }
    }
    drawHeadShape(head, f, OUTLINE, outline);
    drawHeadShape(head, f, 0.0f, headColor);

    Vector2 eyes[2];
    eyePositions(head, g->direction, eyes);
    bool horizontal = f.x != 0;
    for (int e = 0; e < 2; e++) {
        // Bright eye ringed in the outline color, with a reptile slit pupil
        // set forward so the snake looks where it is going.
        DrawCircleV(eyes[e], 3.9f, outline);
        DrawCircleV(eyes[e], 3.1f, p->head);
        Vector2 pupil = offset(eyes[e], f, 1.0f);
        DrawEllipse((int)roundf(pupil.x), (int)roundf(pupil.y), horizontal ? 2.3f : 1.1f,
                    horizontal ? 1.1f : 2.3f, p->background);
    }
}

static void draw(const Game *g, const Theme *theme, bool soundOff, double time) {
    Palette p = themeColors(theme);
    ClearBackground(p.background);
    DrawText("SNAKE", BOARD_X, 25, 38, p.title);
    const char *status = TextFormat("%s%s%s", soundOff ? "SOUND OFF" : "",
                                    soundOff && !theme->enabled ? "   " : "",
                                    theme->enabled ? "" : "EFFECTS OFF");
    DrawText(status, WIDTH - BOARD_X - MeasureText(status, 20), 36, 20, p.text);
    DrawText(TextFormat("SCORE  %03d", g->score), BOARD_X, 78, 20, p.text);
    const char *level = TextFormat("LEVEL  %02d%s", g->level,
                                   speedTier(g->level) == MAX_SPEED_TIER ? "  MAX SPEED" : "");
    DrawText(level, WIDTH - BOARD_X - MeasureText(level, 20), 78, 20, p.text);
    DrawRectangle(BOARD_X - 2, BOARD_Y - 2, COLS * CELL + 4, ROWS * CELL + 4, p.border);
    DrawRectangle(BOARD_X, BOARD_Y, COLS * CELL, ROWS * CELL, p.board);
    for (int x = 1; x < COLS; x++)
        DrawLine(BOARD_X + x * CELL, BOARD_Y, BOARD_X + x * CELL, BOARD_Y + ROWS * CELL, p.grid);
    for (int y = 1; y < ROWS; y++)
        DrawLine(BOARD_X, BOARD_Y + y * CELL, BOARD_X + COLS * CELL, BOARD_Y + y * CELL, p.grid);
    if (theme->pulse < 1.0f) {  // a ring spreading out from where the food was eaten
        float radius = 10.0f + 34.0f * smooth(theme->pulse);
        DrawRing(cellCenter(theme->pulseCell), radius - 3.0f, radius, 0.0f, 360.0f, 48,
                 ColorAlpha(p.accent, 0.55f * (1.0f - theme->pulse)));
    }
    if (g->screen != CLEARED) {  // the new fruit pops in at the start of the pulse
        float pop = fminf(1.0f, theme->pulse * 3.0f);
        DrawCircleV(cellCenter(g->food), 8.0f * (0.4f + 0.6f * smooth(pop)), p.fruit);
    }
    bool tongue = theme->enabled && g->screen == PLAYING &&
                  fmod(time, TONGUE_PERIOD) < TONGUE_SECONDS;
    drawSnake(g, &p, tongue);
    centered("MOVE  WASD / ARROWS     PAUSE  P / SPACE     MUTE  M     EFFECTS  V",
             HEIGHT - 47, 18, p.text);
    if (g->screen != PLAYING) {
        DrawRectangle(BOARD_X, BOARD_Y, COLS * CELL, ROWS * CELL,
                      ColorAlpha(ColorBrightness(p.background, -0.45f), 0.88f));
        const char *heading = g->screen == MENU ? "READY TO PLAY?" :
                              g->screen == PAUSED ? "PAUSED" :
                              g->screen == CLEARED ? "BOARD COMPLETE!" : "GAME OVER";
        const char *detail = g->screen == PAUSED ? "P / SPACE TO RESUME" :
                             "ENTER / SPACE TO START";
        centered(heading, BOARD_Y + ROWS * CELL / 2 - 36, 36, p.title);
        centered(detail, BOARD_Y + ROWS * CELL / 2 + 18, 20, p.accent);
    }
}

int main(void) {
    InitWindow(WIDTH, HEIGHT, "Snake | Raylib Desktop");
    SetExitKey(KEY_ESCAPE);
    SetTargetFPS(60);
    Sfx sfx;
    loadSfx(&sfx);
    Theme theme;
    themeReset(&theme, true);
    Game game;
    reset(&game);
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_M)) sfx.muted = !sfx.muted;
        if (IsKeyPressed(KEY_V)) themeReset(&theme, !theme.enabled);  // off: classic colors
        Screen before = game.screen;
        input(&game);
        if (game.screen == PLAYING && before != PLAYING && before != PAUSED)
            play(&sfx, sfx.start);
        if (game.screen == PLAYING) {
            // Fixed-step movement is independent of rendering frame rate.
            game.accumulator += GetFrameTime();
            if (game.accumulator > 0.5f) game.accumulator = 0.5f;
            float interval = moveInterval(game.level);
            if (game.accumulator >= interval) {
                game.accumulator -= interval;
                int level = game.level;
                step(&game);
                playSounds(&sfx, stepSounds(&game, level));
                if (game.level > level) themeEat(&theme, game.body[0]);
            }
        }
        themeUpdate(&theme, GetFrameTime());
        BeginDrawing();
        draw(&game, &theme, sfx.muted, GetTime());
        EndDrawing();
    }
    unloadSfx(&sfx);
    CloseWindow();
    return 0;
}
