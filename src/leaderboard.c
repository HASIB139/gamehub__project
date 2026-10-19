#include <stdio.h>
#include <string.h>
#include "hub.h"

/* ------------------------------------------------------------------
   Leaderboard: one row per (game, username).
   Saved in data/leaderboard.txt, one line each:   game name score
   ------------------------------------------------------------------ */

#define LB_MAX 500
#define LB_FILE "data/leaderboard.txt"

static LbRow rows[LB_MAX];   /* every row from the file */
static int nrows = 0;

/* Make a name safe to store: spaces/tabs become '_', empty becomes "player". */
static void clean_name(char *dst, const char *src, int size) {
    int j = 0;
    for (int i = 0; src[i] != '\0' && j < size - 1; i++)
        dst[j++] = ((unsigned char)src[i] <= ' ') ? '_' : src[i];
    dst[j] = '\0';
    if (j == 0) snprintf(dst, (size_t)size, "player");
}

static void lb_load(void) {
    nrows = 0;
    FILE *f = fopen(LB_FILE, "r");
    if (f == NULL) return;                         /* no file yet: empty board */
    /* %15s and %23s must match LB_GAME-1 and LB_NAME-1 in hub.h */
    while (nrows < LB_MAX &&
           fscanf(f, "%15s %23s %d", rows[nrows].game, rows[nrows].name, &rows[nrows].score) == 3)
        nrows++;
    fclose(f);
}

static void lb_save(void) {
    FILE *f = fopen(LB_FILE, "w");
    if (f == NULL) return;
    for (int i = 0; i < nrows; i++)
        fprintf(f, "%s %s %d\n", rows[i].game, rows[i].name, rows[i].score);
    fclose(f);
}

static LbRow *lb_find(const char *game, const char *who) {
    for (int i = 0; i < nrows; i++)
        if (strcmp(rows[i].game, game) == 0 && strcmp(rows[i].name, who) == 0)
            return &rows[i];
    return NULL;
}

static void lb_insert(const char *game, const char *who, int score) {
    if (nrows >= LB_MAX) return;
    snprintf(rows[nrows].game, sizeof rows[nrows].game, "%s", game);
    snprintf(rows[nrows].name, sizeof rows[nrows].name, "%s", who);
    rows[nrows].score = score;
    nrows++;
}

/* Keep each player's BEST score for a game.
   low_is_better = 1 for games like golf (fewer strokes wins). */
void lb_submit(const char *game, const char *name, int score, int low_is_better) {
    if (score <= 0) return;
    char who[LB_NAME];
    clean_name(who, name, sizeof who);

    lb_load();
    LbRow *r = lb_find(game, who);
    if (r == NULL) {
        lb_insert(game, who, score);
    } else if (low_is_better ? score < r->score : score > r->score) {
        r->score = score;
    } else {
        return;                                    /* not a new best: nothing to save */
    }
    lb_save();
}

/* Keep a RUNNING TOTAL for a player (used for "number of wins"). */
void lb_add(const char *game, const char *name, int delta) {
    if (delta <= 0) return;
    char who[LB_NAME];
    clean_name(who, name, sizeof who);

    lb_load();
    LbRow *r = lb_find(game, who);
    if (r == NULL) lb_insert(game, who, delta);
    else r->score += delta;
    lb_save();
}

/* Fill out[] with the best `max` rows for one game. Returns how many rows. */
int lb_top(const char *game, int low_is_better, LbRow *out, int max) {
    static LbRow all[LB_MAX];
    int n = 0;

    lb_load();
    for (int i = 0; i < nrows; i++)
        if (strcmp(rows[i].game, game) == 0) all[n++] = rows[i];

    /* insertion sort: best score first */
    for (int i = 1; i < n; i++) {
        LbRow key = all[i];
        int j = i - 1;
        while (j >= 0 && (low_is_better ? all[j].score > key.score : all[j].score < key.score)) {
            all[j + 1] = all[j];
            j--;
        }
        all[j + 1] = key;
    }

    if (n > max) n = max;
    for (int i = 0; i < n; i++) out[i] = all[i];
    return n;
}

/* Hold TAB inside a game window to see the top 5.
   Call this once per frame, just before EndDrawing(). */
void lb_overlay(const char *game, int low_is_better, const char *me) {
    static LbRow top[5];
    static int n = 0;

    if (IsKeyPressed(KEY_TAB)) n = lb_top(game, low_is_better, top, 5);   /* refresh once per press */
    if (!IsKeyDown(KEY_TAB)) return;

    char who[LB_NAME];
    clean_name(who, me, sizeof who);

    Rectangle box = { SW / 2.0f - 200, SH / 2.0f - 180, 400, 360 };
    DrawRectangle(0, 0, SW, SH, Fade(BLACK, 0.55f));
    gfx_panel(box, (Color){ 25, 28, 50, 245 });
    gfx_text_center(TextFormat("TOP 5 - %s", TextToUpper(game)), SW / 2, (int)box.y + 16, 30, GOLD);
    DrawRectangle((int)box.x + 20, (int)box.y + 60, 360, 2, Fade(GOLD, 0.6f));

    for (int i = 0; i < 5; i++) {
        int y = (int)box.y + 76 + i * 48;
        if (i < n) {
            Color c = (strcmp(top[i].name, who) == 0) ? GOLD : RAYWHITE;   /* your own row is gold */
            DrawText(TextFormat("%d.", i + 1), (int)box.x + 28, y, 26, c);
            DrawText(TextFormat("%.14s", top[i].name), (int)box.x + 70, y, 26, c);
            const char *s = TextFormat("%d", top[i].score);
            DrawText(s, (int)box.x + 372 - MeasureText(s, 26), y, 26, c);
        } else {
            DrawText(TextFormat("%d.", i + 1), (int)box.x + 28, y, 26, GRAY);
            DrawText("-", (int)box.x + 70, y, 26, GRAY);
        }
    }
    gfx_text_center("release TAB to close", SW / 2, (int)box.y + 322, 18, LIGHTGRAY);
}

/* Text version for the shell: type  leaderboard */
void lb_print_all(void) {
    static const struct { const char *id; int low; } games[] = {
        { "snake", 0 }, { "candy", 0 }, { "golf", 1 }, { "blackjack", 0 }, { "connect4", 0 }, { "parkour", 0 }
    };
    LbRow top[5];

    for (int g = 0; g < (int)(sizeof games / sizeof games[0]); g++) {
        int n = lb_top(games[g].id, games[g].low, top, 5);
        printf("\n== %s ==\n", games[g].id);
        if (n == 0) printf("  (no scores yet)\n");
        for (int i = 0; i < n; i++)
            printf("  %d. %-14s %d\n", i + 1, top[i].name, top[i].score);
    }
    printf("\n");
}