================================================================================
  GAME HUB  —  Linux-style CLI + raylib game windows
  CSE structured programming project
================================================================================

WHAT IT IS
  A multi-file C99 program with:
    - Terminal shell  (login, neofetch-style banner, player@hub:~$)
    - Eight raylib games opened as separate graphics windows
    - Scores saved to data/scores.txt

  Flow:
    1. Run the program -> type your name at login
    2. Shell prompt:  player@hub:~$
    3. Type a room name (e.g. golf) -> graphics window opens
    4. ESC closes the window -> back to the shell
    5. exit  to quit

GAMES (rooms)
  connect4     Connect Four vs CPU
  blackjack    Card table (hit / stand)
  snakes       Snakes & Ladders
  ludo         Ludo-style tokens
  snake        Classic snake
  word         5-letter word guess
  candy        Match-3
  golf         Mini golf — 8 levels (walls, sand, ponds)

SHELL COMMANDS
  help / man          list commands
  neofetch / fetch    system-style info card
  ls                  list rooms
  status              your scores
  clear               redraw banner
  union               demo union (course topic)
  exit / quit         logout

COURSE TOPICS COVERED
  variables, operators, conditionals, loops, functions,
  arrays, strings, pointers (Player*, FILE*),
  structures, union, macros, headers,
  file I/O, multi-file design, CLI + optional graphics (raylib)

PROJECT LAYOUT
  rayhub/
    include/hub.h
    src/main.c          shell loop
    src/util.c          banner, scores, console setup
    src/g_*.c           one file per game
    data/scores.txt
    build.bat           Windows helper
    README.txt          this file

================================================================================
  BUILD — WINDOWS (MinGW 64-bit)
================================================================================

1. Install a 64-bit MinGW GCC.

2. Download raylib for Windows MinGW 64-bit from:
     https://github.com/raysan5/raylib/releases
   Example package name: raylib-6.0_win64_mingw-w64

3. Extract so you have:
     rayhub/include/raylib-6.0_win64_mingw-w64/include/raylib.h
     rayhub/include/raylib-6.0_win64_mingw-w64/lib/libraylib.a

   IMPORTANT: use win64, not win32, if your gcc is 64-bit.

4. Open PowerShell in the rayhub folder and run:

gcc -Wall -Wextra -std=c99 -Iinclude -I"include/raylib-6.0_win64_mingw-w64/include" -L"include/raylib-6.0_win64_mingw-w64/lib" -o rayhub.exe src/main.c src/util.c src/g_connect4.c src/g_blackjack.c src/g_snakes.c src/g_ludo.c src/g_snake.c src/g_word.c src/g_candy.c src/g_golf.c -lraylib -lopengl32 -lgdi32 -lwinmm

.\rayhub.exe

  Or double-click build.bat if the raylib folder name matches.

5. Prefer Windows Terminal so ANSI colors in the shell look correct.

================================================================================
  BUILD — macOS
================================================================================

Raylib Windows libraries do NOT work on Mac. Install raylib for Mac, then
compile the SAME source with different link flags.

1. Install raylib (Homebrew):
     brew install raylib

2. From the rayhub folder:

gcc -Wall -Wextra -std=c99 -Iinclude -o rayhub \
  src/main.c src/util.c src/g_connect4.c src/g_blackjack.c \
  src/g_snakes.c src/g_ludo.c src/g_snake.c src/g_word.c \
  src/g_candy.c src/g_golf.c \
  -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

./rayhub

If headers/libs are not found (Apple Silicon Homebrew):

  add:  -I/opt/homebrew/include -L/opt/homebrew/lib

(Intel Mac Homebrew often uses /usr/local instead.)

You do NOT need to change the game source for Mac — only install Mac raylib
and use the Mac compile command.

================================================================================
  GITHUB / SHARING
================================================================================

Clone or zip of this repo is fine on Windows and Mac.

  - Same C source for everyone
  - Each person installs raylib for THEIR OS
  - Windows .exe does not run on Mac — Mac users must compile locally

Repo:
  https://github.com/Tanvir2159/cse-project-attempt-0

================================================================================
  TROUBLESHOOTING
================================================================================

- raylib.h not found
    Wrong -I path. Point -I at the folder that CONTAINS raylib.h.

- cannot find -lraylib
    Wrong -L path, or win32 library with 64-bit gcc. Use win64 package.

- Rectangle / CloseWindow redeclared
    windows.h conflicted with raylib. Current util.c uses NOGDI/NOUSER
    to avoid that — use the latest util.c from this project.

- Garbled banner characters
    Use Windows Terminal; enable_console() sets UTF-8 + ANSI.

- Mac friend cannot run .exe
    Expected. They brew install raylib and compile with the macOS section.

================================================================================
