// Tests for the Raylib desktop edition's game rules and sound triggers.
// The game source is compiled in directly so its static functions are visible.
#define main snake_main
#include "snake_raylib.c"
#undef main
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks, failures;

#define CHECK(c) do { checks++; if (!(c)) { failures++; \
    printf("FAIL  %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

static bool inBounds(Vector2 p) { return p.x >= 0 && p.x < COLS && p.y >= 0 && p.y < ROWS; }

static bool foodValid(const Game *g) { return inBounds(g->food) && !onSnake(g, g->food, g->length); }

static Vector2 ahead(const Game *g) {
    Vector2 p = g->body[0];
    switch (g->nextDirection) {
        case UP: p.y--; break;
        case DOWN: p.y++; break;
        case LEFT: p.x--; break;
        case RIGHT: p.x++; break;
    }
    return p;
}

// Runs one movement step the way the main loop does and returns its sounds.
static int move(Game *g) {
    int level = g->level;
    step(g);
    return stepSounds(g, level);
}

static void testSpeedCap(void) {
    // 150 ms at the start, 10 ms faster every three levels, 80 ms from level 22.
    const int expectedMs[] = {150, 140, 130, 120, 110, 100, 90, 80};
    for (int level = 1; level <= 30; level++) {
        int tier = (level - 1) / 3 < 7 ? (level - 1) / 3 : 7;
        CHECK(fabsf(moveInterval(level) - expectedMs[tier] / 1000.0f) < 1e-6f);
    }
    const float minimum = 0.08f;
    CHECK(MAX_SPEED_TIER == 7);
    CHECK(speedTier(22) == MAX_SPEED_TIER && speedTier(21) == MAX_SPEED_TIER - 1);
    CHECK(moveInterval(21) > minimum + 0.005f);  // cap not reached early
    for (int level = 2; level <= MAX_CELLS + 10; level++) {
        float change = moveInterval(level - 1) - moveInterval(level);
        CHECK(change >= 0.0f && change < 0.0101f);  // never slower, no big jumps
        CHECK(moveInterval(level) >= minimum);
    }
    CHECK(moveInterval(22) == minimum);
    CHECK(moveInterval(MAX_CELLS) == minimum);
    CHECK(moveInterval(1000000) == minimum);
}

// Eats 60 food items in a row, steering in a serpentine so the snake never
// meets itself, and checks scoring, growth, spawning and sounds on every bite.
static void testEndlessProgression(void) {
    Game g;
    startGame(&g);
    CHECK(g.screen == PLAYING && foodValid(&g));
    int levelUps = 0;
    for (int eaten = 1; eaten <= 60; eaten++) {
        Vector2 next = ahead(&g);
        if (!inBounds(next)) {  // reached a side wall: drop a row and turn
            g.nextDirection = DOWN;
            next = ahead(&g);
        } else if (g.direction == DOWN) {
            g.nextDirection = g.body[0].x == 0 ? RIGHT : LEFT;
            next = ahead(&g);
        }
        g.food = next;
        int sounds = move(&g);
        CHECK(g.screen == PLAYING);
        CHECK(g.score == eaten * 10);
        CHECK(g.level == eaten + 1);
        CHECK(g.length == 4 + eaten);
        CHECK(foodValid(&g));  // a fresh food item was spawned on a free cell
        CHECK(sounds & SFX_EAT);
        CHECK(!(sounds & SFX_GAME_OVER));
        if (sounds & SFX_LEVEL_UP) {
            levelUps++;
            CHECK((g.level - 1) % LEVELS_PER_TIER == 0 && g.level <= 22);  // levels 4, 7, ... 22
        }
    }
    CHECK(g.level == 61 && g.score == 600);
    CHECK(levelUps == MAX_SPEED_TIER);  // no fanfare once the cap is reached
}

static void testPlainMove(void) {
    Game g;
    startGame(&g);
    g.food = (Vector2){0, 0};
    Vector2 head = g.body[0];
    CHECK(move(&g) == 0);
    CHECK(g.score == 0 && g.length == 4 && g.body[0].x == head.x - 1);
}

static void testCollisions(void) {
    Game g;
    startGame(&g);
    g.food = (Vector2){0, 0};
    for (int i = 0; i < COLS && g.screen == PLAYING; i++) move(&g);
    CHECK(g.screen == LOST);
    CHECK(g.body[0].x == 0);  // died moving off the left edge

    startGame(&g);
    g.direction = g.nextDirection = UP;
    g.body[0] = (Vector2){5, 0};
    CHECK(move(&g) == SFX_GAME_OVER);
    CHECK(g.screen == LOST);

    // Snake curled into a square: moving down hits its own body.
    startGame(&g);
    g.length = 5;
    g.body[0] = (Vector2){5, 5}; g.body[1] = (Vector2){6, 5}; g.body[2] = (Vector2){6, 6};
    g.body[3] = (Vector2){5, 6}; g.body[4] = (Vector2){4, 6};
    g.food = (Vector2){0, 0};
    g.direction = LEFT; g.nextDirection = DOWN;
    CHECK(move(&g) == SFX_GAME_OVER);
    CHECK(g.screen == LOST && g.score == 0);

    // Moving into the cell the tail is leaving is legal.
    startGame(&g);
    g.length = 4;
    g.body[0] = (Vector2){5, 5}; g.body[1] = (Vector2){6, 5}; g.body[2] = (Vector2){6, 6};
    g.body[3] = (Vector2){5, 6};
    g.food = (Vector2){0, 0};
    g.direction = LEFT; g.nextDirection = DOWN;
    CHECK(move(&g) == 0 && g.screen == PLAYING);
}

static void testRestartAfterDeath(void) {
    Game g;
    startGame(&g);
    g.score = 120; g.level = 13; g.length = 16; g.screen = LOST;
    startGame(&g);
    CHECK(g.screen == PLAYING && g.score == 0 && g.level == 1 && g.length == 4);
    CHECK(g.direction == LEFT && foodValid(&g));
    g.food = (Vector2){0, 0};
    CHECK(move(&g) == 0 && g.screen == PLAYING);
}

// The snake occupies every cell but one, in a serpentine, with food on the last
// free cell directly ahead. Eating it must end the run as a completed board.
static void testFullBoard(void) {
    static Vector2 path[MAX_CELLS];
    for (int y = 0, i = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++, i++)
            path[i] = (Vector2){(float)(y % 2 ? COLS - 1 - x : x), (float)y};
    Game g;
    startGame(&g);
    g.length = MAX_CELLS - 1;
    for (int i = 0; i < g.length; i++) g.body[i] = path[MAX_CELLS - 2 - i];
    g.food = path[MAX_CELLS - 1];
    g.direction = g.nextDirection = LEFT;  // the last row runs right to left
    g.level = MAX_CELLS - 4;
    g.score = (g.level - 1) * 10;
    int score = g.score;
    int sounds = move(&g);
    CHECK(g.screen == CLEARED);
    CHECK(g.length == MAX_CELLS && g.score == score + 10);
    CHECK(sounds == (SFX_EAT | SFX_LEVEL_UP));  // one-off fanfare for the result

    g.screen = PLAYING;  // spawning with no free cell must not loop or misplace food
    spawnFood(&g);
    CHECK(g.screen == CLEARED);

    startGame(&g);  // restart from the result screen
    CHECK(g.screen == PLAYING && g.length == 4 && foodValid(&g));
}

static bool sameColor(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }

static bool nearColor(Color a, Color b) {  // within rounding of an interpolation
    return abs(a.r - b.r) <= 1 && abs(a.g - b.g) <= 1 && abs(a.b - b.b) <= 1;
}

// Everything except the fruit, which always shows the target palette.
static bool samePaletteNoFruit(Palette a, Palette b) {
    a.fruit = b.fruit;
    return memcmp(&a, &b, sizeof a) == 0;
}

static float channel(unsigned char c) {
    float v = c / 255.0f;
    return v <= 0.03928f ? v / 12.92f : powf((v + 0.055f) / 1.055f, 2.4f);
}

// WCAG contrast ratio, 1 (none) to 21 (black on white).
static float contrast(Color a, Color b) {
    float la = 0.2126f * channel(a.r) + 0.7152f * channel(a.g) + 0.0722f * channel(a.b);
    float lb = 0.2126f * channel(b.r) + 0.7152f * channel(b.g) + 0.0722f * channel(b.b);
    return (fmaxf(la, lb) + 0.05f) / (fminf(la, lb) + 0.05f);
}

static float distance(Color a, Color b) {
    return sqrtf((float)((a.r - b.r) * (a.r - b.r) + (a.g - b.g) * (a.g - b.g) + (a.b - b.b) * (a.b - b.b)));
}

static void testPalettes(void) {
    CHECK(PALETTE_COUNT >= 6);
    const Palette *classic = &PALETTES[0];  // the original design is kept as the default
    CHECK(sameColor(classic->background, (Color){13, 19, 24, 255}));
    CHECK(sameColor(classic->board, (Color){20, 31, 37, 255}));
    CHECK(sameColor(classic->grid, (Color){30, 43, 49, 255}));
    CHECK(sameColor(classic->body, (Color){160, 234, 103, 255}));
    CHECK(sameColor(classic->head, (Color){238, 244, 239, 255}));
    CHECK(sameColor(classic->fruit, (Color){255, 117, 101, 255}));
    CHECK(sameColor(classic->text, (Color){139, 161, 164, 255}));
    for (int i = 0; i < PALETTE_COUNT; i++) {
        const Palette *p = &PALETTES[i];
        CHECK(contrast(p->fruit, p->board) >= 4.5f);
        CHECK(distance(p->fruit, p->body) >= 150.0f);  // fruit never blends into the snake
        CHECK(distance(p->fruit, p->head) >= 120.0f);
        CHECK(contrast(p->body, p->board) >= 4.5f);
        CHECK(contrast(p->text, p->background) >= 4.5f);
        CHECK(contrast(p->accent, p->background) >= 4.5f);
        CHECK(contrast(p->title, p->background) >= 7.0f);
        CHECK(contrast(p->grid, p->board) > 1.1f);  // grid lines stay visible but quiet
        for (int j = 0; j < PALETTE_COUNT; j++) {
            if (i == j) continue;
            CHECK(distance(p->body, PALETTES[j].body) >= 60.0f);  // each palette looks different
            // While shifting i -> j the new fruit sits on a half-blended board.
            for (int k = 0; k <= 10; k++) {
                Color board = ColorLerp(p->board, PALETTES[j].board, k / 10.0f);
                CHECK(contrast(PALETTES[j].fruit, board) >= 4.5f);
            }
        }
    }
}

static void testPaletteRotation(void) {
    for (int current = 0; current < PALETTE_COUNT; current++) {
        int seen[PALETTE_COUNT] = {0};
        for (int i = 0; i < 2000; i++) {
            int next = nextPalette(current);
            CHECK(next >= 0 && next < PALETTE_COUNT && next != current);
            if (next >= 0 && next < PALETTE_COUNT) seen[next]++;
        }
        for (int p = 0; p < PALETTE_COUNT; p++) CHECK(p == current ? seen[p] == 0 : seen[p] > 0);
    }
    Theme theme;
    themeReset(&theme, true);
    for (int i = 0; i < 500; i++) {  // a long run of meals never repeats a palette
        int before = theme.current;
        themeEat(&theme, (Vector2){1, 1});
        themeUpdate(&theme, 1.0f);
        CHECK(theme.current != before);
    }
}

static void testTransition(void) {
    CHECK(SHIFT_SECONDS >= 0.3f && SHIFT_SECONDS <= 0.4f);
    Theme theme;
    themeReset(&theme, true);
    Palette shown = themeColors(&theme);
    CHECK(memcmp(&shown, &PALETTES[0], sizeof shown) == 0);
    CHECK(theme.pulse == 1.0f);

    themeEat(&theme, (Vector2){7, 3});
    const Palette *target = &PALETTES[theme.current];
    CHECK(theme.current != 0 && theme.shift == 0.0f && theme.pulse == 0.0f);
    CHECK(theme.pulseCell.x == 7 && theme.pulseCell.y == 3);
    shown = themeColors(&theme);
    CHECK(samePaletteNoFruit(shown, PALETTES[0]));      // no flash: starts from the old colors
    CHECK(sameColor(shown.fruit, target->fruit));       // the new fruit is already in the new palette

    themeUpdate(&theme, SHIFT_SECONDS / 2);  // halfway through, colors are halfway
    shown = themeColors(&theme);
    CHECK(nearColor(shown.board, ColorLerp(PALETTES[0].board, target->board, 0.5f)));
    CHECK(nearColor(shown.body, ColorLerp(PALETTES[0].body, target->body, 0.5f)));
    CHECK(theme.pulse > 0.0f && theme.pulse < 1.0f);

    themeUpdate(&theme, SHIFT_SECONDS / 2 + 0.001f);
    CHECK(theme.shift == 1.0f);
    shown = themeColors(&theme);
    CHECK(memcmp(&shown, target, sizeof shown) == 0);  // settles exactly on the new palette
    themeUpdate(&theme, PULSE_SECONDS);
    CHECK(theme.pulse == 1.0f);

    // Eating again mid-shift continues from the colors on screen.
    themeEat(&theme, (Vector2){1, 1});
    themeUpdate(&theme, 0.1f);
    Palette before = themeColors(&theme);
    themeEat(&theme, (Vector2){2, 2});
    CHECK(samePaletteNoFruit(themeColors(&theme), before));

    themeUpdate(&theme, 5.0f);  // a long frame hitch just finishes the shift
    CHECK(theme.shift == 1.0f && theme.pulse == 1.0f);

    // Turning effects off restores the classic colors at once and stops shifting.
    themeEat(&theme, (Vector2){3, 3});
    themeReset(&theme, false);
    shown = themeColors(&theme);
    CHECK(memcmp(&shown, &PALETTES[0], sizeof shown) == 0);
    themeEat(&theme, (Vector2){4, 4});
    CHECK(theme.current == 0 && theme.shift == 1.0f && theme.pulse == 1.0f);
    themeReset(&theme, true);
    themeEat(&theme, (Vector2){4, 4});
    CHECK(theme.current != 0);
}

static void testSnakeShape(void) {
    // Tight coils stay readable: neighbouring runs never touch.
    CHECK(2.0f * (BODY_RADIUS + OUTLINE) <= CELL - 2.0f);
    // The head is wider than the body but stays within its own cell.
    CHECK(HEAD_RADIUS > BODY_RADIUS);
    CHECK(HEAD_RADIUS < CELL / 2.0f);
    CHECK(SNOUT_FORWARD + SNOUT_RADIUS <= CELL / 2.0f);
    CHECK(HEAD_BACK + HEAD_RADIUS + SNOUT_FORWARD + SNOUT_RADIUS > 2.0f * HEAD_RADIUS);  // longer than wide

    for (int length = 4; length <= MAX_CELLS; length++) {
        float previous = BODY_RADIUS;
        for (int i = 1; i < length; i++) {
            float r = segmentRadius(i, length);
            CHECK(r <= previous && r >= 3.0f);  // narrows towards the tail, never vanishes
            previous = r;
        }
        CHECK(segmentRadius(length - 1, length) < BODY_RADIUS * 0.5f);  // tail tip is tapered
        if (length >= 8) CHECK(segmentRadius(length - 1 - TAPER_SEGMENTS, length) == BODY_RADIUS);
    }
    // The starting snake keeps a full body and only tapers at its tail.
    CHECK(segmentRadius(1, 4) == BODY_RADIUS && segmentRadius(3, 4) < BODY_RADIUS);

    // Round joints are drawn only at turns and the tail tip.
    Game s;
    startGame(&s);  // straight starting snake
    CHECK(!needsJoint(&s, 1) && !needsJoint(&s, 2) && needsJoint(&s, 3));
    s.length = 5;   // an L: (5,5) (6,5) (6,6) (6,7) (6,8)
    s.body[0] = (Vector2){5, 5}; s.body[1] = (Vector2){6, 5}; s.body[2] = (Vector2){6, 6};
    s.body[3] = (Vector2){6, 7}; s.body[4] = (Vector2){6, 8};
    CHECK(needsJoint(&s, 1) && !needsJoint(&s, 2) && !needsJoint(&s, 3) && needsJoint(&s, 4));

    // The head and eyes face the way step() actually moves the snake.
    Direction dirs[] = {UP, DOWN, LEFT, RIGHT};
    for (int k = 0; k < 4; k++) {
        Game g;
        startGame(&g);
        g.body[0] = (Vector2){10, 10};
        g.food = (Vector2){0, 0};
        g.direction = g.nextDirection = dirs[k];
        step(&g);
        Vector2 f = heading(g.direction);
        CHECK(g.body[0].x == 10 + f.x && g.body[0].y == 10 + f.y);

        Vector2 head = cellCenter(g.body[0]), eyes[2];
        eyePositions(head, g.direction, eyes);
        for (int e = 0; e < 2; e++) {
            float ahead = (eyes[e].x - head.x) * f.x + (eyes[e].y - head.y) * f.y;
            CHECK(ahead > 0.0f);  // eyes sit towards the front of the head
            Vector2 back = offset(head, f, -HEAD_BACK);
            float fromBack = sqrtf((eyes[e].x - back.x) * (eyes[e].x - back.x) +
                                   (eyes[e].y - back.y) * (eyes[e].y - back.y));
            CHECK(fromBack + 3.1f <= HEAD_RADIUS);  // eye whites sit inside the head
        }
        CHECK(fabsf((eyes[0].x + eyes[1].x) / 2 - (head.x + 2.5f * f.x)) < 1e-4f);  // symmetric
        CHECK(fabsf((eyes[0].y + eyes[1].y) / 2 - (head.y + 2.5f * f.y)) < 1e-4f);
    }
}

// The snake's derived colors stay readable in every palette and mid-shift.
static void testSnakeColors(void) {
    for (int i = 0; i < PALETTE_COUNT; i++) {
        for (int j = 0; j < PALETTE_COUNT; j++) {
            for (int k = 0; k <= 10; k += (i == j ? 10 : 1)) {
                Palette p = blendPalette(&PALETTES[i], &PALETTES[j], k / 10.0f);
                Color headColor = ColorLerp(p.body, p.head, 0.45f);
                Color outline = ColorLerp(p.body, p.background, 0.8f);
                Color stripe = ColorLerp(p.body, p.head, 0.35f);
                CHECK(contrast(headColor, p.board) >= 4.5f);    // head stands out from the board
                CHECK(distance(headColor, p.body) >= 25.0f);    // and from its own body
                CHECK(contrast(outline, headColor) >= 3.0f);    // eye rims and head edge
                CHECK(contrast(p.background, p.head) >= 7.0f);  // pupils on eye whites
                CHECK(distance(stripe, p.body) >= 20.0f);       // back stripe is visible
            }
        }
    }
}

// Playback checks need an audio device; they are skipped when none is present.
static void testPlayback(void) {
    Sfx sfx;
    loadSfx(&sfx);
    if (!sfx.ready) {
        printf("SKIP  playback tests (no audio device)\n");
        return;
    }
    playSounds(&sfx, SFX_EAT | SFX_LEVEL_UP);
    CHECK(IsSoundPlaying(sfx.eat) && IsSoundPlaying(sfx.levelUp) && !IsSoundPlaying(sfx.gameOver));
    StopSound(sfx.eat); StopSound(sfx.levelUp);
    playSounds(&sfx, 0);
    CHECK(!IsSoundPlaying(sfx.eat) && !IsSoundPlaying(sfx.levelUp));
    sfx.muted = true;
    playSounds(&sfx, SFX_EAT | SFX_LEVEL_UP | SFX_GAME_OVER);
    play(&sfx, sfx.start);
    CHECK(!IsSoundPlaying(sfx.eat) && !IsSoundPlaying(sfx.levelUp));
    CHECK(!IsSoundPlaying(sfx.gameOver) && !IsSoundPlaying(sfx.start));
    sfx.muted = false;
    playSounds(&sfx, SFX_GAME_OVER);
    CHECK(IsSoundPlaying(sfx.gameOver));
    unloadSfx(&sfx);
    CHECK(!IsAudioDeviceReady());
}

int main(void) {
    SetTraceLogLevel(LOG_WARNING);
    SetRandomSeed(12345);
    testSpeedCap();
    testEndlessProgression();
    testPlainMove();
    testCollisions();
    testRestartAfterDeath();
    testFullBoard();
    testPalettes();
    testPaletteRotation();
    testTransition();
    testSnakeShape();
    testSnakeColors();
    testPlayback();
    printf("%d checks, %d failures\n", checks, failures);
    return failures != 0;
}
