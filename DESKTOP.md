# Snake — Raylib desktop edition

This graphical version is written in **C + raylib**. The original `snake.c` terminal/ncurses edition is preserved.

## Controls

- WASD or arrow keys — steer
- P or Space — pause/resume during play
- Enter or Space — start/restart from a menu
- Escape — close window

Each food item adds 10 points and advances one level. Win after level 12; colliding with a wall or the snake ends the run. The board is 30 × 20 cells.

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

## Architecture

`desktop/snake_raylib.c` contains a simple fixed-grid C game model, input handling, a fixed-step update, and rendering. The old ncurses version remains available at `snake.c`. The two versions share gameplay rules, not source files, because the original intertwines terminal drawing and game logic.

No graphical assets or external fonts are required.
