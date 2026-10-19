#include "hub.h"

typedef struct { int rank, suit; } Card;

static void shuffle(Card *d, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = GetRandomValue(0, i);
        Card t = d[i]; d[i] = d[j]; d[j] = t;
    }
}
static int val(Card c) {
    if (c.rank >= 10) return 10;
    if (c.rank == 1) return 11;
    return c.rank;
}
static int total(Card *h, int n) {
    int s = 0, a = 0;
    for (int i = 0; i < n; i++) {
        s += val(h[i]);
        if (h[i].rank == 1) a++;
    }
    while (s > 21 && a--) s -= 10;
    return s;
}

/* Draw a suit symbol using circles and triangles.
   suit: 0 spade, 1 heart, 2 diamond, 3 club.  s = size, (cx,cy) = center. */
static void draw_suit(int suit, float cx, float cy, float s, Color c) {
    switch (suit) {
    case 0:  /* spade */
        DrawCircleV((Vector2){ cx - s*0.28f, cy + s*0.10f }, s*0.32f, c);
        DrawCircleV((Vector2){ cx + s*0.28f, cy + s*0.10f }, s*0.32f, c);
        DrawTriangle((Vector2){ cx, cy - s*0.70f },
                     (Vector2){ cx - s*0.58f, cy + s*0.10f },
                     (Vector2){ cx + s*0.58f, cy + s*0.10f }, c);
        DrawTriangle((Vector2){ cx, cy + s*0.10f },
                     (Vector2){ cx - s*0.22f, cy + s*0.70f },
                     (Vector2){ cx + s*0.22f, cy + s*0.70f }, c);
        break;
    case 1:  /* heart */
        DrawCircleV((Vector2){ cx - s*0.28f, cy - s*0.20f }, s*0.34f, c);
        DrawCircleV((Vector2){ cx + s*0.28f, cy - s*0.20f }, s*0.34f, c);
        DrawTriangle((Vector2){ cx - s*0.60f, cy - s*0.10f },
                     (Vector2){ cx, cy + s*0.70f },
                     (Vector2){ cx + s*0.60f, cy - s*0.10f }, c);
        break;
    case 2:  /* diamond */
        DrawPoly((Vector2){ cx, cy }, 4, s*0.75f, 0, c);
        break;
    default: /* club */
        DrawCircleV((Vector2){ cx, cy - s*0.35f }, s*0.30f, c);
        DrawCircleV((Vector2){ cx - s*0.33f, cy + s*0.10f }, s*0.30f, c);
        DrawCircleV((Vector2){ cx + s*0.33f, cy + s*0.10f }, s*0.30f, c);
        DrawTriangle((Vector2){ cx, cy + s*0.10f },
                     (Vector2){ cx - s*0.22f, cy + s*0.70f },
                     (Vector2){ cx + s*0.22f, cy + s*0.70f }, c);
        break;
    }
}

/* Draw one playing card. hide = 1 draws the back of the card. */
static void draw_card(Card c, int x, int y, int hide) {
    const int W = 78, H = 110;

    /* soft shadow, thin dark outline */
    DrawRectangleRounded((Rectangle){ x + 4, y + 5, W, H }, 0.12f, 8, Fade(BLACK, 0.35f));
    DrawRectangleRounded((Rectangle){ x - 1, y - 1, W + 2, H + 2 }, 0.12f, 8, (Color){ 40, 40, 40, 255 });

    if (hide) {
        DrawRectangleRounded((Rectangle){ x, y, W, H }, 0.12f, 8, (Color){ 25, 55, 140, 255 });
        DrawRectangleRounded((Rectangle){ x + 8, y + 8, W - 16, H - 16 }, 0.12f, 8, (Color){ 35, 70, 160, 255 });
        DrawText("HUB", x + 22, y + 42, 22, (Color){ 200, 180, 80, 255 });
        return;
    }

    int red = (c.suit == 1 || c.suit == 2);
    Color ink = red ? (Color){ 200, 30, 30, 255 } : (Color){ 20, 20, 20, 255 };
    const char *rn[] = {"?","A","2","3","4","5","6","7","8","9","10","J","Q","K"};

    DrawRectangleRounded((Rectangle){ x, y, W, H }, 0.12f, 8, RAYWHITE);

    /* top-left: rank + small suit */
    DrawText(rn[c.rank], x + 8, y + 6, 22, ink);
    draw_suit(c.suit, x + 16, y + 44, 12, ink);

    /* center: big suit */
    draw_suit(c.suit, x + W / 2.0f, y + H / 2.0f + 8, 30, ink);

    /* bottom-right: rank */
    int w = MeasureText(rn[c.rank], 22);
    DrawText(rn[c.rank], x + W - w - 8, y + H - 28, 22, ink);
}

void game_blackjack(Player *p) {
    (void)p;
    Card deck[52];
    int k = 0;
    for (int s = 0; s < 4; s++)
        for (int r = 1; r <= 13; r++) {
            deck[k].suit = s;
            deck[k].rank = r;
            k++;
        }
    shuffle(deck, 52);
    int top = 0;
    Card ph[12], dh[12];
    int pn = 0, dn = 0;
    ph[pn++] = deck[top++]; dh[dn++] = deck[top++];
    ph[pn++] = deck[top++]; dh[dn++] = deck[top++];
    int phase = 0;
    const char *result = "";

    gfx_open("Blackjack");

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) break;

        /* ---------------- UPDATE ---------------- */
        if (phase == 0) {
            if (IsKeyPressed(KEY_H) && total(ph, pn) < 21) {
                ph[pn++] = deck[top++];
                if (total(ph, pn) > 21) { phase = 1; result = "BUST - dealer wins"; }
            }
            if (IsKeyPressed(KEY_S)) {
                while (total(dh, dn) < 17) dh[dn++] = deck[top++];
                int pt = total(ph, pn), dt = total(dh, dn);
                if (dt > 21 || pt > dt) {
                    result = "YOU WIN";
                    fx_burst(SW / 2.0f, SH - 50, GOLD, 40);
                }
                else if (pt < dt) result = "DEALER WINS";
                else result = "PUSH (draw)";
                phase = 1;
            }
        }
        if (phase && IsKeyPressed(KEY_N)) {
            /* new hand */
            shuffle(deck, 52); top = 0; pn = dn = 0; phase = 0; result = "";
            ph[pn++] = deck[top++]; dh[dn++] = deck[top++];
            ph[pn++] = deck[top++]; dh[dn++] = deck[top++];
        }

        /* ---------------- DRAW ---------------- */
        BeginDrawing();

        /* felt table: gradient, plus a lighter oval in the middle */
        gfx_background((Color){ 14, 105, 52, 255 }, (Color){ 6, 58, 28, 255 });
        DrawEllipse(SW / 2, SH / 2 + 10, SW * 0.42f, SH * 0.36f, Fade(WHITE, 0.04f));
        DrawEllipseLines(SW / 2, SH / 2 + 10, SW * 0.42f, SH * 0.36f, Fade(GOLD, 0.35f));
        gfx_topbar("BLACKJACK", "H hit | S stand | N new hand | ESC hub");

        /* dealer row */
        gfx_text_shadow("DEALER", 40, 78, 24, (Color){ 230, 230, 190, 255 });
        for (int i = 0; i < dn; i++)
            draw_card(dh[i], 40 + i * 90, 115, phase == 0 && i == 0);
        if (phase)
            gfx_text_shadow(TextFormat("total %d", total(dh, dn)), 40 + dn * 90 + 10, 155, 24, YELLOW);

        /* player row */
        gfx_text_shadow("YOU", 40, 318, 24, (Color){ 230, 230, 190, 255 });
        for (int i = 0; i < pn; i++)
            draw_card(ph[i], 40 + i * 90, 355, 0);
        gfx_text_shadow(TextFormat("total %d", total(ph, pn)), 40 + pn * 90 + 10, 395, 24, YELLOW);

        fx_update_draw();

        /* bottom bar */
        if (phase) {
            DrawRectangleGradientV(0, SH - 74, SW, 74, Fade(BLACK, 0.45f), Fade(BLACK, 0.75f));
            DrawRectangle(0, SH - 74, SW, 2, GOLD);
            gfx_text_shadow(result, 40, SH - 52, 34, GOLD);
            DrawText("press N for new hand", SW - MeasureText("press N for new hand", 22) - 30, SH - 42, 22, LIGHTGRAY);
        } else {
            gfx_text_shadow("Your move:  H = hit    S = stand", 40, SH - 44, 22, RAYWHITE);
        }
        EndDrawing();
    }
    CloseWindow();
}