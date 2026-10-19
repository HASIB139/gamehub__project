#include "hub.h"
#define GW 28
#define GH 18
#define CS 28

void game_snake(Player *p) {
    Vector2 snake[GW*GH];
    int len=3; snake[0]=(Vector2){8,8}; snake[1]=(Vector2){7,8}; snake[2]=(Vector2){6,8};
    Vector2 dir={1,0}, food={14,8};
    int score=0, dead=0; float acc=0;
    int ox=(SW-GW*CS)/2, oy=(SH-GH*CS)/2+20;
    gfx_open("Snake");
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) break;
        if (IsKeyPressed(KEY_R)) { len=3; score=0; dead=0; dir=(Vector2){1,0}; snake[0]=(Vector2){8,8}; }
        if (!dead) {
            if (IsKeyPressed(KEY_RIGHT)&&dir.x==0) dir=(Vector2){1,0};
            if (IsKeyPressed(KEY_LEFT)&&dir.x==0) dir=(Vector2){-1,0};
            if (IsKeyPressed(KEY_DOWN)&&dir.y==0) dir=(Vector2){0,1};
            if (IsKeyPressed(KEY_UP)&&dir.y==0) dir=(Vector2){0,-1};
            acc += GetFrameTime();
            if (acc>0.11f) {
                acc=0;
                for (int i=len;i>0;i--) snake[i]=snake[i-1];
                snake[0].x+=dir.x; snake[0].y+=dir.y;
                if (snake[0].x<0||snake[0].x>=GW||snake[0].y<0||snake[0].y>=GH) dead=1;
                for (int i=1;i<len;i++) if (snake[i].x==snake[0].x&&snake[i].y==snake[0].y) dead=1;
                if (snake[0].x==food.x&&snake[0].y==food.y) {
                    fx_burst(ox + food.x*CS + CS/2.0f, oy + food.y*CS + CS/2.0f, RED, 18);
                    len++; score+=10; food.x=(float)GetRandomValue(0,GW-1); food.y=(float)GetRandomValue(0,GH-1);
                    if (score > p->best_snake) p->best_snake = score;
                    lb_submit("snake", p->name, score, 0);        
                }
            }
        }
        BeginDrawing();
        gfx_background((Color){18, 24, 40, 255}, (Color){8, 10, 20, 255});
gfx_topbar("SNAKE", "arrows move | R restart | ESC hub");
gfx_text_shadow(TextFormat("Score %d    Best %d", score, p->best_snake), ox, 66, 20, RAYWHITE);

/* checkerboard floor */
for (int r = 0; r < GH; r++)
    for (int c = 0; c < GW; c++) {
        Color tile = ((r + c) % 2 == 0) ? (Color){30, 45, 70, 255} : (Color){26, 38, 62, 255};
        DrawRectangle(ox + c*CS, oy + r*CS, CS, CS, tile);
    }
DrawRectangleLinesEx((Rectangle){ ox - 3, oy - 3, GW*CS + 6, GH*CS + 6 }, 3, GOLD);

/* body: rounded segments, bright at the head, darker toward the tail */
for (int i = len - 1; i >= 0; i--) {
    float t = (float)i / len;
    Color col = { (unsigned char)(80 - 50*t), (unsigned char)(230 - 110*t), (unsigned char)(120 - 60*t), 255 };
    Rectangle seg = { ox + snake[i].x*CS + 1, oy + snake[i].y*CS + 1, CS - 2, CS - 2 };
    DrawRectangleRounded(seg, 0.4f, 8, col);
}

/* eyes that look in the direction of travel */
Vector2 h = { ox + snake[0].x*CS + CS/2.0f, oy + snake[0].y*CS + CS/2.0f };
Vector2 side = { -dir.y, dir.x };
for (int e = -1; e <= 1; e += 2) {
    Vector2 eye = { h.x + dir.x*5 + side.x*e*6, h.y + dir.y*5 + side.y*e*6 };
    DrawCircleV(eye, 4, WHITE);
    DrawCircleV((Vector2){ eye.x + dir.x*1.5f, eye.y + dir.y*1.5f }, 2, BLACK);
}

/* pulsing apple */
float pulse = 1.0f + 0.12f * sinf((float)GetTime() * 6.0f);
Vector2 f = { ox + food.x*CS + CS/2.0f, oy + food.y*CS + CS/2.0f };
DrawCircleV(f, CS * 0.7f * pulse, Fade(RED, 0.18f));            /* glow  */
DrawCircleV(f, CS * 0.38f * pulse, (Color){230, 50, 60, 255});  /* apple */
DrawCircleV((Vector2){ f.x - 3, f.y - 4 }, 3, Fade(WHITE, 0.7f)); /* shine */
DrawRectangle((int)f.x - 1, (int)f.y - 14, 3, 6, BROWN);        /* stem  */

fx_update_draw();

if (dead) {
    DrawRectangle(0, 0, SW, SH, Fade(BLACK, 0.6f));
    gfx_panel((Rectangle){ SW/2 - 200, SH/2 - 80, 400, 160 }, (Color){25, 28, 50, 240});
    gfx_text_center("GAME OVER", SW/2, SH/2 - 55, 44, RED);
    gfx_text_center("press R to restart", SW/2, SH/2 + 10, 22, RAYWHITE);
}
        lb_overlay("snake", 0, p->name);
        EndDrawing();
    }
    CloseWindow();
}
