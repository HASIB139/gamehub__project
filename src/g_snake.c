#include "hub.h"

#define GW 28
#define GH 18
#define CS 28
#define OX ((SW - GW * CS) / 2)
#define OY ((SH - GH * CS) / 2 + 20)

typedef struct {
    int up, down, left, right;
} PlayerInput;

typedef struct {
    Vector2 body[GW * GH];
    int len;
    Vector2 dir, want;
    int score, dead;
} Snake;

typedef struct {
    Snake s[2];
    int players;
    Vector2 food;
    float acc;
} Game;

static const Color BRIGHT[2] = { { 80, 230, 120, 255 }, { 255, 190, 70, 255 } };
static const Color DARK[2]   = { { 30, 120, 60, 255 },  { 200, 90, 30, 255 } };

static void read_player1_input(PlayerInput *in, int players) {
    in->up = IsKeyPressed(KEY_UP);
    in->down = IsKeyPressed(KEY_DOWN);
    in->left = IsKeyPressed(KEY_LEFT);
    in->right = IsKeyPressed(KEY_RIGHT);
    if (players == 1) {
        in->up |= IsKeyPressed(KEY_W);
        in->down |= IsKeyPressed(KEY_S);
        in->left |= IsKeyPressed(KEY_A);
        in->right |= IsKeyPressed(KEY_D);
    }
}

static void read_player2_input(PlayerInput *in) {
    in->up = IsKeyPressed(KEY_W);
    in->down = IsKeyPressed(KEY_S);
    in->left = IsKeyPressed(KEY_A);
    in->right = IsKeyPressed(KEY_D);
}

static int same_cell(Vector2 a, Vector2 b) {
    return (int)a.x == (int)b.x && (int)a.y == (int)b.y;
}

static int hits_body(const Snake *s, Vector2 p, int count) {
    for (int k = 0; k < count; k++)
        if (same_cell(s->body[k], p)) return 1;
    return 0;
}

static int on_any_snake(const Game *g, Vector2 p) {
    for (int i = 0; i < g->players; i++)
        if (hits_body(&g->s[i], p, g->s[i].len)) return 1;
    return 0;
}

static void place_food(Game *g) {
    for (int tries = 0; tries < 500; tries++) {
        Vector2 c = { (float)GetRandomValue(0, GW - 1), (float)GetRandomValue(0, GH - 1) };
        if (!on_any_snake(g, c)) {
            g->food = c;
            return;
        }
    }
}

static void init_snake(Snake *s, float x, float y, float dx) {
    memset(s, 0, sizeof *s);
    s->len = 3;
    for (int i = 0; i < 3; i++) s->body[i] = (Vector2){ x - dx * i, y };
    s->dir = (Vector2){ dx, 0 };
    s->want = s->dir;
}

static void reset_game(Game *g, int players) {
    memset(g, 0, sizeof *g);
    g->players = players;
    init_snake(&g->s[0], 8, (players == 2) ? 5 : 8, 1);
    if (players == 2) init_snake(&g->s[1], 19, 12, -1);
    place_food(g);
}

static int all_dead(const Game *g) {
    for (int i = 0; i < g->players; i++)
        if (!g->s[i].dead) return 0;
    return 1;
}

static void steer(Snake *s, const PlayerInput *in) {
    if (s->dead) return;
    if (in->right && s->dir.x == 0) s->want = (Vector2){ 1, 0 };
    if (in->left && s->dir.x == 0) s->want = (Vector2){ -1, 0 };
    if (in->down && s->dir.y == 0) s->want = (Vector2){ 0, 1 };
    if (in->up && s->dir.y == 0) s->want = (Vector2){ 0, -1 };
}

static void update_players(Game *g, const PlayerInput *in1, const PlayerInput *in2, float dt) {
    steer(&g->s[0], in1);
    if (g->players == 2) steer(&g->s[1], in2);

    g->acc += dt;
    if (g->acc <= 0.11f) return;
    g->acc = 0;

    Vector2 nh[2] = { { 0, 0 }, { 0, 0 } };
    int grow[2] = { 0, 0 }, dies[2] = { 0, 0 };

    for (int i = 0; i < g->players; i++) {
        Snake *s = &g->s[i];
        if (s->dead) continue;
        s->dir = s->want;
        nh[i] = (Vector2){ s->body[0].x + s->dir.x, s->body[0].y + s->dir.y };
        grow[i] = same_cell(nh[i], g->food);
    }

    for (int i = 0; i < g->players; i++) {
        Snake *s = &g->s[i];
        if (s->dead) continue;
        if (nh[i].x < 0 || nh[i].x >= GW || nh[i].y < 0 || nh[i].y >= GH) dies[i] = 1;
        if (hits_body(s, nh[i], s->len - (grow[i] ? 0 : 1))) dies[i] = 1;
        for (int j = 0; j < g->players; j++) {
            if (j == i || g->s[j].dead) continue;
            if (same_cell(nh[i], nh[j])) dies[i] = 1;
            if (hits_body(&g->s[j], nh[i], g->s[j].len - (grow[j] ? 0 : 1))) dies[i] = 1;
        }
    }

    int ate = 0;
    for (int i = 0; i < g->players; i++) {
        Snake *s = &g->s[i];
        if (s->dead) continue;
        if (dies[i]) {
            s->dead = 1;
            fx_burst(OX + s->body[0].x * CS + CS / 2.0f, OY + s->body[0].y * CS + CS / 2.0f, ORANGE, 30);
            continue;
        }
        int grown = grow[i] && s->len < GW * GH;
        int newlen = s->len + (grown ? 1 : 0);
        for (int k = newlen - 1; k > 0; k--) s->body[k] = s->body[k - 1];
        s->body[0] = nh[i];
        s->len = newlen;
        if (grow[i]) {
            s->score += 10;
            ate = 1;
            fx_burst(OX + nh[i].x * CS + CS / 2.0f, OY + nh[i].y * CS + CS / 2.0f, RED, 18);
        }
    }
    if (ate) place_food(g);
}

static Color mix(Color a, Color b, float t) {
    return (Color){ (unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
                    (unsigned char)(a.b + (b.b - a.b) * t), 255 };
}

static void draw_snake(const Snake *s, int who) {
    for (int i = s->len - 1; i >= 0; i--) {
        Color col = mix(BRIGHT[who], DARK[who], (float)i / (float)s->len);
        if (s->dead) col = Fade(col, 0.3f);
        Rectangle seg = { OX + s->body[i].x * CS + 1, OY + s->body[i].y * CS + 1, CS - 2, CS - 2 };
        DrawRectangleRounded(seg, 0.4f, 8, col);
    }
    if (s->dead) return;

    Vector2 h = { OX + s->body[0].x * CS + CS / 2.0f, OY + s->body[0].y * CS + CS / 2.0f };
    Vector2 side = { -s->dir.y, s->dir.x };
    for (int e = -1; e <= 1; e += 2) {
        Vector2 eye = { h.x + s->dir.x * 5 + side.x * e * 6, h.y + s->dir.y * 5 + side.y * e * 6 };
        DrawCircleV(eye, 4, WHITE);
        DrawCircleV((Vector2){ eye.x + s->dir.x * 1.5f, eye.y + s->dir.y * 1.5f }, 2, BLACK);
    }
}

static void draw_food(Vector2 food) {
    float pulse = 1.0f + 0.12f * sinf((float)GetTime() * 6.0f);
    Vector2 f = { OX + food.x * CS + CS / 2.0f, OY + food.y * CS + CS / 2.0f };
    DrawCircleV(f, CS * 0.7f * pulse, Fade(RED, 0.18f));
    DrawCircleV(f, CS * 0.38f * pulse, (Color){ 230, 50, 60, 255 });
    DrawCircleV((Vector2){ f.x - 3, f.y - 4 }, 3, Fade(WHITE, 0.7f));
    DrawRectangle((int)f.x - 1, (int)f.y - 14, 3, 6, BROWN);
}

static void draw_board(void) {
    for (int r = 0; r < GH; r++)
        for (int c = 0; c < GW; c++) {
            Color tile = ((r + c) % 2 == 0) ? (Color){ 30, 45, 70, 255 } : (Color){ 26, 38, 62, 255 };
            DrawRectangle(OX + c * CS, OY + r * CS, CS, CS, tile);
        }
    DrawRectangleLinesEx((Rectangle){ OX - 3, OY - 3, GW * CS + 6, GH * CS + 6 }, 3, GOLD);
}

static void draw_scores(const Game *g, int best) {
    if (g->players == 1) {
        gfx_text_shadow(TextFormat("Score %d    Best %d", g->s[0].score, best), OX, 66, 20, RAYWHITE);
        return;
    }
    const char *a = TextFormat("P1  %d%s", g->s[0].score, g->s[0].dead ? "  OUT" : "");
    gfx_text_shadow(a, OX, 66, 22, BRIGHT[0]);
    const char *b = TextFormat("P2  %d%s", g->s[1].score, g->s[1].dead ? "  OUT" : "");
    gfx_text_shadow(b, OX + GW * CS - MeasureText(b, 22), 66, 22, BRIGHT[1]);
}

static void draw_game_over(const Game *g) {
    DrawRectangle(0, 0, SW, SH, Fade(BLACK, 0.6f));
    gfx_panel((Rectangle){ SW / 2 - 220, SH / 2 - 95, 440, 190 }, (Color){ 25, 28, 50, 240 });
    if (g->players == 1) {
        gfx_text_center("GAME OVER", SW / 2, SH / 2 - 70, 44, RED);
        gfx_text_center("R restart    M mode", SW / 2, SH / 2 + 10, 22, RAYWHITE);
        return;
    }
    int a = g->s[0].score, b = g->s[1].score;
    const char *msg = (a > b) ? "PLAYER 1 WINS" : (b > a) ? "PLAYER 2 WINS" : "IT'S A DRAW";
    Color col = (a > b) ? BRIGHT[0] : (b > a) ? BRIGHT[1] : RAYWHITE;
    gfx_text_center(msg, SW / 2, SH / 2 - 75, 40, col);
    gfx_text_center(TextFormat("%d  -  %d", a, b), SW / 2, SH / 2 - 15, 34, RAYWHITE);
    gfx_text_center("R restart    M mode", SW / 2, SH / 2 + 40, 22, LIGHTGRAY);
}

void game_snake(Player *p) {
    gfx_open("Snake");
    int players = gfx_pick_players("SNAKE", "arrow keys", "W A S D");
    if (!players) {
        CloseWindow();
        return;
    }

    Game g;
    reset_game(&g, players);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) break;
        if (IsKeyPressed(KEY_R)) reset_game(&g, players);
        if (IsKeyPressed(KEY_M)) {
            int m = gfx_pick_players("SNAKE", "arrow keys", "W A S D");
            if (!m) break;
            players = m;
            reset_game(&g, players);
        }

        PlayerInput in1, in2;
        read_player1_input(&in1, players);
        read_player2_input(&in2);

        int before = g.s[0].score;
        if (!all_dead(&g)) update_players(&g, &in1, &in2, GetFrameTime());
        if (players == 1 && g.s[0].score != before) {
            if (g.s[0].score > p->best_snake) p->best_snake = g.s[0].score;
            lb_submit("snake", p->name, g.s[0].score, 0);
        }

        BeginDrawing();
        gfx_background((Color){ 18, 24, 40, 255 }, (Color){ 8, 10, 20, 255 });
        gfx_topbar("SNAKE", players == 1 ? "arrows or WASD | R restart | M mode | TAB top 5 | ESC hub"
                                         : "P1 arrows | P2 W A S D | R restart | M mode | ESC hub");
        draw_scores(&g, p->best_snake);
        draw_board();
        draw_snake(&g.s[0], 0);
        if (players == 2) draw_snake(&g.s[1], 1);
        draw_food(g.food);
        fx_update_draw();
        if (all_dead(&g)) draw_game_over(&g);
        lb_overlay("snake", 0, p->name);
        EndDrawing();
    }
    CloseWindow();
}