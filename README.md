# Snake — C + Raylib

An endless arcade Snake game for desktop, built in C with [Raylib](https://www.raylib.com/). Eat fruit to grow, earn points, speed up, and change the game's color theme. Includes sound effects and a stylized snake.

## Play on Ubuntu

Requires a graphical desktop, Git, CMake, a C compiler, and an internet connection for the first Raylib build.

```bash
sudo apt update
sudo apt install -y git cmake build-essential libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev

git clone -b feature/raylib-desktop https://github.com/emma-dasilva-dev/snake.git
cd snake
cmake -S . -B build
cmake --build build -j2
./build/snake_desktop
```

## Controls

| Key | Action |
| --- | --- |
| WASD / Arrow keys | Move |
| Enter / Space | Start or restart |
| P / Space | Pause or resume |
| M | Mute sound |
| V | Toggle color effects |
| Esc | Quit |

More details: [DESKTOP.md](DESKTOP.md).

## Original terminal version

The original `snake.c` uses **ncurses** and was written while following a tutorial on [LabEx](https://labex.io/); it is **not claimed as original code**. The desktop adaptation was developed with AI assistance as a learning project.

To build the terminal version on Ubuntu:

```bash
sudo apt install -y libncurses-dev
gcc snake.c -o snake -lncurses
./snake
```

The original tutorial's redistribution rights have not yet been verified.
