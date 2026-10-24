#ifndef HUB_H
#define HUB_H

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

/* ---------- macros ---------- */
#define SW              1000
#define SH              650
#define MAX_NAME        32
#define SCORE_FILE      "data/scores.txt"
#define ARRAY_LEN(a)    ((int)(sizeof(a)/sizeof((a)[0])))
#define CLAMP(x,a,b)    ((x)<(a)?(a):((x)>(b)?(b):(x)))

#ifdef _WIN32
  #define CLEAR_CMD "cls"
#else
  #define CLEAR_CMD "clear"
#endif

/* ---------- structs / union (course topics) ---------- */
typedef struct {
    char name[MAX_NAME];
    int  wins[16];   
    int  best_golf;
    int  best_candy;
    int  best_snake;
} Player;

/* union: same memory as int or as 4 chars (demo for viva) */
typedef union {
    int   as_int;
    char  as_bytes[4];
} IntBytes;

typedef enum {
    G_CONNECT4 = 0,
    G_BLACKJACK,
    G_SNAKES,
    G_LUDO,
    G_SNAKE,
    G_WORD,
    G_CANDY,
    G_GOLF,
    G_PARKOUR,
    G_COUNT
} GameId;

void enable_console(void);
void shell_banner(const Player *p);
void shell_help(void);
void score_load(Player *p, const char *name);
void score_save(const Player *p);

void game_connect4(Player *p);
void game_blackjack(Player *p);
void game_snakes(Player *p);
void game_ludo(Player *p);
void game_snake(Player *p);
void game_word(Player *p);
void game_candy(Player *p);
void game_golf(Player *p);
void game_parkour(Player *p);

void gfx_open(const char *title);
void gfx_background(Color top, Color bottom);
void gfx_text_shadow(const char *t, int x, int y, int size, Color c);
void gfx_text_center(const char *t, int cx, int y, int size, Color c);
void gfx_topbar(const char *title, const char *hint);
void gfx_panel(Rectangle r, Color c);
void fx_burst(float x, float y, Color c, int n);
void fx_update_draw(void);

#define LB_GAME 16
#define LB_NAME 24
typedef struct { char game[LB_GAME]; char name[LB_NAME]; int score; } LbRow;

int  lb_top(const char *game, int low_is_better, LbRow *out, int max);
void lb_submit(const char *game, const char *name, int score, int low_is_better);
void lb_add(const char *game, const char *name, int delta);
void lb_overlay(const char *game, int low_is_better, const char *me);
void lb_print_all(void);

int auth_login(char *name_out, int size);
int auth_change_password(const char *name);

int gfx_pick_players(const char *game, const char *p1keys, const char *p2keys);

#endif
