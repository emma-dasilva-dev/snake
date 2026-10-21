# Snake — Desktop Arcade Game

A graphical **Snake game written in C with raylib 5.5**. Play locally on Linux, Windows, or macOS after building from source. The game has an animated snake, generated sound effects, eight shifting color themes, and speed that increases as you eat.

> **Desktop application — not a browser game.** Clone this branch and build it locally. A graphical desktop environment is required (on Windows with Ubuntu/WSL, this means working WSLg support).

## Features

- Classic grid-based Snake on a **30 × 20** board
- Endless progression: **+10 points and +1 level** per food item
- Increasing speed every three levels, capped at 80 ms per move at level 22
- Eight palettes with smooth transitions, a food pop animation, and a pulse effect
- Characterful snake rendering with directional eyes and animated tongue
- Procedurally synthesized sound effects, with mute and effects toggles
- Automated gameplay tests through CTest

## Quick start — Ubuntu / Debian

**Requirements:** graphical desktop (or WSLg), Git, CMake 3.20+, GCC and raylib's system development dependencies. Internet is needed the first time CMake fetches raylib 5.5.

```bash
sudo apt update
sudo apt install -y git cmake build-essential \
  libasound2-dev libx11-dev libxrandr-dev libxi-dev \
  libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev

git clone --branch feature/raylib-desktop https://github.com/emma-dasilva-dev/snake.git
cd snake
cmake -S . -B build
cmake --build build -j2
./build/snake_desktop
```

**Already cloned?** From the project directory, run `cmake --build build -j2` followed by `./build/snake_desktop`. If this branch is not checked out, run `git fetch origin` and `git switch feature/raylib-desktop` first (provided you have no conflicting local changes).

### Windows and macOS

Install Git, CMake 3.20+ and a C compiler (Visual Studio C++ Build Tools or MinGW-w64 on Windows; Xcode Command Line Tools on macOS). From the cloned **`feature/raylib-desktop`** branch:

```sh
cmake -S . -B build
cmake --build build --config Release
```

Launch the executable at `build/Release/snake_desktop.exe` (Windows Visual Studio generator), `build/snake_desktop.exe` (Windows MinGW), or `./build/snake_desktop` (macOS). You may need additional platform-specific graphics libraries; see [raylib's build documentation](https://github.com/raysan5/raylib/wiki).

## Controls

| Key | Action |
| --- | --- |
| WASD / arrow keys | Move |
| Enter / Space | Start or restart from a menu |
| P / Space | Pause or resume |
| M | Mute / unmute sound |
| V | Toggle color-shift visual effects |
| Escape | Quit |

## Run tests

After configuring and building:

```bash
ctest --test-dir build -C Release --output-on-failure
```

Tests cover movement, food placement, scoring, collisions, speed progression, restart, board completion, palette behavior, rendering-related calculations, and sound triggers. This README documents how to run them; it does **not** claim a fresh CI run or a tested build on every operating system.

## Project files

```text
desktop/snake_raylib.c      C / raylib desktop game
tests/snake_raylib_test.c   Automated game tests
CMakeLists.txt              Native build and raylib dependency
DESKTOP.md                  Gameplay, visuals, sound and implementation notes
snake.c                     Earlier terminal / ncurses version
```

The original terminal game is preserved as a reference. It does not share its game loop or drawing code with the desktop adaptation.

## More information

Read [DESKTOP.md](DESKTOP.md) for the exact speed curve, palette design, visual effects, sound behavior, cross-platform build notes, and test coverage.

## Project notes and attribution

This is a learning and portfolio project. The older `snake.c` terminal implementation follows a [LabEx tutorial](https://labex.io/); its original authorship is **not** claimed here. The raylib desktop adaptation was built with AI assistance. The tutorial code's redistribution terms have not been verified; review them before making licensing claims or distributing packaged binaries.

**Source:** [emma-dasilva-dev/snake](https://github.com/emma-dasilva-dev/snake) · **Desktop branch:** `feature/raylib-desktop`
