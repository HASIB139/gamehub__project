#include "hub.h"

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

typedef struct {
    int rot_left, rot_right;
    int charge, release;
    int reset, next;
    int mouse_down, mouse_up;
    Vector2 mouse;
} PlayerInput;

typedef struct {
    Vector2 ball, vel;
    float angle, power, guide_t;
    int charging, aiming;
    int strokes, sunk;
} GPlayer;

typedef struct {
    GolfLevel L;
    GPlayer pl[2];
    int players, lid, done;
} Golf;

static const Color BALL_COL[3] = { { 190, 195, 210, 255 }, { 255, 150, 40, 255 }, { 70, 170, 255, 255 } };

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

static void read_player1_input(PlayerInput *in, int players) {
    memset(in, 0, sizeof *in);
    in->rot_left = IsKeyDown(KEY_LEFT);
    in->rot_right = IsKeyDown(KEY_RIGHT);
    in->charge = IsKeyDown(KEY_DOWN);
    in->release = IsKeyReleased(KEY_DOWN);
    in->reset = IsKeyPressed(KEY_UP);
    in->next = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    if (players == 1) {
        in->rot_left |= IsKeyDown(KEY_A);
        in->rot_right |= IsKeyDown(KEY_D);
        in->charge |= IsKeyDown(KEY_S);
        in->release |= IsKeyReleased(KEY_S);
        in->reset |= IsKeyPressed(KEY_W) | IsKeyPressed(KEY_R);
        in->mouse_down = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        in->mouse_up = IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
        in->mouse = GetMousePosition();
    }
}

static void read_player2_input(PlayerInput *in) {
    memset(in, 0, sizeof *in);
    in->rot_left = IsKeyDown(KEY_A);
    in->rot_right = IsKeyDown(KEY_D);
    in->charge = IsKeyDown(KEY_S);
    in->release = IsKeyReleased(KEY_S);
    in->reset = IsKeyPressed(KEY_W);
}

static Vector2 start_of(const Golf *g, int i) {
    Vector2 s = g->L.start;
    if (g->players == 2) s.y += (i == 0) ? -14.0f : 14.0f;
    return s;
}

static void place_ball(Golf *g, int i) {
    GPlayer *b = &g->pl[i];
    b->ball = start_of(g, i);
    b->vel = (Vector2){ 0, 0 };
    b->angle = atan2f(g->L.hole.y - b->ball.y, g->L.hole.x - b->ball.x);
    b->power = 0;
    b->charging = 0;
    b->aiming = 0;
}

static void start_level(Golf *g) {
    load_level(&g->L, g->lid);
    for (int i = 0; i < g->players; i++) {
        g->pl[i].sunk = 0;
        place_ball(g, i);
    }
}

static void reset_game(Golf *g, int players) {
    memset(g, 0, sizeof *g);
    g->players = players;
    start_level(g);
}

static int all_sunk(const Golf *g) {
    for (int i = 0; i < g->players; i++)
        if (!g->pl[i].sunk) return 0;
    return 1;
}

static void update_ball(Golf *g, int i, const PlayerInput *in, float dt) {
    GPlayer *b = &g->pl[i];
    GolfLevel *L = &g->L;
    if (b->sunk) return;

    if (b->guide_t > 0) b->guide_t -= dt;
    if (in->reset) place_ball(g, i);

    float fric = FRICTION;
    for (int k = 0; k < L->sandN; k++)
        if (CheckCollisionCircleRec(b->ball, BALL_R, L->sand[k])) fric = FRICTION * 2.8f;

    b->ball.x += b->vel.x * dt;
    b->ball.y += b->vel.y * dt;
    float sp = sqrtf(b->vel.x * b->vel.x + b->vel.y * b->vel.y);
    if (sp > 0) {
        float ns = sp - fric * dt;
        if (ns < 0) ns = 0;
        b->vel.x = b->vel.x / sp * ns;
        b->vel.y = b->vel.y / sp * ns;
        if (ns < 12) b->vel = (Vector2){ 0, 0 };
    }

    if (b->ball.x < BALL_R) { b->ball.x = BALL_R; b->vel.x *= -0.7f; }
    if (b->ball.x > SW - BALL_R) { b->ball.x = SW - BALL_R; b->vel.x *= -0.7f; }
    if (b->ball.y < 56) { b->ball.y = 56; b->vel.y *= -0.7f; }
    if (b->ball.y > SH - BALL_R) { b->ball.y = SH - BALL_R; b->vel.y *= -0.7f; }

    for (int k = 0; k < L->wallN; k++) bounce(&b->ball, &b->vel, L->walls[k]);

    for (int k = 0; k < L->pondN; k++) {
        if (CheckCollisionCircles(b->ball, BALL_R * 0.7f, L->ponds[k], L->pondR[k])) {
            fx_burst(b->ball.x, b->ball.y, SKYBLUE, 25);
            place_ball(g, i);
            b->strokes++;
        }
    }

    if (CheckCollisionCircles(b->ball, 7, L->hole, HOLE_R) && sp < 80) {
        b->sunk = 1;
        fx_burst(L->hole.x, L->hole.y, GOLD, 40);
        return;
    }

    if (sp < 12) {
        if (in->rot_left) { b->angle -= 2.4f * dt; b->guide_t = 1.5f; }
        if (in->rot_right) { b->angle += 2.4f * dt; b->guide_t = 1.5f; }
        if (in->charge) {
            b->charging = 1;
            b->power += 0.9f * dt;
            if (b->power > 1.0f) b->power = 1.0f;
            b->guide_t = 1.5f;
        }
        if (in->release && b->charging) {
            float pwr = b->power * 900.0f;
            if (pwr > 30.0f) {
                b->vel = (Vector2){ cosf(b->angle) * pwr, sinf(b->angle) * pwr };
                b->strokes++;
            }
            b->charging = 0;
            b->power = 0;
        }

        if (in->mouse_down) b->aiming = 1;
        if (b->aiming && in->mouse_up) {
            Vector2 d = { b->ball.x - in->mouse.x, b->ball.y - in->mouse.y };
            float len = sqrtf(d.x * d.x + d.y * d.y);
            float pwr = len * 3.4f;
            if (pwr > 900) pwr = 900;
            if (pwr > 30) {
                b->vel = (Vector2){ d.x / len * pwr, d.y / len * pwr };
                b->strokes++;
            }
            b->aiming = 0;
        }
    }
}

static void update_players(Golf *g, const PlayerInput *in1, const PlayerInput *in2, float dt) {
    if (g->done) return;
    for (int i = 0; i < g->players; i++)
        update_ball(g, i, (i == 0) ? in1 : in2, dt);

    if (all_sunk(g)) {
        if (g->lid >= MAX_LEVELS - 1) {
            g->done = 1;
        } else if (in1->next || in2->next) {
            g->lid++;
            start_level(g);
        }
    }
}

static void draw_course(const Golf *g) {
    const GolfLevel *L = &g->L;

    ClearBackground(L->grass);
    for (int x = 0; x < SW; x += 100)
        DrawRectangle(x, 56, 50, SH - 56, ColorBrightness(L->grass, 0.07f));
    DrawRectangleGradientV(0, 58, SW, 40, Fade(BLACK, 0.25f), Fade(BLACK, 0.0f));

    for (int i = 0; i < L->sandN; i++) {
        Rectangle s = L->sand[i];
        DrawRectangleRounded((Rectangle){ s.x - 4, s.y - 4, s.width + 8, s.height + 8 }, 0.3f, 8, (Color){ 170, 145, 85, 255 });
        DrawRectangleRounded(s, 0.3f, 8, (Color){ 225, 205, 140, 255 });
        for (int k = 0; k < 14; k++)
            DrawCircle((int)s.x + (k * 37) % (int)s.width, (int)s.y + (k * 53) % (int)s.height, 2, Fade(BROWN, 0.35f));
    }

    for (int i = 0; i < L->pondN; i++) {
        Vector2 c = L->ponds[i];
        float r = L->pondR[i];
        DrawCircleV(c, r + 6, (Color){ 25, 70, 40, 255 });
        DrawCircleV(c, r, (Color){ 35, 95, 170, 255 });
        DrawCircleV(c, r * 0.75f, (Color){ 55, 135, 205, 255 });
        float rip = fmodf((float)GetTime() * 0.6f + i * 0.3f, 1.0f);
        DrawCircleLines((int)c.x, (int)c.y, r * (0.2f + 0.6f * rip), Fade(WHITE, 0.5f * (1.0f - rip)));
        DrawCircleV((Vector2){ c.x - r * 0.35f, c.y - r * 0.35f }, r * 0.12f, Fade(WHITE, 0.35f));
    }

    for (int i = 0; i < L->wallN; i++) {
        Rectangle w = L->walls[i];
        DrawRectangleRec((Rectangle){ w.x + 5, w.y + 6, w.width, w.height }, Fade(BLACK, 0.3f));
        DrawRectangleRec(w, (Color){ 95, 62, 32, 255 });
        DrawRectangleRec((Rectangle){ w.x + 3, w.y + 3, w.width - 6, w.height - 6 }, (Color){ 140, 98, 55, 255 });
        DrawRectangleRec((Rectangle){ w.x + 3, w.y + 3, 4, w.height - 6 }, (Color){ 175, 130, 80, 255 });
        DrawRectangleLinesEx(w, 2, (Color){ 55, 35, 18, 255 });
    }

    DrawCircleV((Vector2){ L->hole.x + 2, L->hole.y + 3 }, HOLE_R + 4, Fade(BLACK, 0.3f));
    DrawCircleV(L->hole, HOLE_R + 4, (Color){ 30, 90, 40, 255 });
    DrawCircleV(L->hole, HOLE_R, BLACK);

    float wave = 4.0f * sinf((float)GetTime() * 5.0f);
    Vector2 top = { L->hole.x, L->hole.y - 52 };
    DrawLineEx(L->hole, top, 3, LIGHTGRAY);
    Vector2 f2 = { top.x, top.y + 20 };
    Vector2 f3 = { top.x + 26, top.y + 10 + wave };
    DrawTriangle(top, f2, f3, RED);
    DrawTriangle(top, f3, f2, RED);
}

static void draw_ball(Vector2 b, Color base) {
    DrawCircleV((Vector2){ b.x + 3, b.y + 5 }, BALL_R, Fade(BLACK, 0.35f));
    DrawCircleV(b, BALL_R, base);
    DrawCircleV((Vector2){ b.x - 2, b.y - 2 }, BALL_R - 2.5f, RAYWHITE);
}

static void draw_keyboard_guide(const GPlayer *b, Color c) {
    Vector2 u = { cosf(b->angle), sinf(b->angle) };
    float reach = 50.0f + 150.0f * b->power;
    DrawLineEx(b->ball, (Vector2){ b->ball.x + u.x * reach, b->ball.y + u.y * reach }, 2, Fade(c, 0.45f));
    for (int k = 1; k <= 6; k++) {
        float d = 24.0f * k * (0.35f + b->power);
        DrawCircleV((Vector2){ b->ball.x + u.x * d, b->ball.y + u.y * d }, 5.0f - k * 0.4f, Fade(c, 1.0f - k * 0.1f));
    }
    if (b->charging) {
        DrawRectangle((int)b->ball.x - 25, (int)b->ball.y - 32, 50, 7, Fade(BLACK, 0.5f));
        DrawRectangle((int)b->ball.x - 25, (int)b->ball.y - 32, (int)(50.0f * b->power), 7, c);
        DrawRectangleLines((int)b->ball.x - 25, (int)b->ball.y - 32, 50, 7, RAYWHITE);
    }
}

static void draw_mouse_aim(const GPlayer *b) {
    Vector2 m = GetMousePosition();
    Vector2 d = { b->ball.x - m.x, b->ball.y - m.y };
    float len = sqrtf(d.x * d.x + d.y * d.y);
    if (len <= 1.0f) return;
    float frac = len * 3.4f / 900.0f;
    if (frac > 1.0f) frac = 1.0f;
    Vector2 u = { d.x / len, d.y / len };
    Color pc = { (unsigned char)(80 + 175 * frac), (unsigned char)(255 - 175 * frac), 70, 255 };

    DrawLineEx(b->ball, m, 2, Fade(WHITE, 0.35f));
    for (int k = 1; k <= 9; k++) {
        float dist = 26.0f * k * (0.3f + frac);
        DrawCircleV((Vector2){ b->ball.x + u.x * dist, b->ball.y + u.y * dist }, 5.0f - k * 0.3f, Fade(pc, 1.0f - k * 0.08f));
    }
    DrawText("POWER", 16, SH - 58, 16, RAYWHITE);
    DrawRectangle(16, SH - 36, 200, 16, Fade(BLACK, 0.5f));
    DrawRectangle(16, SH - 36, (int)(200 * frac), 16, pc);
    DrawRectangleLines(16, SH - 36, 200, 16, RAYWHITE);
}

static void draw_stats(const Golf *g, const Player *p) {
    if (g->players == 1) {
        gfx_text_shadow(TextFormat("Strokes %d    Best %d    Level %d/8", g->pl[0].strokes, p->best_golf, g->lid + 1),
                        360, 18, 20, RAYWHITE);
        return;
    }
    gfx_text_shadow(TextFormat("Level %d/8", g->lid + 1), 360, 18, 20, RAYWHITE);
    const char *a = TextFormat("P1  strokes %d%s", g->pl[0].strokes, g->pl[0].sunk ? "  IN!" : "");
    gfx_text_shadow(a, 16, 66, 22, BALL_COL[1]);
    const char *b = TextFormat("P2  strokes %d%s", g->pl[1].strokes, g->pl[1].sunk ? "  IN!" : "");
    gfx_text_shadow(b, SW - MeasureText(b, 22) - 16, 66, 22, BALL_COL[2]);

    const char *h1a = "P1  LEFT/RIGHT aim";
    const char *h1b = "hold DOWN, release = shoot   UP = reset";
    const char *h2a = "P2  A/D aim";
    const char *h2b = "hold S, release = shoot   W = reset";
    DrawText(h1a, 16, SH - 62, 16, Fade(BALL_COL[1], 0.9f));
    DrawText(h1b, 16, SH - 42, 16, Fade(BALL_COL[1], 0.9f));
    DrawText(h2a, SW - MeasureText(h2a, 16) - 16, SH - 62, 16, Fade(BALL_COL[2], 0.9f));
    DrawText(h2b, SW - MeasureText(h2b, 16) - 16, SH - 42, 16, Fade(BALL_COL[2], 0.9f));
}

static void draw_messages(const Golf *g) {
    if (all_sunk(g) && !g->done) {
        DrawRectangle(0, SH/2 - 60, SW, 120, Fade(BLACK, 0.55f));
        gfx_text_center("HOLE!", SW/2, SH/2 - 48, 44, GOLD);
        gfx_text_center("ENTER / SPACE  ->  next level", SW/2, SH/2 + 12, 22, RAYWHITE);
    }
    if (!g->done) return;

    DrawRectangle(0, 0, SW, SH, Fade(BLACK, 0.65f));
    gfx_panel((Rectangle){ SW/2 - 230, SH/2 - 110, 460, 230 }, (Color){ 25, 28, 50, 240 });
    gfx_text_center("COURSE COMPLETE", SW/2, SH/2 - 90, 40, GOLD);
    if (g->players == 1) {
        gfx_text_center(TextFormat("Total strokes: %d", g->pl[0].strokes), SW/2, SH/2 - 20, 28, RAYWHITE);
        gfx_text_center("ESC return to hub    M mode", SW/2, SH/2 + 40, 20, LIGHTGRAY);
        return;
    }
    int a = g->pl[0].strokes, b = g->pl[1].strokes;
    gfx_text_center(TextFormat("P1  %d     P2  %d", a, b), SW/2, SH/2 - 35, 30, RAYWHITE);
    const char *msg = (a < b) ? "PLAYER 1 WINS" : (b < a) ? "PLAYER 2 WINS" : "IT'S A DRAW";
    Color col = (a < b) ? BALL_COL[1] : (b < a) ? BALL_COL[2] : RAYWHITE;
    gfx_text_center(msg, SW/2, SH/2 + 10, 32, col);
    gfx_text_center("fewest strokes wins    ESC hub    M mode", SW/2, SH/2 + 62, 18, LIGHTGRAY);
}

void game_golf(Player *p) {
    gfx_open("Mini Golf");
    int players = gfx_pick_players("MINI GOLF", "LEFT/RIGHT aim, hold DOWN to shoot (or the mouse)", "A/D aim, hold S to shoot");
    if (!players) {
        CloseWindow();
        return;
    }

    Golf g;
    reset_game(&g, players);
    int reported = 0;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) break;
        if (IsKeyPressed(KEY_M)) {
            int m = gfx_pick_players("MINI GOLF", "LEFT/RIGHT aim, hold DOWN to shoot (or the mouse)", "A/D aim, hold S to shoot");
            if (!m) break;
            players = m;
            reset_game(&g, players);
            reported = 0;
        }

        PlayerInput in1, in2;
        read_player1_input(&in1, players);
        read_player2_input(&in2);
        update_players(&g, &in1, &in2, GetFrameTime());

        if (g.done && !reported) {
            reported = 1;
            if (players == 1) {
                if (p->best_golf == 0 || g.pl[0].strokes < p->best_golf) p->best_golf = g.pl[0].strokes;
                lb_submit("golf", p->name, g.pl[0].strokes, 1);
            }
        }

        BeginDrawing();
        draw_course(&g);

        for (int i = 0; i < players; i++) {
            const GPlayer *b = &g.pl[i];
            if (b->sunk) continue;
            Color base = (players == 1) ? BALL_COL[0] : BALL_COL[i + 1];
            draw_ball(b->ball, base);
            int resting = (b->vel.x == 0 && b->vel.y == 0);
            if (resting && (players == 2 || b->guide_t > 0)) draw_keyboard_guide(b, (players == 1) ? WHITE : base);
            if (b->aiming) draw_mouse_aim(b);
        }

        fx_update_draw();

        gfx_topbar(g.L.name, players == 1 ? "drag or arrows to aim | R reset | M mode | ESC hub"
                                          : "M mode | TAB top 5 | ESC hub");
        draw_stats(&g, p);

        for (int i = 0; i < MAX_LEVELS; i++) {
            Color c = i < g.lid ? LIME : (i == g.lid ? GOLD : GRAY);
            DrawCircle(SW/2 - 70 + i * 20, SH - 18, 8, Fade(BLACK, 0.5f));
            DrawCircle(SW/2 - 70 + i * 20, SH - 18, 5, c);
        }

        draw_messages(&g);
        lb_overlay("golf", 1, p->name);
        EndDrawing();
    }
    CloseWindow();
}