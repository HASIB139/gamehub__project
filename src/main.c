#include "hub.h"

static void status(const Player *p) {
    const char *names[] = {
    "connect4","blackjack","snakes","ludo",
    "snake","word","candy","golf","parkour"
    };
    printf("\n  player: %s\n", p->name);
    for (int i = 0; i < G_COUNT; i++)
        printf("    %-12s wins %d\n", names[i], p->wins[i]);
    printf("    best golf %d  candy %d  snake %d\n\n",
           p->best_golf, p->best_candy, p->best_snake);
}

static void run_game(void (*fn)(Player *), Player *p, GameId id) {
    printf("  opening graphics window...\n");
    fflush(stdout);
    fn(p);
    p->wins[id]++; /* count session entry; games may also bump wins */
    score_save(p);
    printf("  back to hub-sh.\n");
}

int main(void) {
    enable_console();
    srand((unsigned)time(NULL));

    char name[MAX_NAME];
    if (!auth_login(name, MAX_NAME)) return 0;


    Player p;
    score_load(&p, name);
    shell_banner(&p);

    /* demo union for viva topic */
    IntBytes u;
    u.as_int = 0x41424344;

    char line[128];
    while (1) {
        printf("\033[1;32mplayer@hub\033[0m:\033[1;36m~\033[0m$ ");
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin)) break;
        line[strcspn(line, "\n")] = '\0';
        char *cmd = line;
        while (*cmd == ' ') cmd++;
        if (!*cmd) continue;

        if (!strcmp(cmd, "help") || !strcmp(cmd, "man")) shell_help();
        else if (!strcmp(cmd, "neofetch") || !strcmp(cmd, "fetch")) shell_banner(&p);
        else if (!strcmp(cmd, "ls")) {
            printf("  rooms/  connect4 blackjack snakes ludo snake word candy golf parkour\n");
            printf("  data/   scores.txt\n");
        } else if (!strcmp(cmd, "status")) status(&p);
         else if (!strcmp(cmd, "leaderboard") || !strcmp(cmd, "lb")) lb_print_all();
        else if (!strcmp(cmd, "passwd")) auth_change_password(p.name);    
        else if (!strcmp(cmd, "clear") || !strcmp(cmd, "cls")) shell_banner(&p);
        else if (!strcmp(cmd, "union")) {
            printf("  union IntBytes demo: as_int=%d bytes='%c%c%c%c'\n",
                   u.as_int, u.as_bytes[0], u.as_bytes[1],
                   u.as_bytes[2], u.as_bytes[3]);
        } else if (!strcmp(cmd, "exit") || !strcmp(cmd, "quit")) {
            score_save(&p);
            printf("  logout. bye %s\n", p.name);
            break;
        } else if (!strcmp(cmd, "connect4") || !strcmp(cmd, "1"))
            run_game(game_connect4, &p, G_CONNECT4);
        else if (!strcmp(cmd, "blackjack") || !strcmp(cmd, "2"))
            run_game(game_blackjack, &p, G_BLACKJACK);
        else if (!strcmp(cmd, "snakes") || !strcmp(cmd, "3"))
            run_game(game_snakes, &p, G_SNAKES);
        else if (!strcmp(cmd, "ludo") || !strcmp(cmd, "4"))
            run_game(game_ludo, &p, G_LUDO);
        else if (!strcmp(cmd, "snake") || !strcmp(cmd, "5"))
            run_game(game_snake, &p, G_SNAKE);
        else if (!strcmp(cmd, "word") || !strcmp(cmd, "6"))
            run_game(game_word, &p, G_WORD);
        else if (!strcmp(cmd, "candy") || !strcmp(cmd, "7"))
            run_game(game_candy, &p, G_CANDY);
        else if (!strcmp(cmd, "golf") || !strcmp(cmd, "8"))
            run_game(game_golf, &p, G_GOLF);
        else if (!strcmp(cmd, "parkour") || !strcmp(cmd, "9"))
            run_game(game_parkour, &p, G_PARKOUR);
        else
            printf("  hub-sh: \033[1;31m%s\033[0m: not found — try help\n", cmd);
    }
    return 0;
}
