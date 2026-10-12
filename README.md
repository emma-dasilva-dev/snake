# Snake — C + Raylib

An endless desktop arcade Snake game with sound effects, eight color themes, and a stylized snake.

## Run on Ubuntu

Requires an Ubuntu graphical desktop and an internet connection for the first build.

**1. Install dependencies**

```bash
sudo apt update
sudo apt install -y git cmake build-essential \
  libasound2-dev libx11-dev libxrandr-dev libxi-dev \
  libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev
```

**2. Clone and build**

```bash
git clone -b feature/raylib-desktop https://github.com/emma-dasilva-dev/snake.git
cd snake
cmake -S . -B build
cmake --build build -j2
```

**3. Play**

```bash
./build/snake_desktop
```

Raylib 5.5 is downloaded automatically by CMake. To run tests, use `ctest --test-dir build --output-on-failure`.

## Controls

| Key | Action |
| --- | --- |
| WASD / Arrow keys | Move |
| Enter / Space | Start or restart |
| P / Space | Pause or resume |
| M | Mute sound |
| V | Toggle color effects |
| Esc | Quit |

See [DESKTOP.md](DESKTOP.md) for more details.

## Attribution

The original terminal version (`snake.c`) follows a [LabEx](https://labex.io/) tutorial and is not claimed as original code. The Raylib desktop adaptation was developed with AI assistance as a learning project. The original tutorial's redistribution rights have not yet been verified.
