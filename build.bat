@echo off
cd /d "%~dp0"

REM Use WIN64 raylib only (your gcc is 64-bit)
set RL=include\raylib-6.0_win64_mingw-w64
if not exist "%RL%\include\raylib.h" set RL=include\raylib-5.5_win64_mingw-w64
if not exist "%RL%\include\raylib.h" set RL=include\raylib-5.0_win64_mingw-w64

echo Looking for raylib at %RL%
if not exist "%RL%\include\raylib.h" (
  echo.
  echo Put win64 raylib here:
  echo   %CD%\include\raylib-6.0_win64_mingw-w64\include\raylib.h
  echo   %CD%\include\raylib-6.0_win64_mingw-w64\lib\libraylib.a
  echo.
  pause
  exit /b 1
)

echo Compiling...
gcc -Wall -Wextra -std=c99 -Iinclude -I"%RL%\include" -L"%RL%\lib" -o rayhub.exe ^
src/main.c src/util.c src/gfx.c src/leaderboard.c src/auth.c ^
src/g_connect4.c src/g_blackjack.c src/g_snakes.c src/g_ludo.c ^
src/g_snake.c src/g_word.c src/g_candy.c src/g_golf.c src/g_parkour.c ^
-lraylib -lopengl32 -lgdi32 -lwinmm


if errorlevel 1 (
  echo BUILD FAILED
  pause
  exit /b 1
)

echo BUILD OK
echo.
echo Run: rayhub.exe
echo CLI login then type: help / connect4 / blackjack / snakes / ludo / snake / word / candy / golf
rayhub.exe
