// Pirate Chest 3DS - donnees communes
#ifndef COMMON_H
#define COMMON_H

#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>

#define MAXC    30
#define BASEC   150
#define PRICE   1000
#define RBBASE  2000      /* le rebirth commence a 2000 */
#define MAXLVL  50
#define LCAP    0.35
#define RPR     (1.0/50)
#define WPR     (1.0/20)
#define BPR     (1.0/15)
#define WW      1300
#define WH      860
#define NMAPS   8
#define NSKINS  21
#define GOLDSK  20
#define NODDS   6
#define PI_F    3.14159265f

#define RGB(h)     C2D_Color32((((h)>>16)&255),(((h)>>8)&255),((h)&255),255)
#define RGBA(h,a)  C2D_Color32((((h)>>16)&255),(((h)>>8)&255),((h)&255),(a))

static float rnd(float a, float b) { return a + (rand() / (float)RAND_MAX) * (b - a); }
static float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

enum { A_NONE=0, A_NINJA, A_ROBOT, A_ASTRO, A_COWBOY, A_CROWN, A_CHEF, A_ZOMBIE, A_WIZARD, A_PIRATE,
       A_BOLT, A_WINGS, A_SAILOR, A_STARS, A_HORNS, A_CRYSTAL, A_HALO, A_VISOR, A_VIKING, A_SAMURAI };
enum { T_NONE=0, T_GAL, T_GOLD, T_LAVA, T_ICE, T_NEON };

typedef struct { const char *n; u32 c; int acc, p; float spd, cm, ch; int tex, legend, nw; } Skin;

static const Skin SKINS[NSKINS] = {
  {"Classique", 0xffffff, A_NONE,   0,     1,   1, 1,   T_NONE, 0, 0},
  {"Ninja",     0x2a2a2a, A_NINJA,  0,     1,   1, 1,   T_NONE, 0, 0},
  {"Robo",      0xb8c4d6, A_ROBOT,  0,     1,   1, 1,   T_NONE, 0, 0},
  {"Astronaute",0xf2f2f2, A_ASTRO,  0,     1,   1, 1,   T_NONE, 0, 0},
  {"Cow-boy",   0xe8b78a, A_COWBOY, 0,     1,   1, 1,   T_NONE, 0, 0},
  {"Roi",       0x4da3ff, A_CROWN,  0,     1,   1, 1,   T_NONE, 0, 0},
  {"Chef",      0xff9d2e, A_CHEF,   0,     1,   1, 1,   T_NONE, 0, 0},
  {"Zombie",    0x7ac96b, A_ZOMBIE, 0,     1,   1, 1,   T_NONE, 0, 0},
  {"Sorcier",   0xb07bff, A_WIZARD, 0,     1,   1, 1,   T_NONE, 0, 0},
  {"Pirate",    0xff4d4d, A_PIRATE, 0,     1,   1, 1,   T_NONE, 0, 0},
  {"Matelot",   0x7fd1ff, A_SAILOR, 100,   1,   1, 1,   T_NONE, 0, 1},
  {"Viking",    0xe8c9a0, A_VIKING, 0,     1,   1, 1,   T_NONE, 0, 1},
  {"Fantôme",   0xe8f0ff, A_HALO,   0,     1,   1, 1,   T_NONE, 0, 1},
  {"Samouraï",  0xc0392b, A_SAMURAI,0,     1,   1, 1,   T_NONE, 0, 1},
  {"Éclair",    0xffe14d, A_BOLT,   75000, 1.5f,1, 1,   T_NONE, 0, 1},
  {"Turbo",     0xff6a2b, A_WINGS,  75000, 1.5f,1, 1,   T_NONE, 0, 1},
  {"Galaxie",   0x7a3cff, A_STARS,  250000,1,   2, 1,   T_GAL,  0, 1},
  {"Magma",     0xff6a00, A_HORNS,  250000,1,   3, 1,   T_LAVA, 0, 1},
  {"Cristal",   0x7fd6ff, A_CRYSTAL,250000,1.25f,1,1.5f,T_ICE,  0, 1},
  {"Néon",      0xff00e5, A_VISOR,  250000,1.75f,1.5f,1,T_NEON, 0, 1},
  {"Or hebdo",  0xffd41f, A_CROWN,  0,     2,   5, 2,   T_GOLD, 1, 0}
};

typedef struct { const char *lab; double p; u32 col; } Odd;
static const Odd ODDS[NODDS] = {
  {"Jeton boutique gratuite", 1.0/137500,  0xff3d7f},
  {"Skin d'or hebdo",         1.0/1000000, 0xffd41f},
  {"5 000 pièces",            1.0/7813,    0x9d4dff},
  {"500 pièces",              1.0/568,     0x2b8cff},
  {"75 pièces",               1.0/100,     0x3ec43e},
  {"10 pièces",               1.0/75,      0xf08a1c}
};
static const int ODDV[NODDS] = {0, 0, 5000, 500, 75, 10};

enum { D_PALM=0, D_ROCK, D_BUSH, D_CACTUS, D_DUNE, D_CAMEL, D_PINE, D_SNOWMAN, D_FLAKE, D_VOLCANO,
       D_FIRE, D_SKULL, D_FLOWER, D_PARROT, D_LOLLI, D_CANDY, D_CUPCAKE, D_DONUT, D_ICECREAM,
       D_STAR, D_PLANET, D_COMET, D_UFO, D_MOON, D_GEM, D_CROWN, D_BAG, D_TROPHY };
enum { ST_WOOD=0, ST_BRICK, ST_ICE, ST_LAVA, ST_VINE, ST_CANDY, ST_SPACE, ST_GEM };
enum { G_GRASS=0, G_SAND, G_SNOW, G_LAVA, G_CANDY, G_SPACE, G_GOLD };

typedef struct {
  const char *n; u32 gr, sea, ed; int dec[5];
  u32 cb, cl, cm, ck; int cst;
  int gt; u32 s1, s2, pd, pt;
} Map;

static const Map MAPS[NMAPS] = {
 {"Île",      0x3f9a46,0x1b5a9c,0xe9d28f,{D_PALM,D_PALM,D_ROCK,D_BUSH,D_CACTUS},   0xa05a1c,0xc4731f,0xffd41f,0xffe65c,ST_WOOD,  G_GRASS,0x62c167,0x2d7a35,0x2f86d6,0xd8c48a},
 {"Désert",   0xd9b45a,0x8a6a2a,0xf3e2a0,{D_CACTUS,D_CACTUS,D_ROCK,D_DUNE,D_CAMEL},0xb88a4a,0xd6a85c,0x7a5a2a,0xffe08a,ST_BRICK, G_SAND, 0xf0d488,0xb88f3c,0x3aa6c9,0xc9a05a},
 {"Neige",    0xe6f1f8,0x2c6f9c,0xffffff,{D_PINE,D_PINE,D_SNOWMAN,D_ROCK,D_FLAKE},0x7ec8f0,0xa8e0ff,0xe8f8ff,0xffffff,ST_ICE,   G_SNOW, 0xffffff,0xb9d4e6,0x8fd3f5,0xcfe3ee},
 {"Volcan",   0x4a3535,0xc2410c,0xff7a1a,{D_VOLCANO,D_ROCK,D_FIRE,D_SKULL,D_ROCK},0x2a2a33,0x3a3a46,0xff6a1a,0xffb02e,ST_LAVA,  G_LAVA, 0x6a4a4a,0x2a1a1a,0xff6a1a,0x5a4040},
 {"Jungle",   0x1f7a3a,0x0f4a5a,0x6b4a1a,{D_PALM,D_BUSH,D_BUSH,D_PARROT,D_FLOWER},0x3d6b2a,0x4f8a35,0xc9a227,0xffe65c,ST_VINE,  G_GRASS,0x3fae5a,0x0f5a28,0x2a8aa0,0x8a6a3a},
 {"Bonbons",  0xff9ac8,0xb57bff,0xffffff,{D_LOLLI,D_CANDY,D_CUPCAKE,D_DONUT,D_ICECREAM},0xff5fa2,0xff8cc0,0xffffff,0xffe65c,ST_CANDY, G_CANDY,0xffd1e6,0xff5fa2,0xb57bff,0xffe9f4},
 {"Espace",   0x241a4a,0x080520,0x7a5cff,{D_STAR,D_PLANET,D_COMET,D_UFO,D_MOON},   0x4a3a9a,0x6a52d0,0x48e0ff,0x9bf0ff,ST_SPACE, G_SPACE,0xffffff,0x5a45a0,0x0d0a2a,0x3a2a7a},
 {"Trésor d'or",0xd4a017,0x6a4a00,0xfff1a0,{D_GEM,D_CROWN,D_BAG,D_TROPHY,D_GEM},   0xe0a800,0xffd41f,0xfff1a0,0xff3d7f,ST_GEM,   G_GOLD, 0xfff1a0,0xa8780a,0x3ee0ff,0xf0c43a}
};

static const u32 LB[NMAPS][5] = {
 {0x1b2a85,0xff9a6a,0xfff2a8,0x2a6b4a,0x1f5a3a},
 {0xff9a4a,0xffe0a0,0xffffff,0xd9a050,0xc08838},
 {0x1a3a6a,0xa8d8f0,0xffffff,0xdff0fa,0xbcd8ea},
 {0x2a0a0a,0xc2410c,0xff7a1a,0x3a2020,0x2a1414},
 {0x0f3a2a,0x4ac27a,0xe8ffb0,0x1f7a3a,0x0f5a28},
 {0xff9ac8,0xffe0f0,0xffffff,0xff7ab8,0xb57bff},
 {0x05031a,0x3a1a7a,0xc8a8ff,0x3a2a7a,0x241a4a},
 {0x6a4a00,0xffd41f,0xfff1a0,0xe0a800,0xb88400}
};

typedef struct {
  u32 magic, ver;
  s64 coins;
  s32 lvl, rb, map, eq;
  u32 skins;
  s64 freeUntil;
  s32 mute, hideAnim;
  u32 hide;
} Save;

#define SAVE_MAGIC 0x50434833u
static Save S;
static int saveDirty = 0;

static int hasSkin(int i) { return (S.skins >> i) & 1; }
static int isFree(void) { return S.freeUntil > (s64)time(NULL); }
static double luckv(void) { return pow(1.1, S.lvl) * SKINS[S.eq].ch; }
static s64 rbCost(void) { return (s64)RBBASE << S.rb; }
static s64 upCost(void) { return (s64)floor(BASEC * pow(1.1, S.lvl) + 0.5); }

static void fmtn(char *o, size_t n, s64 v) {
  char t[32]; int l, i, j = 0;
  snprintf(t, sizeof t, "%lld", (long long)(v < 0 ? -v : v));
  l = (int)strlen(t);
  if (v < 0 && j < (int)n - 1) o[j++] = '-';
  for (i = 0; i < l && j < (int)n - 2; i++) {
    if (i && (l - i) % 3 == 0) o[j++] = ' ';
    o[j++] = t[i];
  }
  o[j] = 0;
}
static void fmtf(char *o, size_t n, double v, int dec) {
  char *p; snprintf(o, n, "%.*f", dec, v);
  for (p = o; *p; p++) if (*p == '.') *p = ',';
}
static void pctf(char *o, size_t n, double p) {
  double v = p * 100; char t[32];
  snprintf(t, sizeof t, v >= 1 ? "%.1f" : (v >= .01 ? "%.2f" : "%.5f"), v);
  { char *q; for (q = t; *q; q++) if (*q == '.') *q = ','; }
  snprintf(o, n, "%s %%", t);
}
static void oddtxt(char *o, size_t n, double p) {
  char t[32];
  if (p >= .5) { pctf(o, n, p); return; }
  fmtn(t, sizeof t, (s64)floor(1.0 / p + 0.5));
  snprintf(o, n, "1 / %s", t);
}
static void bonus(const Skin *s, char *o, size_t n) {
  char a[16], b[64] = "";
  o[0] = 0;
  if (s->cm > 1) { fmtf(a, sizeof a, s->cm, (s->cm == (int)s->cm) ? 0 : 1); snprintf(b, sizeof b, "×%s pièces ", a); strncat(o, b, n - strlen(o) - 1); }
  if (s->ch > 1) { fmtf(a, sizeof a, s->ch, (s->ch == (int)s->ch) ? 0 : 1); snprintf(b, sizeof b, "×%s chance ", a); strncat(o, b, n - strlen(o) - 1); }
  if (s->spd > 1) { fmtf(a, sizeof a, s->spd, (s->spd * 4 == (int)(s->spd * 4) && s->spd * 2 != (int)(s->spd * 2)) ? 2 : 1); snprintf(b, sizeof b, "×%s vitesse", a); strncat(o, b, n - strlen(o) - 1); }
  if (!o[0]) snprintf(o, n, "Tenue");
}

typedef struct { double p[NODDS]; double common; } OddsCalc;
static OddsCalc calcOdds(double L) {
  OddsCalc r; double raw[NODDS], s = 0, k, sum = 0; int i;
  for (i = 0; i < NODDS; i++) raw[i] = ODDS[i].p * (i < 2 ? (L < 8 ? L : 8) : L);
  for (i = 2; i < NODDS; i++) s += raw[i];
  k = s > LCAP ? LCAP / s : 1;
  for (i = 0; i < NODDS; i++) { r.p[i] = i < 2 ? raw[i] : raw[i] * k; sum += r.p[i]; }
  r.common = sum < 1 ? 1 - sum : 0;
  return r;
}
#endif
