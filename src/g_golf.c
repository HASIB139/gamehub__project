#include "hub.h"

/* Mini Golf - 8 levels (walls, sand, ponds) */

#define BALL_R  12.0f
#define HOLE_R  16.0f
#define FRICTION 120.0f
#define MAX_LEVELS 8

typedef struct {
    Vector2 start, hole;
    Rectangle walls[8];
    int wallN;
    Rectangle sand[4];
    int sandN;
    Vector2 ponds[4];
    float pondR[4];
    int pondN;
    const char *name;
    Color grass;
} GolfLevel;

static void load_level(GolfLevel *L, int id) {
    memset(L, 0, sizeof *L);
    L->grass = (Color){ 42, 140, 62, 255 };
    switch (id) {
    case 0:
        L->name = "1  Open Fairway";
        L->start = (Vector2){ 100, SH/2.f };
        L->hole  = (Vector2){ 900, SH/2.f };
        break;
    case 1:
        L->name = "2  Pillars";
        L->start = (Vector2){ 90, SH/2.f };
        L->hole  = (Vector2){ 910, SH/2.f };
        L->walls[0] = (Rectangle){ 250, 40, 32, 360 };
        L->walls[1] = (Rectangle){ 480, 220, 32, 360 };
        L->walls[2] = (Rectangle){ 710, 40, 32, 360 };
        L->wallN = 3;
        break;
    case 2:
        L->name = "3  Sand Channels";
        L->start = (Vector2){ 80, SH/2.f };
        L->hole  = (Vector2){ 920, SH/2.f };
        L->sand[0] = (Rectangle){ 220, 50, 90, SH-60 };
        L->sand[1] = (Rectangle){ 700, 50, 90, SH-60 };
        L->sandN = 2;
        break;
    case 3:
        L->name = "4  Twin Ponds";
        L->start = (Vector2){ 80, 520 };
        L->hole  = (Vector2){ 920, 100 };
        L->ponds[0] = (Vector2){ 350, 300 }; L->pondR[0] = 70;
        L->ponds[1] = (Vector2){ 650, 350 }; L->pondR[1] = 55;
        L->pondN = 2;
        L->walls[0] = (Rectangle){ 500, 0, 28, 250 };
        L->wallN = 1;
        break;
    case 4:
        L->name = "5  Dogleg";
        L->start = (Vector2){ 80, 520 };
        L->hole  = (Vector2){ 900, 100 };
        L->walls[0] = (Rectangle){ 300, 200, 28, 450 };
        L->walls[1] = (Rectangle){ 550, 0, 28, 400 };
        L->walls[2] = (Rectangle){ 750, 250, 28, 400 };
        L->wallN = 3;
        break;
    case 5:
        L->name = "6  Corridor";
        L->start = (Vector2){ 70, SH/2.f };
        L->hole  = (Vector2){ 930, SH/2.f };
        L->walls[0] = (Rectangle){ 250, 0, 24, 240 };
        L->walls[1] = (Rectangle){ 250, 400, 24, 250 };
        L->walls[2] = (Rectangle){ 500, 150, 24, 340 };
        L->walls[3] = (Rectangle){ 720, 0, 24, 260 };
        L->walls[4] = (Rectangle){ 720, 380, 24, 270 };
        L->wallN = 5;
        break;
    case 6:
        L->name = "7  Maze";
        L->start = (Vector2){ 70, 100 };
        L->hole  = (Vector2){ 920, 550 };
        L->walls[0] = (Rectangle){ 200, 0, 24, 400 };
        L->walls[1] = (Rectangle){ 400, 200, 24, 450 };
        L->walls[2] = (Rectangle){ 600, 0, 24, 380 };
        L->walls[3] = (Rectangle){ 800, 180, 24, 470 };
        L->sand[0] = (Rectangle){ 450, 300, 120, 80 };
        L->sandN = 1;
        L->ponds[0] = (Vector2){ 300, 500 }; L->pondR[0] = 45;
        L->pondN = 1;
        L->wallN = 4;
        break;
    default:
        L->name = "8  Final Gauntlet";
        L->start = (Vector2){ 60, SH/2.f };
        L->hole  = (Vector2){ 940, SH/2.f };
        L->walls[0] = (Rectangle){ 180, 50, 26, 280 };
        L->walls[1] = (Rectangle){ 180, 380, 26, 250 };
        L->walls[2] = (Rectangle){ 360, 0, 26, 320 };
        L->walls[3] = (Rectangle){ 540, 280, 26, 360 };
        L->walls[4] = (Rectangle){ 720, 0, 26, 300 };
        L->walls[5] = (Rectangle){ 720, 400, 26, 250 };
        L->wallN = 6;
        L->ponds[0] = (Vector2){ 450, 200 }; L->pondR[0] = 40;
        L->ponds[1] = (Vector2){ 620, 480 }; L->pondR[1] = 50;
        L->pondN = 2;
        L->sand[0] = (Rectangle){ 300, 300, 80, SH-350 };
        L->sandN = 1;
        break;
    }
}

static void bounce(Vector2 *b, Vector2 *v, Rectangle w) {
    if (!CheckCollisionCircleRec(*b, BALL_R, w)) return;
    if (b->x < w.x) { b->x = w.x - BALL_R; v->x = -fabsf(v->x)*0.72f; }
    else if (b->x > w.x+w.width) { b->x = w.x+w.width+BALL_R; v->x = fabsf(v->x)*0.72f; }
    if (b->y < w.y) { b->y = w.y - BALL_R; v->y = -fabsf(v->y)*0.72f; }
    else if (b->y > w.y+w.height) { b->y = w.y+w.height+BALL_R; v->y = fabsf(v->y)*0.72f; }
}

void game_golf(Player *p) {
    int lid = 0, strokes = 0, sunk = 0, aiming = 0, done = 0;
    GolfLevel L;
    load_level(&L, lid);
    Vector2 ball = L.start, vel = {0,0};

    gfx_open("Mini Golf");

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) break;
        if (IsKeyPressed(KEY_R) && !done) {
            ball = L.start; vel = (Vector2){0,0}; sunk = 0;
        }

        /* ---------------- UPDATE ---------------- */
        float dt = GetFrameTime();
        if (!done && !sunk) {
            float fric = FRICTION;
            for (int i = 0; i < L.sandN; i++)
                if (CheckCollisionCircleRec(ball, BALL_R, L.sand[i]))
                    fric = FRICTION * 2.8f;

            ball.x += vel.x * dt; ball.y += vel.y * dt;
            float sp = sqrtf(vel.x*vel.x + vel.y*vel.y);
            if (sp > 0) {
                float ns = sp - fric * dt;
                if (ns < 0) ns = 0;
                vel.x = vel.x/sp*ns; vel.y = vel.y/sp*ns;
                if (ns < 12) vel = (Vector2){0,0};
            }

            if (ball.x < BALL_R) { ball.x = BALL_R; vel.x *= -0.7f; }
            if (ball.x > SW-BALL_R) { ball.x = SW-BALL_R; vel.x *= -0.7f; }
            if (ball.y < 56) { ball.y = 56; vel.y *= -0.7f; }
            if (ball.y > SH-BALL_R) { ball.y = SH-BALL_R; vel.y *= -0.7f; }

            for (int i = 0; i < L.wallN; i++) bounce(&ball, &vel, L.walls[i]);

            for (int i = 0; i < L.pondN; i++) {
                if (CheckCollisionCircles(ball, BALL_R*0.7f, L.ponds[i], L.pondR[i])) {
                    fx_burst(ball.x, ball.y, SKYBLUE, 25);
                    ball = L.start; vel = (Vector2){0,0}; strokes++;
                }
            }

            if (CheckCollisionCircles(ball, 7, L.hole, HOLE_R) && sp < 80) {
                sunk = 1;
                fx_burst(L.hole.x, L.hole.y, GOLD, 40);
                if (lid >= MAX_LEVELS - 1) {
                    done = 1;
                    if (p->best_golf == 0 || strokes < p->best_golf)
                        p->best_golf = strokes;
                }
            }

            if (sp < 12 && !sunk) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) aiming = 1;
                if (aiming && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    Vector2 m = GetMousePosition();
                    Vector2 d = { ball.x - m.x, ball.y - m.y };
                    float pwr = sqrtf(d.x*d.x+d.y*d.y) * 3.4f;
                    if (pwr > 900) pwr = 900;
                    if (pwr > 30) {
                        float len = sqrtf(d.x*d.x+d.y*d.y);
                        vel.x = d.x/len*pwr; vel.y = d.y/len*pwr;
                        strokes++;
                    }
                    aiming = 0;
                }
            }
        }

        if (sunk && !done && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))) {
            lid++;
            load_level(&L, lid);
            ball = L.start; vel = (Vector2){0,0}; sunk = 0;
        }

        /* ---------------- DRAW ---------------- */
        BeginDrawing();

        /* lawn with mowed stripes */
        ClearBackground(L.grass);
        for (int x = 0; x < SW; x += 100)
            DrawRectangle(x, 56, 50, SH - 56, ColorBrightness(L.grass, 0.07f));
        DrawRectangleGradientV(0, 58, SW, 40, Fade(BLACK, 0.25f), Fade(BLACK, 0.0f));

        /* sand: dark rim, light middle, little speckles */
        for (int i = 0; i < L.sandN; i++) {
            Rectangle s = L.sand[i];
            DrawRectangleRounded((Rectangle){ s.x - 4, s.y - 4, s.width + 8, s.height + 8 }, 0.3f, 8, (Color){ 170, 145, 85, 255 });
            DrawRectangleRounded(s, 0.3f, 8, (Color){ 225, 205, 140, 255 });
            for (int k = 0; k < 14; k++)
                DrawCircle((int)s.x + (k * 37) % (int)s.width, (int)s.y + (k * 53) % (int)s.height, 2, Fade(BROWN, 0.35f));
        }

        /* ponds: bank, deep water, shallow water, moving ripple */
        for (int i = 0; i < L.pondN; i++) {
            Vector2 c = L.ponds[i];
            float r = L.pondR[i];
            DrawCircleV(c, r + 6, (Color){ 25, 70, 40, 255 });
            DrawCircleV(c, r, (Color){ 35, 95, 170, 255 });
            DrawCircleV(c, r * 0.75f, (Color){ 55, 135, 205, 255 });
            float rip = fmodf((float)GetTime() * 0.6f + i * 0.3f, 1.0f);
            DrawCircleLines((int)c.x, (int)c.y, r * (0.2f + 0.6f * rip), Fade(WHITE, 0.5f * (1.0f - rip)));
            DrawCircleV((Vector2){ c.x - r * 0.35f, c.y - r * 0.35f }, r * 0.12f, Fade(WHITE, 0.35f));
        }

        /* wooden walls with shadow and a light edge */
        for (int i = 0; i < L.wallN; i++) {
            Rectangle w = L.walls[i];
            DrawRectangleRec((Rectangle){ w.x + 5, w.y + 6, w.width, w.height }, Fade(BLACK, 0.3f));
            DrawRectangleRec(w, (Color){ 95, 62, 32, 255 });
            DrawRectangleRec((Rectangle){ w.x + 3, w.y + 3, w.width - 6, w.height - 6 }, (Color){ 140, 98, 55, 255 });
            DrawRectangleRec((Rectangle){ w.x + 3, w.y + 3, 4, w.height - 6 }, (Color){ 175, 130, 80, 255 });
            DrawRectangleLinesEx(w, 2, (Color){ 55, 35, 18, 255 });
        }

        /* hole */
        DrawCircleV((Vector2){ L.hole.x + 2, L.hole.y + 3 }, HOLE_R + 4, Fade(BLACK, 0.3f));
        DrawCircleV(L.hole, HOLE_R + 4, (Color){ 30, 90, 40, 255 });
        DrawCircleV(L.hole, HOLE_R, BLACK);

        /* waving flag */
        float wave = 4.0f * sinf((float)GetTime() * 5.0f);
        Vector2 top = { L.hole.x, L.hole.y - 52 };
        DrawLineEx(L.hole, top, 3, LIGHTGRAY);
        Vector2 f2 = { top.x, top.y + 20 };
        Vector2 f3 = { top.x + 26, top.y + 10 + wave };
        DrawTriangle(top, f2, f3, RED);
        DrawTriangle(top, f3, f2, RED);   /* same triangle, other direction, so it always shows */

        /* ball (hidden once it drops in the hole) */
        if (!sunk) {
            DrawCircleV((Vector2){ ball.x + 3, ball.y + 5 }, BALL_R, Fade(BLACK, 0.35f));
            DrawCircleV(ball, BALL_R, (Color){ 190, 195, 210, 255 });
            DrawCircleV((Vector2){ ball.x - 2, ball.y - 2 }, BALL_R - 2.5f, RAYWHITE);
        }

        /* aiming: dotted guide + power bar */
        if (aiming) {
            Vector2 m = GetMousePosition();
            Vector2 d = { ball.x - m.x, ball.y - m.y };
            float len = sqrtf(d.x * d.x + d.y * d.y);
            if (len > 1.0f) {
                float frac = len * 3.4f / 900.0f;
                if (frac > 1.0f) frac = 1.0f;
                Vector2 u = { d.x / len, d.y / len };
                Color pc = { (unsigned char)(80 + 175 * frac), (unsigned char)(255 - 175 * frac), 70, 255 };

                DrawLineEx(ball, m, 2, Fade(WHITE, 0.35f));
                for (int k = 1; k <= 9; k++) {
                    float dist = 26.0f * k * (0.3f + frac);
                    DrawCircleV((Vector2){ ball.x + u.x * dist, ball.y + u.y * dist }, 5.0f - k * 0.3f, Fade(pc, 1.0f - k * 0.08f));
                }

                DrawText("POWER", 16, SH - 58, 16, RAYWHITE);
                DrawRectangle(16, SH - 36, 200, 16, Fade(BLACK, 0.5f));
                DrawRectangle(16, SH - 36, (int)(200 * frac), 16, pc);
                DrawRectangleLines(16, SH - 36, 200, 16, RAYWHITE);
            }
        }

        fx_update_draw();

        /* top bar */
        gfx_topbar(L.name, "drag to aim | R reset | ESC hub");
        gfx_text_shadow(TextFormat("Strokes %d    Best %d    Level %d/8", strokes, p->best_golf, lid + 1),
                        360, 18, 20, RAYWHITE);

        /* level dots */
        for (int i = 0; i < MAX_LEVELS; i++) {
            Color c = i < lid ? LIME : (i == lid ? GOLD : GRAY);
            DrawCircle(SW/2 - 70 + i * 20, SH - 18, 8, Fade(BLACK, 0.5f));
            DrawCircle(SW/2 - 70 + i * 20, SH - 18, 5, c);
        }

        /* messages */
        if (sunk && !done) {
            DrawRectangle(0, SH/2 - 60, SW, 120, Fade(BLACK, 0.55f));
            gfx_text_center("HOLE!", SW/2, SH/2 - 48, 44, GOLD);
            gfx_text_center("ENTER / SPACE  ->  next level", SW/2, SH/2 + 12, 22, RAYWHITE);
        }
        if (done) {
            DrawRectangle(0, 0, SW, SH, Fade(BLACK, 0.65f));
            gfx_panel((Rectangle){ SW/2 - 230, SH/2 - 110, 460, 220 }, (Color){ 25, 28, 50, 240 });
            gfx_text_center("COURSE COMPLETE", SW/2, SH/2 - 85, 40, GOLD);
            gfx_text_center(TextFormat("Total strokes: %d", strokes), SW/2, SH/2 - 20, 28, RAYWHITE);
            gfx_text_center("ESC to return to the hub", SW/2, SH/2 + 40, 20, LIGHTGRAY);
        }
        EndDrawing();
    }
    CloseWindow();
}
