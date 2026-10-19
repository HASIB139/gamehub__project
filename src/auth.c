#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#define USERS_FILE    "data/users.txt"
#define MAX_USERS     200
#define NAME_MIN      3
#define NAME_MAX_LEN  20
#define PW_MIN        6
#define PW_MAX        40
#define BACKUP_LEN    6         
#define ROUNDS        20000     

typedef struct {
    char name[NAME_MAX_LEN + 1];
    char salt[33];      
    char hash[65];      
    char bsalt[33];     
    char bhash[65];
} User;

static User users[MAX_USERS];
static int  nusers = 0;


static const uint32_t K256[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

#define ROR32(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static void sha256_block(uint32_t h[8], const unsigned char *p) {
    uint32_t w[64], a, b, c, d, e, f, g, hh;
    for (int i = 0; i < 16; i++)
        w[i] = ((uint32_t)p[4*i] << 24) | ((uint32_t)p[4*i+1] << 16) |
               ((uint32_t)p[4*i+2] << 8) | (uint32_t)p[4*i+3];
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = ROR32(w[i-15], 7) ^ ROR32(w[i-15], 18) ^ (w[i-15] >> 3);
        uint32_t s1 = ROR32(w[i-2], 17) ^ ROR32(w[i-2], 19) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    a = h[0]; b = h[1]; c = h[2]; d = h[3]; e = h[4]; f = h[5]; g = h[6]; hh = h[7];
    for (int i = 0; i < 64; i++) {
        uint32_t S1 = ROR32(e, 6) ^ ROR32(e, 11) ^ ROR32(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t t1 = hh + S1 + ch + K256[i] + w[i];
        uint32_t S0 = ROR32(a, 2) ^ ROR32(a, 13) ^ ROR32(a, 22);
        uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = S0 + mj;
        hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}

static void sha256(const unsigned char *data, size_t len, unsigned char out[32]) {
    uint32_t h[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                      0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    unsigned char block[128];
    size_t full = len / 64, rem = len % 64;

    for (size_t i = 0; i < full; i++) sha256_block(h, data + i * 64);

    memcpy(block, data + full * 64, rem);
    block[rem++] = 0x80;
    size_t total = (rem <= 56) ? 64 : 128;
    memset(block + rem, 0, total - rem);
    uint64_t bits = (uint64_t)len * 8;
    for (int i = 0; i < 8; i++) block[total - 1 - i] = (unsigned char)(bits >> (8 * i));
    sha256_block(h, block);
    if (total == 128) sha256_block(h, block + 64);

    for (int i = 0; i < 8; i++) {
        out[4*i]   = (unsigned char)(h[i] >> 24);
        out[4*i+1] = (unsigned char)(h[i] >> 16);
        out[4*i+2] = (unsigned char)(h[i] >> 8);
        out[4*i+3] = (unsigned char)(h[i]);
    }
}

static void to_hex(const unsigned char *in, int n, char *out) {
    static const char digits[] = "0123456789abcdef";
    for (int i = 0; i < n; i++) {
        out[2*i]   = digits[in[i] >> 4];
        out[2*i+1] = digits[in[i] & 15];
    }
    out[2*n] = '\0';
}


static void hash_secret(const char *salt_hex, const char *secret, char out_hex[65]) {
    unsigned char buf[32 + 32 + 128], h[32];
    size_t sl = strlen(salt_hex), pl = strlen(secret);
    if (pl > 128) pl = 128;

    memcpy(buf, salt_hex, sl);
    memcpy(buf + sl, secret, pl);
    sha256(buf, sl + pl, h);
    for (int i = 1; i < ROUNDS; i++) {
        memcpy(buf, h, 32);
        memcpy(buf + 32, salt_hex, sl);
        memcpy(buf + 32 + sl, secret, pl);
        sha256(buf, 32 + sl + pl, h);
    }
    to_hex(h, 32, out_hex);
}

static int same_hash(const char *a, const char *b) {
    size_t la = strlen(a), lb = strlen(b);
    unsigned char diff = 0;
    if (la != lb) return 0;
    for (size_t i = 0; i < la; i++) diff |= (unsigned char)(a[i] ^ b[i]);
    return diff == 0;
}

static uint64_t rng_state = 0;

static void rng_mix(uint64_t v) {
    rng_state ^= v + 0x9e3779b97f4a7c15ULL + (rng_state << 6) + (rng_state >> 2);
}

static void rng_init(void) {
    static int done = 0;
    if (done) return;
    done = 1;
    rng_mix((uint64_t)time(NULL));
    rng_mix((uint64_t)clock());
    rng_mix((uint64_t)(uintptr_t)&done);
#ifndef _WIN32
    FILE *f = fopen("/dev/urandom", "rb");
    if (f) {
        uint64_t r = 0;
        if (fread(&r, sizeof r, 1, f) == 1) rng_mix(r);
        fclose(f);
    }
#endif
}

static uint32_t rng_next(void) {            
    rng_init();
    rng_state += 0x9e3779b97f4a7c15ULL;
    uint64_t z = rng_state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    z ^= z >> 31;
    return (uint32_t)(z >> 16);
}

static void new_salt(char out[33]) {
    unsigned char b[16];
    for (int i = 0; i < 16; i++) b[i] = (unsigned char)(rng_next() & 0xff);
    to_hex(b, 16, out);
}



static int read_line(char *buf, int size) {
    if (!fgets(buf, size, stdin)) return 0;
    if (!strchr(buf, '\n')) {                 
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) { }
    }
    buf[strcspn(buf, "\r\n")] = '\0';
    rng_mix((uint64_t)clock());
    return 1;
}

/* read a secret: nothing (or * on Windows) is shown while typing */
static int read_hidden(const char *prompt, char *buf, int size) {
    printf("%s", prompt);
    fflush(stdout);
#ifdef _WIN32
    int n = 0, ch;
    for (;;) {
        ch = _getch();
        rng_mix((uint64_t)clock());
        if (ch == '\r' || ch == '\n') break;
        if (ch == 3) { printf("\n"); exit(0); }                   
        if (ch == 0 || ch == 224) { (void)_getch(); continue; }    
        if (ch == '\b' || ch == 127) {
            if (n > 0) { n--; printf("\b \b"); fflush(stdout); }
        } else if (isprint(ch) && n < size - 1) {
            buf[n++] = (char)ch;
            printf("*");
            fflush(stdout);
        }
    }
    buf[n] = '\0';
    printf("\n");
    return 1;
#else
    struct termios oldt, newt;
    int tty = isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &oldt) == 0;
    if (tty) {
        newt = oldt;
        newt.c_lflag &= ~(tcflag_t)ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    }
    int ok = read_line(buf, size);
    if (tty) {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        printf("\n");
    }
    return ok;
#endif
}

static void trim(char *s) {
    char *start = s;
    while (*start == ' ' || *start == '\t') start++;
    memmove(s, start, strlen(start) + 1);
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == ' ' || s[n-1] == '\t')) s[--n] = '\0';
}



static int valid_name(const char *s) {
    int n = (int)strlen(s);
    if (n < NAME_MIN || n > NAME_MAX_LEN) return 0;
    for (int i = 0; i < n; i++)
        if (!isalnum((unsigned char)s[i]) && s[i] != '_') return 0;
    return 1;
}

static const char *password_problem(const char *pw) {
    if ((int)strlen(pw) < PW_MIN) return "is too short (use at least 6 characters)";
    if (strcmp(pw, "!forgot") == 0) return "is a reserved word";
    return NULL;
}

static int same_name(const char *a, const char *b) {      /* ignores upper/lower case */
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}



static void load_users(void) {
    nusers = 0;
    FILE *f = fopen(USERS_FILE, "r");
    if (f == NULL) return;                              
    while (nusers < MAX_USERS &&
           fscanf(f, "%20s %32s %64s %32s %64s",
                  users[nusers].name, users[nusers].salt, users[nusers].hash,
                  users[nusers].bsalt, users[nusers].bhash) == 5)
        nusers++;
    fclose(f);
}

static int save_users(void) {
    FILE *f = fopen(USERS_FILE, "w");
    if (f == NULL) {
        printf("  error: cannot write %s (does the data folder exist?)\n", USERS_FILE);
        return 0;
    }
    for (int i = 0; i < nusers; i++)
        fprintf(f, "%s %s %s %s %s\n", users[i].name, users[i].salt, users[i].hash,
                users[i].bsalt, users[i].bhash);
    fclose(f);
    return 1;
}

static User *find_user(const char *name) {
    for (int i = 0; i < nusers; i++)
        if (same_name(users[i].name, name)) return &users[i];
    return NULL;
}


static void set_password(User *u, const char *pw) {
    new_salt(u->salt);
    hash_secret(u->salt, pw, u->hash);
}

static void make_backup_code(User *u, char *code) {
    char norm[BACKUP_LEN + 1];
    int k = 0;
    for (int i = 0; i < BACKUP_LEN; i++) {
        char c = (char)('0' + rng_next() % 10);
        norm[i] = c;
        if (i > 0 && i % 3 == 0) code[k++] = '-';
        code[k++] = c;
    }
    norm[BACKUP_LEN] = '\0';
    code[k] = '\0';
    new_salt(u->bsalt);
    hash_secret(u->bsalt, norm, u->bhash);
}

static void normalize_code(const char *in, char *out, int size) {   
    int n = 0;
    for (; *in && n < size - 1; in++)
        if (isalnum((unsigned char)*in)) out[n++] = (char)toupper((unsigned char)*in);
    out[n] = '\0';
}

static void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void show_backup_code(const char *code) {
    char dummy[16];
    printf("\n");
    printf("  +----------------------------------------------+\n");
    printf("  |  YOUR BACKUP CODE:   %-24s|\n", code);
    printf("  |                                              |\n");
    printf("  |  Write it down and keep it somewhere safe.   |\n");
    printf("  |  If you forget your password, this code is   |\n");
    printf("  |  the ONLY way back in. It is shown just once.|\n");
    printf("  +----------------------------------------------+\n\n");
    printf("  Press Enter after you have saved it... ");
    fflush(stdout);
    read_line(dummy, sizeof dummy);
    clear_screen();
}


static int ask_new_password(char *pw, int size) {
    char again[PW_MAX + 1];
    for (int tries = 0; tries < 3; tries++) {
        if (!read_hidden("  choose a password (min 6 characters): ", pw, size)) return 0;
        const char *bad = password_problem(pw);
        if (bad) { printf("  that password %s\n", bad); continue; }
        if (!read_hidden("  type it again: ", again, (int)sizeof again)) return 0;
        if (strcmp(pw, again) != 0) { printf("  the two passwords do not match\n"); continue; }
        return 1;
    }
    printf("  too many tries.\n");
    return 0;
}

static User *create_account(const char *name) {
    char pw[PW_MAX + 1], code[BACKUP_LEN + 4];
    if (nusers >= MAX_USERS) { printf("  sorry, the user list is full\n"); return NULL; }
    if (!ask_new_password(pw, (int)sizeof pw)) return NULL;

    User *u = &users[nusers];
    memset(u, 0, sizeof *u);
    snprintf(u->name, sizeof u->name, "%s", name);
    set_password(u, pw);
    make_backup_code(u, code);
    nusers++;
    if (!save_users()) { nusers--; return NULL; }     
    show_backup_code(code);
    return u;
}

static int recover(User *u) {
    char typed[64], norm[32], check[65], pw[PW_MAX + 1], code[BACKUP_LEN + 4];
    for (int tries = 0; tries < 3; tries++) {
        if (!read_hidden("  backup code: ", typed, (int)sizeof typed)) return 0;
        normalize_code(typed, norm, (int)sizeof norm);
        hash_secret(u->bsalt, norm, check);
        if (same_hash(check, u->bhash)) {
            printf("  code accepted. choose a new password.\n");
            if (!ask_new_password(pw, (int)sizeof pw)) return 0;
            set_password(u, pw);
            make_backup_code(u, code);               
            if (!save_users()) return 0;
            show_backup_code(code);
            return 1;
        }
        printf("  wrong backup code\n");
    }
    printf("  too many wrong codes.\n");
    return 0;
}

static int check_password(User *u) {
    char pw[PW_MAX + 1], h[65];
    for (int tries = 0; tries < 3; tries++) {
        if (!read_hidden("  password (or type !forgot): ", pw, (int)sizeof pw)) return 0;
        if (strcmp(pw, "!forgot") == 0) return recover(u);
        hash_secret(u->salt, pw, h);
        if (same_hash(h, u->hash)) return 1;
        printf("  wrong password\n");
    }
    printf("  too many wrong passwords.\n");
    return 0;
}


int auth_login(char *name_out, int size) {
    char typed[64], answer[16];

    load_users();
    printf("\n  === game-hub login ===\n");
    for (int round = 0; round < 5; round++) {
        printf("  username: ");
        fflush(stdout);
        if (!read_line(typed, (int)sizeof typed)) return 0;
        trim(typed);
        if (!valid_name(typed)) {
            printf("  usernames: 3 to 20 letters, digits or _ (no spaces)\n");
            continue;
        }

        User *u = find_user(typed);
        if (u == NULL) {                                   /* new username */
            printf("  no account named '%s'. create it? (y/n): ", typed);
            fflush(stdout);
            if (!read_line(answer, (int)sizeof answer)) return 0;
            if (tolower((unsigned char)answer[0]) != 'y') continue;
            u = create_account(typed);
            if (u == NULL) continue;
        } else if (!check_password(u)) {                   
            return 0;
        }

        snprintf(name_out, (size_t)size, "%s", u->name);   
        printf("  welcome, %s!\n", u->name);
        return 1;
    }
    printf("  too many tries.\n");
    return 0;
}


int auth_change_password(const char *name) {
    char pw[PW_MAX + 1], h[65];

    load_users();
    User *u = find_user(name);
    if (u == NULL) { printf("  account not found\n"); return 0; }
    if (!read_hidden("  current password: ", pw, (int)sizeof pw)) return 0;
    hash_secret(u->salt, pw, h);
    if (!same_hash(h, u->hash)) { printf("  wrong password\n"); return 0; }
    if (!ask_new_password(pw, (int)sizeof pw)) return 0;
    set_password(u, pw);
    if (!save_users()) return 0;
    printf("  password changed.\n");
    return 1;
}