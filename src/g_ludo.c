#include "hub.h"
#define YARD -1
#define HOME 30
#define NT 2

void game_ludo(Player *p) {
    (void)p;
    int you[NT]={YARD,YARD}, cpu[NT]={YARD,YARD};
    int turn=0, dice=1, msg=0;
    InitWindow(SW, SH, "Ludo");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) break;
        if (IsKeyPressed(KEY_SPACE) && !msg) {
            dice = GetRandomValue(1,6);
            int *t = turn? cpu : you;
            int *o = turn? you : cpu;
            int moved=0;
            for (int i=0;i<NT && !moved;i++) {
                if (t[i]==YARD && dice==6) { t[i]=0; moved=1; }
                else if (t[i]>=0 && t[i]<HOME && t[i]+dice<=HOME) {
                    t[i]+=dice;
                    if (t[i]==HOME) moved=1;
                    else {
                        for (int j=0;j<NT;j++) if (o[j]==t[i]) o[j]=YARD;
                        moved=1;
                    }
                }
            }
            if (you[0]>=HOME && you[1]>=HOME) msg=1;
            if (cpu[0]>=HOME && cpu[1]>=HOME) msg=2;
            if (dice!=6) turn = 1-turn;
        }
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("LUDO  SPACE=roll  ESC=hub", 20, 20, 22, BLACK);
        DrawText(TextFormat("Dice: %d   turn: %s", dice, turn?"CPU":"YOU"), 20, 55, 24, DARKGRAY);
        /* yards */
        DrawRectangle(40, 100, 160, 160, GREEN);
        DrawRectangle(SW-200, 100, 160, 160, RED);
        DrawRectangle(40, SH-200, 160, 160, GOLD);
        DrawRectangle(SW-200, SH-200, 160, 160, BLUE);
        DrawText("YOU", 90, 160, 28, WHITE);
        DrawText("CPU", SW-160, 160, 28, WHITE);
        for (int i=0;i<NT;i++) {
            DrawText(TextFormat("Y%d:%s", i+1, you[i]==YARD?"yard": you[i]>=HOME?"HOME":TextFormat("%d",you[i])), 40, 280+i*28, 22, DARKGREEN);
            DrawText(TextFormat("C%d:%s", i+1, cpu[i]==YARD?"yard": cpu[i]>=HOME?"HOME":TextFormat("%d",cpu[i])), SW-200, 280+i*28, 22, MAROON);
        }
        if (msg==1) DrawText("YOU WIN LUDO", 350, 320, 36, GREEN);
        if (msg==2) DrawText("CPU WINS", 380, 320, 36, RED);
        EndDrawing();
    }
    CloseWindow();
}
