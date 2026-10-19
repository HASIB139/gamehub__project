#include "hub.h"
#define N 8
#define T 5

static Color COLS[] = { RED, GREEN, BLUE, GOLD, PURPLE };

/* One simple shape per candy type, so each color also has its own shape. */
static void draw_shape(int t, float cx, float cy, float r, Color c) {
    switch (t) {
    case 0:  /* circle */
        DrawCircleV((Vector2){ cx, cy }, r, c);
        break;
    case 1:  /* rounded square */
        DrawRectangleRounded((Rectangle){ cx - r * 0.9f, cy - r * 0.9f, r * 1.8f, r * 1.8f }, 0.35f, 8, c);
        break;
    case 2:  /* diamond */
        DrawPoly((Vector2){ cx, cy }, 4, r * 1.2f, 0, c);
        break;
    case 3:  /* hexagon */
        DrawPoly((Vector2){ cx, cy }, 6, r, 0, c);
        break;
    default: /* triangle (point up) */
        DrawPoly((Vector2){ cx, cy }, 3, r * 1.15f, -90, c);
        break;
    }
}

/* A glossy gem: shadow, dark edge, bright body, soft highlight, sparkle. */
static void draw_gem(int t, float cx, float cy, float r, Color col) {
    draw_shape(t, cx + 2, cy + 3, r, Fade(BLACK, 0.3f));
    draw_shape(t, cx, cy, r, ColorBrightness(col, -0.35f));
    draw_shape(t, cx, cy - 1.5f, r * 0.84f, col);
    draw_shape(t, cx - r * 0.1f, cy - r * 0.2f, r * 0.45f, Fade(WHITE, 0.18f));
    DrawCircleV((Vector2){ cx - r * 0.32f, cy - r * 0.38f }, r * 0.13f, Fade(WHITE, 0.85f));
}

/* Little burst of sparks where a candy was cleared. */
static void pop(float ox, float oy, float cell, int r, int c, int color) {
    fx_burst(ox + c * cell + cell / 2, oy + r * cell + cell / 2, COLS[color % T], 8);
}

void game_candy(Player *p) {
    int g[N][N];
    for (int r = 0; r < N; r++) for (int c = 0; c < N; c++) g[r][c] = GetRandomValue(0, T - 1);
    int score = 0, moves = 15, sel = -1;
    float cell = 55, ox = (SW - N * cell) / 2, oy = 100;

    gfx_open("Candy");

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) break;

        /* R = start a new game */
        if (IsKeyPressed(KEY_R)) {
            for (int r = 0; r < N; r++) for (int c = 0; c < N; c++) g[r][c] = GetRandomValue(0, T - 1);
            score = 0; moves = 15; sel = -1;
        }

        /* ---------------- UPDATE ---------------- */
        if (moves > 0 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            /* floorf so clicks just outside the board are not counted as column/row 0 */
            int c = (int)floorf((GetMouseX() - ox) / cell);
            int r = (int)floorf((GetMouseY() - oy) / cell);
            if (r >= 0 && r < N && c >= 0 && c < N) {
                int id = r * N + c;
                if (sel < 0) sel = id;
                else {
                    int r1 = sel / N, c1 = sel % N;
                    if (abs(r1 - r) + abs(c1 - c) == 1) {
                        int t = g[r1][c1]; g[r1][c1] = g[r][c]; g[r][c] = t;

                        /* clear horizontal and vertical matches of 3 */
                        int cleared = 0;
                        for (int rr = 0; rr < N; rr++) for (int cc = 0; cc < N - 2; cc++)
                            if (g[rr][cc] == g[rr][cc + 1] && g[rr][cc] == g[rr][cc + 2] && g[rr][cc] >= 0) {
                                for (int k = 0; k < 3; k++) pop(ox, oy, cell, rr, cc + k, g[rr][cc]);
                                g[rr][cc] = g[rr][cc + 1] = g[rr][cc + 2] = -1; cleared += 3;
                            }
                        for (int cc = 0; cc < N; cc++) for (int rr = 0; rr < N - 2; rr++)
                            if (g[rr][cc] == g[rr + 1][cc] && g[rr][cc] == g[rr + 2][cc] && g[rr][cc] >= 0) {
                                for (int k = 0; k < 3; k++) pop(ox, oy, cell, rr + k, cc, g[rr][cc]);
                                g[rr][cc] = g[rr + 1][cc] = g[rr + 2][cc] = -1; cleared += 3;
                            }

                        if (!cleared) {
                            /* no match: swap back */
                            t = g[r1][c1]; g[r1][c1] = g[r][c]; g[r][c] = t;
                        } else {
                            score += cleared * 10; moves--;
                            /* gravity: drop pieces down, fill the top with new ones */
                            for (int cc = 0; cc < N; cc++) {
                                int w = N - 1;
                                for (int rr = N - 1; rr >= 0; rr--)
                                    if (g[rr][cc] >= 0) { g[w][cc] = g[rr][cc]; if (w != rr) g[rr][cc] = -1; w--; }
                                for (int rr = w; rr >= 0; rr--) g[rr][cc] = GetRandomValue(0, T - 1);
                            }
                            if (score > p->best_candy) p->best_candy = score;
                        }
                    }
                    sel = -1;
                }
            }
        }

        /* ---------------- DRAW ---------------- */
        BeginDrawing();
        gfx_background((Color){ 52, 22, 78, 255 }, (Color){ 22, 10, 38, 255 });
        gfx_topbar("CANDY", "click two neighbours to swap | R restart | ESC hub");

        /* score panel on the left */
        gfx_panel((Rectangle){ 25, oy, ox - 50, 150 }, (Color){ 36, 20, 60, 235 });
        gfx_text_shadow("SCORE", 45, (int)oy + 14, 20, LIGHTGRAY);
        gfx_text_shadow(TextFormat("%d", score), 45, (int)oy + 38, 36, GOLD);
        gfx_text_shadow(TextFormat("Moves  %d", moves), 45, (int)oy + 88, 24, RAYWHITE);
        DrawText(TextFormat("Best %d", p->best_candy), 45, (int)oy + 122, 18, LIGHTGRAY);

        /* board background */
        gfx_panel((Rectangle){ ox - 14, oy - 14, N * cell + 28, N * cell + 28 }, (Color){ 34, 18, 56, 255 });

        int hr = (int)floorf((GetMouseY() - oy) / cell);
        int hc = (int)floorf((GetMouseX() - ox) / cell);

        for (int r = 0; r < N; r++) for (int c = 0; c < N; c++) {
            float x = ox + c * cell, y = oy + r * cell;

            /* checkerboard tile */
            Color tile = ((r + c) % 2 == 0) ? (Color){ 52, 30, 80, 255 } : (Color){ 44, 24, 70, 255 };
            DrawRectangleRounded((Rectangle){ x + 1, y + 1, cell - 2, cell - 2 }, 0.2f, 6, tile);

            /* soft highlight under the mouse */
            if (moves > 0 && hr == r && hc == c)
                DrawRectangleRounded((Rectangle){ x + 1, y + 1, cell - 2, cell - 2 }, 0.2f, 6, Fade(WHITE, 0.12f));

            if (g[r][c] >= 0) {
                float pulse = (sel == r * N + c) ? 1.0f + 0.07f * sinf((float)GetTime() * 10.0f) : 1.0f;
                draw_gem(g[r][c] % T, x + cell / 2, y + cell / 2, cell * 0.38f * pulse, COLS[g[r][c] % T]);
            }
            if (sel == r * N + c)
                DrawRectangleLinesEx((Rectangle){ x + 2, y + 2, cell - 4, cell - 4 }, 3, WHITE);
        }

        fx_update_draw();

        if (moves <= 0) {
            DrawRectangle(0, 0, SW, SH, Fade(BLACK, 0.6f));
            gfx_panel((Rectangle){ SW / 2 - 200, SH / 2 - 90, 400, 180 }, (Color){ 36, 20, 60, 245 });
            gfx_text_center("OUT OF MOVES", SW / 2, SH / 2 - 65, 38, GOLD);
            gfx_text_center(TextFormat("Final score: %d", score), SW / 2, SH / 2 - 10, 26, RAYWHITE);
            gfx_text_center("R = play again    ESC = hub", SW / 2, SH / 2 + 40, 20, LIGHTGRAY);
        }
        EndDrawing();
    }
    CloseWindow();
}