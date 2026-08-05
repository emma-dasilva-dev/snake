
# Snake Game in C

A terminal-based Snake game written in C using the ncurses library.

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
git clone YOUR_REPOSITORY_LINK
cd snakeio
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
git clone YOUR_REPOSITORY_LINK
cd snakeio
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
git clone YOUR_REPOSITORY_LINK
cd snakeio
gcc -Wall -Wextra snake.c -o snake -lncurses
./snake
```

## Project Structure

```text
snakeio/
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

## Author

Emma Da Silva  
Junior Full-Stack Developer exploring software engineering, Linux, C, and cybersecurity.
