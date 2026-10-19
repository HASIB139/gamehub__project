#include "hub.h"

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #define NOGDI
  #define NOUSER
  #include <windows.h>
#endif

void enable_console(void) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | 0x0004);
#endif
}

void score_load(Player *p, const char *name) {
    memset(p, 0, sizeof *p);
    strncpy(p->name, name, MAX_NAME - 1);
    FILE *fp = fopen(SCORE_FILE, "r");
    if (!fp) return;
    char n[MAX_NAME];
    int w[8], bg, bc, bs;
    while (fscanf(fp, "%31s %d %d %d %d %d %d %d %d %d %d %d",
                  n,&w[0],&w[1],&w[2],&w[3],&w[4],&w[5],&w[6],&w[7],
                  &bg,&bc,&bs) == 12) {
        if (strcmp(n, name) == 0) {
            memcpy(p->wins, w, sizeof w);
            p->best_golf = bg; p->best_candy = bc; p->best_snake = bs;
            break;
        }
    }
    fclose(fp);
}

void score_save(const Player *p) {
    FILE *fp = fopen(SCORE_FILE, "w");
    if (!fp) return;
    fprintf(fp, "%s", p->name);
    for (int i = 0; i < 8; i++) fprintf(fp, " %d", p->wins[i]);
    fprintf(fp, " %d %d %d\n", p->best_golf, p->best_candy, p->best_snake);
    fclose(fp);
}

#define R   "\033[0m"
#define B   "\033[1m"
#define D   "\033[2m"
#define C   "\033[1;36m"
#define Y   "\033[1;33m"
#define G   "\033[1;32m"
#define M   "\033[1;35m"
#define RD  "\033[1;31m"
#define BL  "\033[1;34m"

void shell_banner(const Player *p) {
    time_t t = time(NULL);
    char *ts = ctime(&t);
    ts[strlen(ts) - 1] = '\0';
    system(CLEAR_CMD);

    /* neofetch-style: logo left, info right */
    printf("\n");
    printf(C B "   _____          __  __ ______" R "     " G "%s" R "@" C "hub" R "\n", p->name);
    printf(C B "  / ____|   /\\   |  \\/  |  ____|" R "     ---------------\n");
    printf(C B " | |  __   /  \\  | \\  / | |__   " R "     " Y "OS" R ":      GameHub Console\n");
    printf(C B " | | |_ | / /\\ \\ | |\\/| |  __|  " R "     " Y "Host" R ":    campus-terminal\n");
    printf(C B " | |__| |/ ____ \\| |  | | |____ " R "     " Y "Kernel" R ":  C99 + raylib\n");
    printf(C B "  \\_____/_/    \\_\\_|  |_|______|" R "     " Y "Shell" R ":   hub-sh 2.1\n");
    printf(Y B "           H U B                " R "     " Y "Rooms" R ":   8 games\n");
    printf("                                  " Y "User" R ":    %s\n", p->name);
    printf("                                  " Y "Time" R ":    %s\n", ts);
    printf("                                  " Y "Theme" R ":   "
           RD "*" R " " G "*" R " " Y "*" R " " BL "*" R " " M "*" R " " C "*" R "\n");
    printf(D "  ========================================================\n" R);
    printf("  type " Y "help" R " · " Y "neofetch" R " · " Y "ls" R " · room name · " Y "exit" R "\n\n");
}

void shell_help(void) {
    printf("\n  " C B "hub-sh commands" R "\n\n");
    printf("  " B "system" R "\n");
    printf("    " Y "neofetch" R " / " Y "fetch" R "   system card (this look)\n");
    printf("    " Y "help" R "                 this list\n");
    printf("    " Y "ls" R "                   list rooms\n");
    printf("    " Y "status" R "               your scores\n");
    printf("    " Y "clear" R "                redraw\n");
    printf("    " Y "union" R "                demo union\n");
    printf("    " Y "exit" R "                 logout\n\n");
    printf("  " B "rooms" R "  (opens raylib window)\n");
    printf("    " G "connect4" R "    " G "blackjack" R "   " G "snakes" R "   " G "ludo" R "\n");
    printf("    " G "snake" R "       " G "word" R "        " G "candy" R "    " G "golf" R " (8 levels)\n\n");
}
