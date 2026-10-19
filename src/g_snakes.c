#include "hub.h"

static int snake_at(int p) {
    int s[][2]={{16,6},{47,26},{49,11},{56,53},{62,19},{64,60},{87,24},{93,73},{95,75},{98,78}};
    for (int i=0;i<10;i++) if(s[i][0]==p) return s[i][1]; return p;
}
static int ladder_at(int p) {
    int l[][2]={{1,38},{4,14},{9,31},{21,42},{28,84},{36,44},{51,67},{71,91},{80,100}};
    for (int i=0;i<9;i++) if(l[i][0]==p) return l[i][1]; return p;
}

void game_snakes(Player *p) {
    (void)p;
    int you=0, cpu=0, turn=0; /* 0 you */
    InitWindow(SW, SH, "Snakes & Ladders");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) break;
        if (IsKeyPressed(KEY_SPACE) && you<100 && cpu<100) {
            int d = GetRandomValue(1,6);
            if (turn==0) {
                if (you+d<=100) you+=d;
                you = ladder_at(snake_at(you));
                turn=1;
            } else {
                if (cpu+d<=100) cpu+=d;
                cpu = ladder_at(snake_at(cpu));
                turn=0;
            }
        }
        BeginDrawing();
        ClearBackground((Color){240,240,230,255});
        DrawText("SNAKES & LADDERS  SPACE=roll  ESC=hub", 20, 15, 22, BLACK);
        DrawText(TextFormat("You: %d    CPU: %d    turn: %s", you, cpu, turn?"CPU":"YOU"), 20, 50, 24, DARKBLUE);
        float cell=50, ox=80, oy=100;
        for (int row=9; row>=0; row--) {
            int ltr = (row%2==0);
            for (int col=0; col<10; col++) {
                int n = ltr ? row*10+col+1 : row*10+(9-col)+1;
                int x = (int)(ox+col*cell), y=(int)(oy+(9-row)*cell);
                Color bg = ((row+col)%2)? (Color){200,220,200,255}:(Color){255,255,255,255};
                DrawRectangle(x,y,(int)cell-2,(int)cell-2,bg);
                DrawText(TextFormat("%d",n), x+4, y+4, 12, GRAY);
                if (n==you) DrawCircle(x+(int)cell/2, y+(int)cell/2, 12, GREEN);
                if (n==cpu) DrawCircle(x+(int)cell/2, y+(int)cell/2, 8, ORANGE);
            }
        }
        if (you>=100) DrawText("YOU WIN", 400, 30, 30, GREEN);
        if (cpu>=100) DrawText("CPU WINS", 400, 30, 30, RED);
        EndDrawing();
    }
    CloseWindow();
}
