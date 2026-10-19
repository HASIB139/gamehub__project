#include "hub.h"

static const char *WORDS[] = {
    "APPLE","BRAVE","CLOUD","DREAM","EARTH","FLAME","GRAPE","HOUSE","INPUT","JOKER"
};

void game_word(Player *p) {
    (void)p;
    const char *secret = WORDS[GetRandomValue(0,9)];
    char grid[6][6];
    int marks[6][5]; /* 0 miss 1 yellow 2 green */
    int row=0, col=0, done=0, won=0;
    memset(grid,0,sizeof grid);
    memset(marks,0,sizeof marks);

    InitWindow(SW, SH, "Word");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) break;
        if (!done) {
            int ch = GetCharPressed();
            while (ch>0) {
                if (col<5 && ch>='a'&&ch<='z') { grid[row][col++]=(char)(ch-32); }
                if (col<5 && ch>='A'&&ch<='Z') { grid[row][col++]=(char)ch; }
                ch = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && col>0) grid[row][--col]=0;
            if (IsKeyPressed(KEY_ENTER) && col==5) {
                int used[5]={0};
                for (int i=0;i<5;i++) {
                    if (grid[row][i]==secret[i]) { marks[row][i]=2; used[i]=1; }
                }
                for (int i=0;i<5;i++) {
                    if (marks[row][i]==2) continue;
                    for (int j=0;j<5;j++)
                        if (!used[j] && grid[row][i]==secret[j]) { marks[row][i]=1; used[j]=1; break; }
                }
                if (strncmp(grid[row], secret, 5)==0) { done=1; won=1; }
                else if (row==5) done=1;
                else { row++; col=0; }
            }
        }
        BeginDrawing();
        ClearBackground((Color){30,30,40,255});
        DrawText("WORD  type letters  ENTER  BACKSPACE  ESC", 20, 20, 22, RAYWHITE);
        for (int r=0;r<=row && r<6;r++) {
            for (int c=0;c<5;c++) {
                Color bg = DARKGRAY;
                if (r<row || done) {
                    if (marks[r][c]==2) bg=GREEN;
                    else if (marks[r][c]==1) bg=GOLD;
                    else bg=(Color){60,60,60,255};
                }
                int x=SW/2-160+c*70, y=120+r*70;
                DrawRectangle(x,y,60,60,bg);
                if (grid[r][c])
                    DrawText(TextFormat("%c", grid[r][c]), x+18, y+12, 36, WHITE);
            }
        }
        if (done) DrawText(won?"CORRECT":TextFormat("Word was %s", secret), 40, 560, 28, won?GREEN:ORANGE);
        EndDrawing();
    }
    CloseWindow();
}
