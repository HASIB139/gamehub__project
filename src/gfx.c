#include "hub.h"

void gfx_open(const char *title) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);   
    InitWindow(SW, SH, title);
    SetTargetFPS(60);
}

void gfx_background(Color top, Color bottom) {
    DrawRectangleGradientV(0, 0, SW, SH, top, bottom);
}

void gfx_text_shadow(const char *t, int x, int y, int size, Color c) {
    DrawText(t, x + 2, y + 2, size, Fade(BLACK, 0.5f));
    DrawText(t, x, y, size, c);
}

void gfx_text_center(const char *t, int cx, int y, int size, Color c) {
    gfx_text_shadow(t, cx - MeasureText(t, size) / 2, y, size, c);
}

void gfx_topbar(const char *title, const char *hint) {
    DrawRectangleGradientV(0, 0, SW, 56, (Color){30, 35, 60, 255}, (Color){18, 20, 38, 255});
    DrawRectangle(0, 56, SW, 2, GOLD);
    gfx_text_shadow(title, 20, 12, 30, GOLD);
    DrawText(hint, SW - MeasureText(hint, 18) - 20, 20, 18, LIGHTGRAY);
}

void gfx_panel(Rectangle r, Color c) {
    DrawRectangleRounded((Rectangle){ r.x + 4, r.y + 5, r.width, r.height }, 0.2f, 10, Fade(BLACK, 0.35f));
    DrawRectangleRounded(r, 0.2f, 10, c);
}


#define MAX_P 200
typedef struct { Vector2 pos, vel; float life, size; Color col; } Particle;
static Particle parts[MAX_P];

void fx_burst(float x, float y, Color c, int n) {
    for (int i = 0; i < MAX_P && n > 0; i++) {
        if (parts[i].life > 0) continue;       
        float a = GetRandomValue(0, 359) * DEG2RAD;
        float s = (float)GetRandomValue(40, 220);
        parts[i] = (Particle){ {x, y}, {cosf(a) * s, sinf(a) * s}, 0.6f,
                               (float)GetRandomValue(2, 5), c };
        n--;
    }
}

void fx_update_draw(void) {
    float dt = GetFrameTime();
    for (int i = 0; i < MAX_P; i++) {
        if (parts[i].life <= 0) continue;
        parts[i].life  -= dt;
        parts[i].pos.x += parts[i].vel.x * dt;
        parts[i].pos.y += parts[i].vel.y * dt;
        parts[i].vel.y += 300 * dt;            
        DrawCircleV(parts[i].pos, parts[i].size, Fade(parts[i].col, parts[i].life / 0.6f));
    }
}

int gfx_pick_players(const char *game, const char *p1keys, const char *p2keys) {
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) return 1;
        if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) return 2;

        BeginDrawing();
        gfx_background((Color){ 18, 24, 40, 255 }, (Color){ 8, 10, 20, 255 });
        gfx_topbar(game, "ESC hub");
        gfx_panel((Rectangle){ SW / 2 - 310, 150, 620, 360 }, (Color){ 25, 28, 50, 235 });
        gfx_text_center("HOW MANY PLAYERS?", SW / 2, 180, 40, GOLD);
        gfx_text_center("1   one player", SW / 2, 260, 32, RAYWHITE);
        gfx_text_center("2   two players on this keyboard", SW / 2, 310, 32, RAYWHITE);
        DrawText(TextFormat("Player 1:  %s", p1keys), SW / 2 - 280, 390, 22, (Color){ 255, 150, 40, 255 });
        DrawText(TextFormat("Player 2:  %s", p2keys), SW / 2 - 280, 430, 22, (Color){ 70, 170, 255, 255 });
        gfx_text_center("press 1 or 2", SW / 2, 470, 20, LIGHTGRAY);
        EndDrawing();
    }
    return 0;
}