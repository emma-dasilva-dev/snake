# Snake — Raylib desktop edition

This graphical version is written in **C + raylib**. The original `snake.c` terminal/ncurses edition is preserved.

## Controls

- WASD or arrow keys — steer
- P or Space — pause/resume during play
- Enter or Space — start/restart from a menu
- M — mute/unmute sound effects (works on any screen; "SOUND OFF" shows while muted)
- V — turn visual effects on/off (works on any screen; "EFFECTS OFF" shows while they are off)
- Escape — close window

## Endless gameplay

The game is an endless survival run on a 30 × 20 board: there is no fixed level limit; a board-completion screen appears only if all 600 cells are filled. Each food item adds 10 points, grows the snake by one cell and advances one level, and a new food item appears on a random free cell. The run ends only when the snake hits a wall or its own body.

**Speed and cap.** The snake starts at 150 ms per move and gets 10 ms faster every three levels. That gives seven gentle steps until it reaches the 80 ms cap (12.5 moves per second) at level 22. From then on the speed stays the same and the header shows `MAX SPEED`:

| Levels | Milliseconds per move |
|---|---|
| 1–3 | 150 |
| 4–6 | 140 |
| 7–9 | 130 |
| 10–12 | 120 |
| 13–15 | 110 |
| 16–18 | 100 |
| 19–21 | 90 |
| 22+ | 80 (cap) |

The curve is set by `BASE_INTERVAL_MS`, `SPEED_STEP_MS` and `MIN_INTERVAL_MS` in `desktop/snake_raylib.c`. The number of speed steps is calculated from those three values.

**Board complete.** If the snake ever fills all 600 cells, no space is left for food, so the run ends with a `BOARD COMPLETE!` screen. Press Enter or Space to play again.

## Build (Windows, Linux or macOS)

Prerequisites: Git, CMake 3.20+, a C compiler (e.g. Visual Studio C++ Build Tools on Windows, MinGW-w64, or GCC/Clang). CMake will download raylib 5.5 during the first configure, so Internet access is required for that step.

From the repository directory:

```sh
cmake -S . -B build
cmake --build build --config Release
```

Run the executable:
- **Windows (Visual Studio):** `build\\Release\\snake_desktop.exe`
- **Windows (MinGW):** `build\\snake_desktop.exe`
- **Linux/macOS (single-config generators):** `./build/snake_desktop`

On Linux, raylib may require additional X11/OpenGL development libraries. See the [official raylib build instructions](https://github.com/raysan5/raylib/wiki) for your distribution.

## Tests

The same build also produces `snake_tests`, which covers scoring, growth, food spawning, progression past level 12, the speed cap, collisions, restart, the full-board case, sound triggers, palette contrast and rotation, color-shift timing, and the snake's shape, eye direction and colors:

```sh
ctest --test-dir build -C Release --output-on-failure
```

The tests need no window. The few that check sound playback are skipped when there is no audio device.

## Architecture

`desktop/snake_raylib.c` contains a simple fixed-grid C game model, input handling, a fixed-step update, and rendering. The old ncurses version remains available at `snake.c`. The two versions share no source files, because the original intertwines terminal drawing and game logic, and the desktop edition's rules have since moved to endless play.

No graphical assets, audio files or external fonts are required.

## Color shift

Each time the snake eats, the whole color theme changes:
- **Smooth change.** Background, board, grid, border, snake, text and fruit blend into a new palette over 350 ms.
- **New fruit.** It appears straight away in the new palette's fruit color and pops in to full size.
- **Pulse.** A faint ring spreads out from the spot where the food was eaten.

There are 8 palettes: Classic (the original look), Ocean, Ember, Grape, Mint, Desert, Neon and Frost. The next palette is picked at random but is never the same as the current one. If the snake eats again mid-change, the new change starts from the colors already on screen, so the colors never jump.

Every palette keeps a dark board and a bright fruit in a hue well away from the snake's, so the fruit stays easy to see, including partway through a change. The tests check the contrast for every palette and every in-between step.

Press **V** to turn the effects off. The game returns to the Classic colors and stops changing palettes or pulsing. The effects are presentation only: they never affect movement timing, scoring, progression, sounds, pause or restart.

## Snake design

The snake is drawn as a bold arcade character, using only raylib's built-in shapes:
- **Body:** chunky and connected, with rounded corners where it turns, a dark outline and a light stripe down its back.
- **Tail:** narrows over the last three segments, or fewer on a short snake.
- **Head:** a lighter, bullet-shaped head, wider than the body, with outlined eyes and slit pupils that face the direction of travel.
- **Tongue:** flicks briefly every few seconds during play, but only when there is open board ahead.

All of the snake's colors come from the current palette, so it follows every color shift. Every part still sits on the grid, and tight coils keep a visible gap between rows. Gameplay and collisions are unchanged: they work on grid cells, not on the drawn shapes.

Turning visual effects off with **V** also stops the tongue flick.

## Sound effects

Sound effects are synthesized at startup with raylib's audio API (short triangle-wave tones), so there are no audio files to ship:

- **Eat** — quick rising blip on every food item
- **Level up** — ascending four-note arpeggio when the snake speeds up (every third level from 4 to 22, then never again once top speed is reached) and once on a completed board
- **Game over** — descending tones when the run ends in a collision
- **Start** — soft two-note chime when a game starts or restarts

If no audio device is available, the game runs silently.
