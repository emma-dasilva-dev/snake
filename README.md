
# Snake Game in C

A terminal-based Snake game written in C using the ncurses library.

## Play in your browser

A browser-based edition is included in [index.html](./index.html). It runs with no build step or external dependencies.

Once GitHub Pages is enabled for the repository, play at **https://emma-dasilva-dev.github.io/snake/**.

To enable Pages: **Settings → Pages → Build and deployment → Deploy from a branch → master → /(root) → Save**.

You can also open `index.html` locally in a modern browser. Use arrow keys or WASD to move, Space to pause, R to restart, or use touch controls on a phone. High scores are saved in the browser.

The original C/ncurses version remains available below.

---

## Features

- Keyboard-controlled snake movement
- Random food generation
- Increasing speed and difficulty
- Level system
- Collision detection
- Terminal-based interface

## Controls

| Key | Action |
|-----|--------|
| W | Move up |
| S | Move down |
| A | Move left |
| D | Move right |
| Q | Quit the game |

Both uppercase and lowercase letters work.

## Requirements

The game requires:

- GCC
- The ncurses development library
- A Linux or macOS terminal

## Run on Ubuntu or Debian

Install the required tools:

```bash
sudo apt update
sudo apt install gcc libncurses-dev
```

Clone the repository:

```bash
git clone https://github.com/emma-dasilva-dev/snake.git
cd snake
```

Compile the game:

```bash
gcc -Wall -Wextra snake.c -o snake -lncurses
```

Run it:

```bash
./snake
```

## Run on macOS

Install the compiler tools if necessary:

```bash
xcode-select --install
```

Clone and enter the repository:

```bash
git clone https://github.com/emma-dasilva-dev/snake.git
cd snake
```

Compile:

```bash
gcc -Wall -Wextra snake.c -o snake -lncurses
```

Run:

```bash
./snake
```

## Run on Windows

This version does not run directly in the normal Windows Command Prompt because it uses ncurses.

Windows users can use one of these options:

### Option 1: Run it online

Import the GitHub repository into an online Linux coding environment such as:

- LabEx
- Replit
- GitHub Codespaces

Then compile it with:

```bash
gcc -Wall -Wextra snake.c -o snake -lncurses
./snake
```

### Option 2: Use WSL

Install Windows Subsystem for Linux from PowerShell:

```powershell
wsl --install
```

After restarting the computer, open Ubuntu and install the requirements:

```bash
sudo apt update
sudo apt install gcc libncurses-dev git
```

Then clone, compile, and run the project:

```bash
git clone https://github.com/emma-dasilva-dev/snake.git
cd snake
gcc -Wall -Wextra snake.c -o snake -lncurses
./snake
```

## Project Structure

```text
snake/
├── snake.c
└── README.md
```

## What I Learned

Through this project, I practised:

- Writing and compiling C programs
- Working with functions and structures
- Using pointers and dynamic memory
- Handling keyboard input
- Building a game loop
- Using the ncurses library
- Debugging compiler errors and warnings
