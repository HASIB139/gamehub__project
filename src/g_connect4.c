#include <stddef.h>
#include "hub.h"

#define R 6
#define C 7

#define COL_PLAYER (Color){ 232, 52, 62, 255 }
#define COL_CPU    (Color){ 252, 200, 40, 255 }

/* Drop a piece for player p (1 = you, 2 = CPU) into column col.
   Returns 1 if it worked, 0 if the column is full or invalid. */
static int drop(int b[R][C], int col, int p) {
    if (col < 0 || col >= C || b[0][col]) return 0;
    for (int r = R - 1; r >= 0; r--)
        if (!b[r][col]) { b[r][col] = p; return 1; }
    return 0;
}

/* Look for four in a row for player p.
   If found, returns 1 and (when out is not NULL) writes the 4 cells into out. */
static int find_four(int b[R][C], int p, int out[4][2]) {
    static const int dr[4] = { 0, 1, 1,  1 };   /* right, down, down-right, down-left */
    static const int dc[4] = { 1, 0, 1, -1 };
    for (int r = 0; r < R; r++)
        for (int c = 0; c < C; c++)
            for (int d = 0; d < 4; d++) {
                int ok = 1;
                for (int k = 0; k < 4 && ok; k++) {
                    int rr = r + dr[d] * k, cc = c + dc[d] * k;
                    if (rr < 0 || rr >= R || cc < 0 || cc >= C || b[rr][cc] != p) ok = 0;
                }
                if (ok) {
                    if (out)
                        for (int k = 0; k < 4; k++) {
                            out[k][0] = r + dr[d] * k;
                            out[k][1] = c + dc[d] * k;
                        }
                    return 1;
                }
            }
    return 0;
}

static int four(int b[R][C], int p) { return find_four(b, p, NULL); }

static int board_full(int b[R][C]) {
    for (int c = 0; c < C; c++)
        if (b[0][c] == 0) return 0;
    return 1;
}

/* The CPU's move: win if it can, block if it must, else center, else random. */
static void cpu(int b[R][C]) {
    /* 1. can the CPU win right now? */
    for (int c = 0; c < C; c++) {
        if (!drop(b, c, 2)) continue;
        if (four(b, 2)) return;                  /* yes: keep the winning move */
        for (int r = 0; r < R; r++)              /* no: undo the test move */
            if (b[r][c] == 2) { b[r][c] = 0; break; }
    }
    /* 2. could the player win next turn? then block that column */
    for (int c = 0; c < C; c++) {
        if (!drop(b, c, 1)) continue;
        int n = four(b, 1);
        for (int r = 0; r < R; r++)
            if (b[r][c] == 1) { b[r][c] = 0; break; }
        if (n) { drop(b, c, 2); return; }
    }
    /* 3. prefer the center column */
    if (drop(b, 3, 2)) return;
    /* 4. otherwise try random columns */
    for (int i = 0; i < 30; i++)
        if (drop(b, GetRandomValue(0, C - 1), 2)) return;
}

/* A glossy disc: dark edge, bright body, inner ring, shine. */
static void draw_disc(float cx, float cy, float rad, Color base) {
    Vector2 c = { cx, cy };
    DrawCircleV((Vector2){ cx + 1, cy + 2 }, rad, Fade(BLACK, 0.3f));       /* shadow */
    DrawCircleV(c, rad, ColorBrightness(base, -0.35f));                      /* dark edge */
    DrawCircleV(c, rad * 0.86f, base);                                       /* body */
    DrawCircleV(c, rad * 0.62f, ColorBrightness(base, -0.15f));              /* ring... */
    DrawCircleV(c, rad * 0.52f, base);                                       /* ...made of two circles */
    DrawCircleV((Vector2){ cx - rad * 0.32f, cy - rad * 0.34f }, rad * 0.18f, Fade(WHITE, 0.5f)); /* shine */
}

void game_connect4(Player *p) {
    (void)p;
    int b[R][C] = {0};
    int over = 0, msg = 0, haswin = 0;
    int win[4][2] = {{0}};
    float cell = 70, ox = (SW - C * cell) / 2, oy = 130;

    gfx_open("Connect Four");

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) break;
        if (IsKeyPressed(KEY_R)) { memset(b, 0, sizeof b); over = msg = haswin = 0; }

        /* ---------------- UPDATE ---------------- */
        int hover = (int)floorf((GetMouseX() - ox) / cell);   /* column under the mouse */

        if (!over && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (drop(b, hover, 1)) {
                if (four(b, 1)) { over = 1; msg = 1; }
                else {
                    cpu(b);
                    if (four(b, 2)) { over = 1; msg = 2; }
                }
                if (!over && board_full(b)) { over = 1; msg = 3; }

                if (msg == 1 || msg == 2) {          /* remember the winning four + celebrate */
                    haswin = find_four(b, msg, win);
                    for (int k = 0; k < 4 && haswin; k++)
                        fx_burst(ox + win[k][1] * cell + cell / 2, oy + win[k][0] * cell + cell / 2,
                                 msg == 1 ? GOLD : ORANGE, 14);
                }
            }
        }

        /* ---------------- DRAW ---------------- */
        BeginDrawing();
        gfx_background((Color){ 24, 50, 105, 255 }, (Color){ 10, 20, 50, 255 });
        gfx_topbar("CONNECT FOUR", "click a column | R restart | ESC hub");

        /* ghost disc above the board shows where your piece will go */
        if (!over && hover >= 0 && hover < C) {
            float bob = 3.0f * sinf((float)GetTime() * 5.0f);
            DrawCircleV((Vector2){ ox + hover * cell + cell / 2, oy - 40 + bob }, cell / 2 - 9, Fade(COL_PLAYER, 0.6f));
        }

        /* board */
        gfx_panel((Rectangle){ ox - 14, oy - 14, C * cell + 28, R * cell + 28 }, (Color){ 30, 80, 190, 255 });

        for (int r = 0; r < R; r++) for (int c = 0; c < C; c++) {
            float cx = ox + c * cell + cell / 2, cy = oy + r * cell + cell / 2, rad = cell / 2 - 7;

            /* the hole */
            DrawCircleV((Vector2){ cx, cy }, rad + 2, (Color){ 18, 50, 130, 255 });
            DrawCircleV((Vector2){ cx, cy }, rad, (Color){ 8, 18, 42, 255 });
            DrawCircleV((Vector2){ cx, cy + 2 }, rad - 3, (Color){ 14, 28, 62, 255 });

            if (b[r][c]) {
                /* pulsing glow behind the four winning discs */
                int isw = 0;
                if (haswin)
                    for (int k = 0; k < 4; k++)
                        if (win[k][0] == r && win[k][1] == c) isw = 1;
                if (isw)
                    DrawCircleV((Vector2){ cx, cy }, rad + 6, Fade(WHITE, 0.45f + 0.3f * sinf((float)GetTime() * 8.0f)));

                draw_disc(cx, cy, rad - 2, b[r][c] == 1 ? COL_PLAYER : COL_CPU);
            }
        }

        /* faint highlight on the column under the mouse */
        if (!over && hover >= 0 && hover < C)
            DrawRectangle((int)(ox + hover * cell), (int)oy, (int)cell, (int)(R * cell), Fade(WHITE, 0.07f));

        fx_update_draw();

        /* bottom bar */
        if (over) {
            DrawRectangleGradientV(0, SH - 74, SW, 74, Fade(BLACK, 0.45f), Fade(BLACK, 0.75f));
            DrawRectangle(0, SH - 74, SW, 2, GOLD);
            const char *t = msg == 1 ? "YOU WIN!" : (msg == 2 ? "CPU WINS" : "DRAW");
            Color tc = msg == 1 ? GREEN : (msg == 2 ? ORANGE : LIGHTGRAY);
            gfx_text_shadow(t, 40, SH - 54, 36, tc);
            const char *h = "press R to play again";
            DrawText(h, SW - MeasureText(h, 22) - 30, SH - 44, 22, LIGHTGRAY);
        } else {
            gfx_text_shadow("Your move: click a column", 40, SH - 44, 22, RAYWHITE);
        }
        EndDrawing();
    }
    CloseWindow();
}
