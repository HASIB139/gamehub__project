#include "hub.h"

#define GRAV      1700.0f
#define JUMP_V     640.0f
#define RUN_X      220.0f
#define RUN_W       28.0f
#define STAND_H     44.0f
#define SLIDE_H     24.0f
#define SLIDE_TIME   0.6f
#define MAXP        12
#define MAXD        60

#define DASH_TIME    3.0f
#define SLOW_TIME    6.0f
#define WINGS_TIME  10.0f
#define X2_TIME     10.0f
#define GEM_METERS  50

enum { PU_NONE = 0, PU_SHIELD, PU_DASH, PU_SLOW, PU_WINGS, PU_X2, PU_GEM };

typedef struct {
    float x, y, w;
    int   obs;
    float off;
    int   seed;
    int   pu;
    float pu_off;
    float pu_y;
} Plat;

typedef struct { float x, y, vx, vy, life; } Dust;

typedef struct {
    Plat  pl[MAXP];
    Dust  dust[MAXD];
    float feet, vy;
    float dist, speed;
    float score_px;
    float slide_t;
    float shake, runtime;
    int   onground, jumps, dead, saved;

    int   shield;
    float dash_t, slow_t, wings_t, x2_t;
    float invuln_t;
    int   popup;
    float popup_t;
} Run;

static Color pu_color(int t) {
    switch (t) {
    case PU_SHIELD: return (Color){  80, 170, 255, 255 };
    case PU_DASH:   return (Color){ 255, 140,  40, 255 };
    case PU_SLOW:   return (Color){ 170, 110, 240, 255 };
    case PU_WINGS:  return (Color){ 120, 230, 140, 255 };
    case PU_X2:     return (Color){ 255, 205,  60, 255 };
    default:        return (Color){ 255, 110, 190, 255 };
    }
}

static const char *pu_name(int t) {
    switch (t) {
    case PU_SHIELD: return "SHIELD";
    case PU_DASH:   return "DASH";
    case PU_SLOW:   return "SLOW-MO";
    case PU_WINGS:  return "WINGS";
    case PU_X2:     return "DOUBLE SCORE";
    default:        return "GEM +50 m";
    }
}

static void tri2(Vector2 a, Vector2 b, Vector2 c, Color col) {
    DrawTriangle(a, b, c, col);
    DrawTriangle(a, c, b, col);
}

static void draw_pickup_icon(int t, float cx, float cy, float r) {
    Color c = pu_color(t);
    Vector2 p = { cx, cy };
    DrawCircleV(p, r * 1.5f, Fade(c, 0.25f));
    DrawCircleV(p, r, ColorBrightness(c, -0.3f));
    DrawCircleV(p, r * 0.85f, c);

    switch (t) {
    case PU_SHIELD:
        DrawCircleLines((int)cx, (int)cy, r * 0.5f, WHITE);
        DrawCircleLines((int)cx, (int)cy, r * 0.5f + 1.0f, WHITE);
        break;
    case PU_DASH:
        for (int k = 0; k < 2; k++) {
            float x = cx - r * 0.4f + k * r * 0.45f;
            DrawLineEx((Vector2){ x, cy - r * 0.4f }, (Vector2){ x + r * 0.35f, cy }, 2, WHITE);
            DrawLineEx((Vector2){ x + r * 0.35f, cy }, (Vector2){ x, cy + r * 0.4f }, 2, WHITE);
        }
        break;
    case PU_SLOW:
        DrawCircleLines((int)cx, (int)cy, r * 0.55f, WHITE);
        DrawLineEx(p, (Vector2){ cx, cy - r * 0.4f }, 2, WHITE);
        DrawLineEx(p, (Vector2){ cx + r * 0.3f, cy }, 2, WHITE);
        break;
    case PU_WINGS:
        tri2((Vector2){ cx - r * 0.1f, cy + r * 0.1f }, (Vector2){ cx - r * 0.65f, cy - r * 0.45f },
             (Vector2){ cx - r * 0.5f, cy + r * 0.3f }, WHITE);
        tri2((Vector2){ cx + r * 0.1f, cy + r * 0.1f }, (Vector2){ cx + r * 0.65f, cy - r * 0.45f },
             (Vector2){ cx + r * 0.5f, cy + r * 0.3f }, WHITE);
        break;
    case PU_X2: {
        int fs = (int)(r * 0.95f);
        if (fs < 10) fs = 10;
        DrawText("x2", (int)(cx - MeasureText("x2", fs) / 2.0f), (int)(cy - fs / 2.0f), fs, WHITE);
        break;
    }
    default:
        DrawPoly(p, 4, r * 0.55f, 0, WHITE);
        break;
    }
}

static void spawn(Plat *pl, const Plat *prev, float speed, int allow_obs) {
    float gap_max = speed * 0.45f;
    if (gap_max < 100.0f) gap_max = 100.0f;
    float gap = (float)GetRandomValue(70, (int)gap_max);

    pl->x = prev->x + prev->w + gap;
    pl->w = (float)GetRandomValue(170, 420);

    int max_rise = (gap > speed * 0.30f) ? 20 : 80;
    float y = prev->y + (float)GetRandomValue(-max_rise, 90);
    float lo = SH * 0.42f, hi = SH - 90.0f;
    if (y < lo) y = lo;
    if (y > hi) y = hi;
    pl->y = y;

    pl->obs = 0;
    pl->off = 0;
    pl->seed = GetRandomValue(0, 999);
    pl->pu = PU_NONE;
    pl->pu_off = 0;
    pl->pu_y = 0;

    if (allow_obs) {
        int roll = GetRandomValue(0, 99);
        if (roll < 55) {
            pl->obs = (roll < 35) ? 1 : 2;

            float vs = fminf(speed + 150.0f, 640.0f);
            float land = 0.75f * vs - gap;
            if (land < 0) land = 0;
            float min_off = fmaxf(120.0f, land + 120.0f);
            float need_w = min_off + 56.0f + (0.25f * vs + 60.0f);
            if (pl->w < need_w) pl->w = need_w + (float)GetRandomValue(0, 60);
            pl->off = min_off + (float)GetRandomValue(0, (int)(pl->w - need_w));
        }

        if (GetRandomValue(0, 99) < 20) {
            float from = 40.0f, to = pl->w - 60.0f;
            if (pl->obs == 2) from = pl->off + 56.0f + 70.0f;
            if (to > from) {
                int r = GetRandomValue(0, 99);
                pl->pu = (r < 22) ? PU_SHIELD : (r < 34) ? PU_DASH : (r < 52) ? PU_SLOW :
                         (r < 72) ? PU_WINGS : (r < 90) ? PU_X2 : PU_GEM;
                pl->pu_off = from + (float)GetRandomValue(0, (int)(to - from));
                pl->pu_y = (float)GetRandomValue(70, 110);
            }
        }
    }
}

static void reset_run(Run *g) {
    memset(g, 0, sizeof *g);
    g->pl[0] = (Plat){ -60, SH * 0.68f, 560, 0, 0, 7, PU_NONE, 0, 0 };
    g->speed = 300.0f;
    for (int i = 1; i < MAXP; i++)
        spawn(&g->pl[i], &g->pl[i - 1], g->speed, i > 2);
    g->feet = g->pl[0].y;
    g->onground = 1;
}

static void add_dust(Run *g, float x, float y, int n) {
    for (int i = 0; i < MAXD && n > 0; i++) {
        if (g->dust[i].life > 0) continue;
        g->dust[i] = (Dust){ x, y, -g->speed * 0.5f + (float)GetRandomValue(-60, 40),
                             (float)GetRandomValue(-120, -20), 0.4f };
        n--;
    }
}

static void update_dust(Run *g, float dt) {
    for (int i = 0; i < MAXD; i++) {
        Dust *d = &g->dust[i];
        if (d->life <= 0) continue;
        d->life -= dt;
        d->x += d->vx * dt;
        d->y += d->vy * dt;
        d->vy += 400.0f * dt;
    }
}

static void die(Run *g) {
    if (g->dead) return;
    g->dead = 1;
    g->shake = 14.0f;
    fx_burst(RUN_X + RUN_W / 2, fminf(g->feet - 20.0f, SH - 40.0f), ORANGE, 40);
}

static int absorb_hit(Run *g) {
    if (g->dash_t > 0 || g->invuln_t > 0) return 1;
    if (g->shield) {
        g->shield = 0;
        g->invuln_t = 1.2f;
        g->shake = 8.0f;
        fx_burst(RUN_X + RUN_W / 2, g->feet - 20.0f, SKYBLUE, 25);
        return 1;
    }
    return 0;
}

static void grab(Run *g, int type) {
    switch (type) {
    case PU_SHIELD: g->shield = 1;               break;
    case PU_DASH:   g->dash_t = DASH_TIME;       break;
    case PU_SLOW:   g->slow_t = SLOW_TIME;       break;
    case PU_WINGS:  g->wings_t = WINGS_TIME;     break;
    case PU_X2:     g->x2_t = X2_TIME;           break;
    default:        g->score_px += GEM_METERS * 40.0f; break;
    }
    g->popup = type;
    g->popup_t = 1.0f;
    fx_burst(RUN_X + RUN_W / 2, g->feet - 30.0f, pu_color(type), 18);
}

static void update_effects(Run *g, float dt) {
    if (g->dash_t > 0 && g->dash_t - dt <= 0) g->invuln_t = 0.8f;
    float *timers[] = { &g->dash_t, &g->slow_t, &g->wings_t, &g->x2_t, &g->invuln_t, &g->popup_t };
    for (int i = 0; i < 6; i++)
        if (*timers[i] > 0) {
            *timers[i] -= dt;
            if (*timers[i] < 0) *timers[i] = 0;
        }
}

static void update_run(Run *g, float dt) {

    g->speed = 300.0f + g->dist * 0.015f;
    if (g->speed > 640.0f) g->speed = 640.0f;
    float move = g->speed * ((g->dash_t > 0) ? 1.5f : 1.0f) * dt;
    g->dist += move;
    g->score_px += move * ((g->x2_t > 0) ? 2.0f : 1.0f);
    g->runtime += dt;

    for (int i = 0; i < MAXP; i++) g->pl[i].x -= move;
    if (g->pl[0].x + g->pl[0].w < -80) {
        for (int i = 0; i < MAXP - 1; i++) g->pl[i] = g->pl[i + 1];
        spawn(&g->pl[MAXP - 1], &g->pl[MAXP - 2], g->speed, 1);
    }

    int max_jumps = (g->wings_t > 0) ? 3 : 2;
    if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) && g->jumps < max_jumps) {
        g->vy = -JUMP_V;
        g->jumps++;
        g->onground = 0;
        g->slide_t = 0;
        add_dust(g, RUN_X + RUN_W / 2, g->feet, 6);
    }
    if ((IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) && g->onground) g->slide_t = SLIDE_TIME;
    if (g->slide_t > 0) g->slide_t -= dt;

    int was_on = g->onground;
    float prev_feet = g->feet;
    g->vy += GRAV * dt;
    float hit_v = g->vy;
    g->feet += g->vy * dt;

    int landed = 0;
    for (int i = 0; i < MAXP; i++) {
        Plat *q = &g->pl[i];
        if (!(RUN_X + RUN_W > q->x && RUN_X < q->x + q->w)) continue;
        if (g->vy >= 0 && prev_feet <= q->y + 12.0f && g->feet >= q->y) {
            g->feet = q->y;
            g->vy = 0;
            landed = 1;
        } else if (g->feet > q->y + 16.0f) {
            if (absorb_hit(g)) {
                g->feet = q->y;
                g->vy = 0;
                landed = 1;
            } else {
                die(g);
            }
        }
    }

    if (landed && !was_on) {
        add_dust(g, RUN_X + RUN_W / 2, g->feet, 7);
        if (hit_v > 600.0f) g->shake = 3.0f;
    }
    if (landed && (int)(g->runtime * 12.0f) != (int)((g->runtime - dt) * 12.0f))
        add_dust(g, RUN_X + 4, g->feet, 1);

    g->onground = landed;
    if (landed) g->jumps = 0;
    else if (was_on) g->jumps = 1;
    if (!landed) g->slide_t = 0;

    if (g->feet > SH + 80) die(g);

    float h = (g->slide_t > 0) ? SLIDE_H : STAND_H;

    Rectangle body = { RUN_X, g->feet - h, RUN_W, h };
    for (int i = 0; i < MAXP; i++) {
        Plat *q = &g->pl[i];
        if (q->pu == PU_NONE) continue;
        Vector2 c = { q->x + q->pu_off, q->y - q->pu_y };
        if (CheckCollisionCircleRec(c, 18.0f, body)) {
            grab(g, q->pu);
            q->pu = PU_NONE;
        }
    }

    Rectangle me = { RUN_X + 5, g->feet - h + 3, RUN_W - 10, h - 3 };
    for (int i = 0; i < MAXP; i++) {
        Plat *q = &g->pl[i];
        Rectangle r;
        if (q->obs == 1)      r = (Rectangle){ q->x + q->off, q->y - 40, 36, 40 };
        else if (q->obs == 2) r = (Rectangle){ q->x + q->off, 58, 56, q->y - 30 - 58 };
        else continue;
        if (CheckCollisionRecs(me, r)) {
            if (absorb_hit(g)) {
                fx_burst(r.x + r.width / 2, r.y + r.height / 2, ORANGE, 12);
                q->obs = 0;
            } else {
                die(g);
            }
        }
    }
}

static void draw_city(float dist, float factor, int bw, int base_h, Color col, int windows) {
    float off = fmodf(dist * factor, (float)bw);
    int first = (int)(dist * factor / (float)bw);
    for (int i = 0; i * bw - off < SW + bw; i++) {
        int idx = first + i;
        int h = base_h + ((idx * 37) % 5) * 28;
        int x = (int)(i * bw - off);
        DrawRectangle(x, SH - h, bw - 6, h, col);
        if (windows)
            for (int row = 0; row < h / 30; row++)
                for (int k = 0; k < 3; k++)
                    if ((idx * 31 + row * 17 + k * 13) % 5 == 0)
                        DrawRectangle(x + 10 + k * 22, SH - h + 12 + row * 30, 10, 14, Fade(GOLD, 0.5f));
    }
}

static void draw_runner(const Run *g) {
    float t = g->runtime;
    Color shirt = { 255, 120, 60, 255 }, skin = { 245, 205, 170, 255 };
    Color pants = { 40, 44, 70, 255 }, scarf = { 230, 50, 60, 255 };

    if (g->onground)
        DrawEllipse((int)(RUN_X + RUN_W / 2), (int)g->feet + 1, 16, 4, Fade(BLACK, 0.3f));

    if (g->slide_t > 0) {

        float y = g->feet;
        DrawLineEx((Vector2){ RUN_X + 22, y - 9 }, (Vector2){ RUN_X + 44, y - 5 }, 6, pants);
        DrawRectangleRounded((Rectangle){ RUN_X - 4, y - 18, 30, 14 }, 0.5f, 6, shirt);
        DrawCircleV((Vector2){ RUN_X - 4, y - 15 }, 8, skin);
        DrawLineEx((Vector2){ RUN_X - 8, y - 18 }, (Vector2){ RUN_X - 28, y - 18 + sinf(t * 14.0f) * 4.0f }, 4, scarf);
        return;
    }

    float top = g->feet - STAND_H;
    float hx = RUN_X + RUN_W / 2;
    float hip = top + 34;
    float a = g->onground ? sinf(t * 20.0f) * 0.9f : 0.7f;
    float b = g->onground ? -a : -0.4f;

    if (g->wings_t > 0) {
        float flap = sinf(t * 14.0f) * 8.0f;
        Color wing = Fade(pu_color(PU_WINGS), 0.9f);
        tri2((Vector2){ hx - 4, top + 20 }, (Vector2){ hx - 30, top + 4 + flap }, (Vector2){ hx - 22, top + 28 }, wing);
        tri2((Vector2){ hx - 4, top + 22 }, (Vector2){ hx - 24, top + 14 + flap }, (Vector2){ hx - 16, top + 34 }, wing);
    }

    DrawLineEx((Vector2){ hx, hip }, (Vector2){ hx + sinf(b) * 11, hip + cosf(b) * 11 }, 5, pants);
    DrawLineEx((Vector2){ hx, top + 21 }, (Vector2){ hx + sinf(a) * 10, top + 29 }, 4, skin);

    DrawRectangleRounded((Rectangle){ hx - 9, top + 17, 18, 18 }, 0.4f, 6, shirt);
    DrawCircleV((Vector2){ hx, top + 9 }, 9, skin);
    DrawLineEx((Vector2){ hx - 4, top + 15 }, (Vector2){ hx - 26, top + 15 + sinf(t * 14.0f) * 5.0f }, 4, scarf);

    DrawLineEx((Vector2){ hx, hip }, (Vector2){ hx + sinf(a) * 11, hip + cosf(a) * 11 }, 5, pants);
    DrawLineEx((Vector2){ hx, top + 21 }, (Vector2){ hx + sinf(b) * 10, top + 29 }, 4, skin);
}

static void draw_world(const Run *g) {

    for (int i = 0; i < MAXP; i++) {
        const Plat *q = &g->pl[i];
        if (q->x > SW + 40 || q->x + q->w < -40) continue;
        int x = (int)q->x, y = (int)q->y, w = (int)q->w, h = SH - y + 40;

        DrawRectangleGradientV(x, y, w, h, (Color){ 78, 84, 118, 255 }, (Color){ 32, 34, 60, 255 });
        DrawRectangle(x, y, w, 10, (Color){ 150, 158, 192, 255 });
        DrawRectangle(x, y + 10, w, 3, Fade(BLACK, 0.25f));

        for (int k = 0; 20 + k * 40 < w - 24; k++)
            for (int r = 0; y + 36 + r * 46 < SH; r++) {
                int lit = ((q->seed + k * 7 + r * 3) % 4 == 0);
                DrawRectangle(x + 20 + k * 40, y + 36 + r * 46, 14, 22,
                              lit ? Fade(GOLD, 0.55f) : Fade(BLACK, 0.18f));
            }

        if (q->obs == 1) {
            Rectangle r = { q->x + q->off, q->y - 40, 36, 40 };
            Color dark = { 95, 60, 30, 255 };
            DrawRectangleRec(r, (Color){ 160, 105, 55, 255 });
            DrawRectangleLinesEx(r, 3, dark);
            DrawLineEx((Vector2){ r.x + 3, r.y + 3 }, (Vector2){ r.x + r.width - 3, r.y + r.height - 3 }, 3, dark);
            DrawLineEx((Vector2){ r.x + r.width - 3, r.y + 3 }, (Vector2){ r.x + 3, r.y + r.height - 3 }, 3, dark);
        } else if (q->obs == 2) {
            float bx = q->x + q->off, bottom = q->y - 30;
            DrawRectangle((int)bx, 58, 56, (int)(bottom - 58), (Color){ 60, 62, 80, 255 });
            DrawRectangle((int)bx, (int)bottom - 30, 56, 30, (Color){ 250, 200, 40, 255 });
            for (int s = 0; s < 4; s++)
                DrawRectangle((int)bx + 6 + s * 14, (int)bottom - 30, 7, 30, (Color){ 30, 30, 30, 255 });
        }

        if (q->pu != PU_NONE) {
            float bob = sinf(g->runtime * 4.0f + (float)q->seed) * 4.0f;
            draw_pickup_icon(q->pu, q->x + q->pu_off, q->y - q->pu_y + bob, 15.0f);
        }
    }

    int lines = (int)((g->speed - 300.0f) / 45.0f) + ((g->dash_t > 0) ? 6 : 0);
    for (int i = 0; i < lines && i < 14; i++) {
        float y = 90.0f + (float)((i * 97) % (SH - 200));
        float x = SW - fmodf(g->dist * 2.0f + i * 211.0f, SW + 200.0f);
        DrawLineEx((Vector2){ x, y }, (Vector2){ x + 50.0f + g->speed * 0.06f, y }, 2, Fade(WHITE, 0.15f));
    }

    if (!g->dead) {

        if (g->dash_t > 0)
            for (int i = 0; i < 6; i++) {
                float len = 40.0f + (float)((i * 37) % 50);
                float y = g->feet - 8.0f - i * 7.0f;
                float x = RUN_X - 6.0f - fmodf(g->runtime * 600.0f + i * 53.0f, 60.0f);
                DrawLineEx((Vector2){ x, y }, (Vector2){ x - len, y }, 3, Fade(pu_color(PU_DASH), 0.55f));
            }

        if (!(g->invuln_t > 0 && ((int)(g->runtime * 20.0f) & 1)))
            draw_runner(g);

        if (g->shield) {
            Vector2 c = { RUN_X + RUN_W / 2, g->feet - 24 };
            float pulse = 1.0f + 0.05f * sinf(g->runtime * 8.0f);
            DrawCircleV(c, 34.0f * pulse, Fade(pu_color(PU_SHIELD), 0.22f));
            DrawCircleLines((int)c.x, (int)c.y, 34.0f * pulse, Fade(WHITE, 0.8f));
        }
    }

    for (int i = 0; i < MAXD; i++)
        if (g->dust[i].life > 0)
            DrawCircleV((Vector2){ g->dust[i].x, g->dust[i].y }, 2.0f + g->dust[i].life * 4.0f,
                        Fade(LIGHTGRAY, g->dust[i].life / 0.4f));

    fx_update_draw();
}

static void draw_effects_hud(const Run *g) {
    int types[5]  = { PU_SHIELD, PU_DASH, PU_SLOW, PU_WINGS, PU_X2 };
    float left[5] = { g->shield ? 1.0f : 0.0f, g->dash_t / DASH_TIME, g->slow_t / SLOW_TIME,
                      g->wings_t / WINGS_TIME, g->x2_t / X2_TIME };
    int y = 112;
    for (int i = 0; i < 5; i++) {
        if (left[i] <= 0) continue;
        draw_pickup_icon(types[i], 34, (float)y, 12.0f);
        DrawRectangle(58, y - 4, 90, 8, Fade(BLACK, 0.45f));
        DrawRectangle(58, y - 4, (int)(90.0f * left[i]), 8, pu_color(types[i]));
        y += 34;
    }
}

void game_parkour(Player *p) {
    static int best = 0;
    Run g;
    int newbest = 0;

    reset_run(&g);
    gfx_open("Parkour");

    Camera2D cam;
    cam.offset = (Vector2){ SW / 2.0f, SH / 2.0f };
    cam.rotation = 0.0f;
    cam.zoom = 1.0f;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) break;
        float dt = GetFrameTime();
        if (dt > 0.033f) dt = 0.033f;

        if (IsKeyPressed(KEY_R)) { reset_run(&g); newbest = 0; }
        if (!g.dead) {
            update_effects(&g, dt);
            float world_dt = (g.slow_t > 0) ? dt * 0.6f : dt;
            update_run(&g, world_dt);
        }
        update_dust(&g, dt);
        if (g.shake > 0) { g.shake -= 40.0f * dt; if (g.shake < 0) g.shake = 0; }

        int meters = (int)(g.score_px / 40.0f);
        if (g.dead && !g.saved) {
            g.saved = 1;
            lb_submit("parkour", p->name, meters, 0);
            if (meters > best) { best = meters; newbest = 1; }
        }

        BeginDrawing();

        gfx_background((Color){ 30, 24, 70, 255 }, (Color){ 235, 120, 90, 255 });
        DrawCircle((int)(SW * 0.75f), (int)(SH * 0.55f), 70, Fade((Color){ 255, 205, 130, 255 }, 0.9f));
        draw_city(g.dist, 0.15f, 110, 150, (Color){ 52, 40, 92, 255 }, 0);
        draw_city(g.dist, 0.35f, 80, 90, (Color){ 36, 30, 72, 255 }, 1);

        cam.target = (Vector2){ SW / 2.0f + (float)GetRandomValue(-100, 100) / 100.0f * g.shake,
                                SH / 2.0f + (float)GetRandomValue(-100, 100) / 100.0f * g.shake };
        BeginMode2D(cam);
        draw_world(&g);
        EndMode2D();

        if (g.slow_t > 0 && !g.dead)
            DrawRectangle(0, 0, SW, SH, Fade(pu_color(PU_SLOW), 0.14f));

        gfx_topbar("PARKOUR", "SPACE jump (x2) | DOWN slide | R restart | TAB top 5 | ESC hub");
        gfx_text_center(TextFormat("%d m", meters), SW / 2, 66, 40, RAYWHITE);
        if (g.x2_t > 0) DrawText("x2", SW / 2 + 70, 72, 28, pu_color(PU_X2));
        DrawText(TextFormat("Best %d m", best), 20, 70, 20, LIGHTGRAY);
        DrawText(TextFormat("Speed %d", (int)g.speed), SW - 140, 70, 20, LIGHTGRAY);
        if (!g.dead) draw_effects_hud(&g);

        if (g.popup_t > 0 && !g.dead) {
            const char *nm = pu_name(g.popup);
            int w = MeasureText(nm, 26);
            DrawText(nm, (int)(RUN_X + RUN_W / 2 - w / 2.0f), (int)(g.feet - 90.0f - (1.0f - g.popup_t) * 34.0f),
                     26, Fade(pu_color(g.popup), g.popup_t));
        }

        if (g.runtime < 6.0f && !g.dead) {
            gfx_text_center("SPACE = jump (again in the air = double jump)    DOWN = slide", SW / 2, SH - 78, 22, RAYWHITE);
            gfx_text_center("Grab the glowing orbs for powerups!", SW / 2, SH - 46, 22, GOLD);
        }

        if (g.dead) {
            DrawRectangle(0, 0, SW, SH, Fade(BLACK, 0.55f));
            gfx_panel((Rectangle){ SW / 2 - 210, SH / 2 - 100, 420, 200 }, (Color){ 25, 28, 50, 245 });
            gfx_text_center("CRASHED!", SW / 2, SH / 2 - 78, 44, RED);
            gfx_text_center(TextFormat("You ran %d m", meters), SW / 2, SH / 2 - 20, 30, RAYWHITE);
            if (newbest) gfx_text_center("NEW BEST!", SW / 2, SH / 2 + 22, 24, GOLD);
            gfx_text_center("press R to run again", SW / 2, SH / 2 + 60, 20, LIGHTGRAY);
        }

        lb_overlay("parkour", 0, p->name);
        EndDrawing();
    }
    CloseWindow();
}