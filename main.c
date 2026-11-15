// Pirate Chest 3DS - jeu complet (.3dsx)
// Haut = gameplay / scene.  Bas = menus, minimap, bouton OUVRIR (tactile).
#include "common.h"
#include "audio.h"
#include "draw.h"

enum { SC_LOBBY = 0, SC_LOCKER, SC_SHOP, SC_GAME };
enum { OV_NONE = 0, OV_MENU, OV_SET, OV_END };

#define TOPW 400
#define TOPH 240
#define BOTW 320

static C3D_RenderTarget *gTop, *gBot;
static int scr = SC_LOBBY, ovl = OV_NONE, ovFrom = OV_NONE, quitReq = 0;
static u32 kD, kH;
static touchPosition tp;
static circlePosition cp;
static int curLock = 0, curShop = 0, topLock = 0, topShop = 0, curSet = 0, curMenu = 0;
static int rbArm = 0; static double rbArmT = 0;
static char msgTxt[64]; static double msgT = 0;
static double wall = 0, lastSaveT = 0;

static float rand01(void) { return rand() / (RAND_MAX + 1.0f); }
static void setMsg(const char *s) { snprintf(msgTxt, sizeof msgTxt, "%s", s); msgT = 1.6; }
#define DIRTY() (saveDirty = 1)

static void saveFile(void) {
  FILE *f;
  mkdir("sdmc:/3ds", 0777); mkdir("sdmc:/3ds/PirateChest", 0777);
  f = fopen("sdmc:/3ds/PirateChest/save.bin", "wb");
  if (f) { S.magic = SAVE_MAGIC; S.ver = 1; fwrite(&S, sizeof S, 1, f); fclose(f); }
  saveDirty = 0;
}
static void loadFile(void) {
  FILE *f; Save t;
  memset(&S, 0, sizeof S); S.skins = 1;
  f = fopen("sdmc:/3ds/PirateChest/save.bin", "rb");
  if (f) { if (fread(&t, sizeof t, 1, f) == 1 && t.magic == SAVE_MAGIC) S = t; fclose(f); }
  S.skins |= 1;
  if (S.eq < 0 || S.eq >= NSKINS || !hasSkin(S.eq)) S.eq = 0;
  if (S.rb < 0) S.rb = 0;
  if (S.rb > NMAPS - 1) S.rb = NMAPS - 1;
  if (S.map < 0) S.map = 0;
  if (S.map > S.rb) S.map = S.rb;
  if (S.lvl < 0) S.lvl = 0;
  if (S.lvl > MAXLVL) S.lvl = MAXLVL;
  if (S.coins < 0) S.coins = 0;
}

typedef struct { float x, y; double o; int rare; } Chest;
typedef struct { float x, y; int k; } Decor;
typedef struct { float x, y, vx, vy, g, l, m, r; u32 c; } Part;
typedef struct { int on; float sx, sy, tx, ty, mx, my, t, dur, delay; int boost; s64 v; } Fly;
typedef struct { float x, y, t; char s[24]; } FText;
typedef struct { float x, y, vx, vy, t, m, sz; u32 c; int on; } Conf;

static struct { int n; s64 gain; int done; int log[7]; int fly, pend; double boost; double dl; double finT; } G;
static struct { float x, y, f, fs, t, mv, w; int sc; } P;
static Chest chests[MAXC];
static Decor decor[64]; static int ndecor;
static Part parts[300];
static Fly flys[64];
static FText fts[16];
static Conf confs[48];
static float spkx[320], spky[320], pondx[2], pondy[2]; static int npond;
#define PN 101
static float pathx[PN], pathy[PN];
static const Map *curMap = &MAPS[0];
static double gameT = 0;
static float shk = 0, flashA = 0; static u32 flashC = 0;
static float bannerT = 0, ringT = 0, ringX = 0, ringY = 0, toastT = 0; static char toastS[64];
static int gid = 0;
static float camX = 0, camY = 0;

static struct { int on, big, rev; double t, end; char txt[64]; } LK;
static struct { int on, m, phase, ok, chest, gid; double t; float th; char txt[48], sub[32]; } W;

static float cmul(void) { return powf(2, (float)S.rb) * SKINS[S.eq].cm * (G.boost > 0 ? 2.0f : 1.0f); }

static void spawn(float x, float y, int n, u32 col, float sp, float g, float life, float r) {
  int i, k = 0;
  for (i = 0; i < n; i++) {
    float a = rnd(0, 6.283f), s = rnd(.3f, 1) * sp; Part *p = NULL;
    for (; k < 300; k++) if (parts[k].l <= 0) { p = &parts[k++]; break; }
    if (!p) return;
    p->x = x; p->y = y; p->vx = cosf(a) * s; p->vy = sinf(a) * s - sp * .5f; p->g = g;
    p->m = life; p->l = life * rnd(.7f, 1); p->r = rnd(r * .6f, r); p->c = col;
  }
}
static void shake(float d) { if (d > shk) shk = d; }
static void flash(u32 col) { flashC = col; flashA = .5f; }

static int cmpDecor(const void *a, const void *b) { float d = ((const Decor *)a)->y - ((const Decor *)b)->y; return d < 0 ? -1 : (d > 0); }

static void startGame(void) {
  int i, tries = 0, n;
  sfxPlay(SFX_START);
  scr = SC_GAME; ovl = OV_NONE; musicSet(1);
  curMap = &MAPS[S.map < S.rb ? S.map : S.rb];
  memset(&G, 0, sizeof G); G.finT = -1; gid++; gameT = 0;
  memset(parts, 0, sizeof parts); memset(flys, 0, sizeof flys); memset(fts, 0, sizeof fts); memset(confs, 0, sizeof confs);
  P.x = WW / 2; P.y = WH / 2; P.f = 1; P.fs = 1; P.t = 0; P.mv = 0; P.w = 0; P.sc = 0;
  n = 0;
  while (n < MAXC && tries++ < 5000) {
    float x = rnd(70, WW - 70), y = rnd(110, WH - 70); int ok = hypotf(x - P.x, y - P.y) > 140;
    for (i = 0; i < n; i++) if (hypotf(x - chests[i].x, y - chests[i].y) < 115) ok = 0;
    if (ok) { chests[n].x = x; chests[n].y = y; chests[n].o = 0; chests[n].rare = rand01() < RPR; n++; }
  }
  while (n < MAXC) { chests[n].x = 80 + n * 30; chests[n].y = 150; chests[n].o = 0; chests[n].rare = 0; n++; }
  ndecor = 0;
  for (i = 0; i < 60; i++) {
    float x = rnd(30, WW - 30), y = rnd(30, WH - 30); int ok = 1, j;
    for (j = 0; j < MAXC; j++) if (hypotf(chests[j].x - x, chests[j].y - y) < 70) ok = 0;
    if (hypotf(x - P.x, y - P.y) < 90) ok = 0;
    if (ok && ndecor < 64) { decor[ndecor].x = x; decor[ndecor].y = y; decor[ndecor].k = curMap->dec[i % 5]; ndecor++; }
  }
  qsort(decor, ndecor, sizeof(Decor), cmpDecor);
  for (i = 0; i < 320; i++) { spkx[i] = rnd(20, WW - 20); spky[i] = rnd(20, WH - 10); }
  npond = 0;
  { int k, t2;
    for (k = 0; k < 2; k++) for (t2 = 0; t2 < 40; t2++) {
      float x = rnd(120, WW - 120), y = rnd(140, WH - 100); int ok = 1, j;
      for (j = 0; j < MAXC; j++) if (hypotf(chests[j].x - x, chests[j].y - y) < 130) ok = 0;
      if (ok) { pondx[npond] = x; pondy[npond] = y; npond++; break; }
    } }
  {
    int h = PN / 2;
    for (i = 0; i <= 50; i++) {
      float t = i / 50.0f, u = 1 - t;
      float x0 = -20, y0 = WH * .7f, x1 = WW * .25f, y1 = WH * .95f, x2 = WW * .4f, y2 = WH * .2f, x3 = WW * .6f, y3 = WH * .5f;
      pathx[i] = u * u * u * x0 + 3 * u * u * t * x1 + 3 * u * t * t * x2 + t * t * t * x3;
      pathy[i] = u * u * u * y0 + 3 * u * u * t * y1 + 3 * u * t * t * y2 + t * t * t * y3;
    }
    for (i = 1; i <= 50; i++) {
      float t = i / 50.0f, u = 1 - t;
      float x0 = WW * .6f, y0 = WH * .5f, x1 = WW * .75f, y1 = WH * .7f, x2 = WW * .85f, y2 = WH * .2f, x3 = WW + 20, y3 = WH * .3f;
      pathx[h + i] = u * u * u * x0 + 3 * u * u * t * x1 + 3 * u * t * t * x2 + t * t * t * x3;
      pathy[h + i] = u * u * u * y0 + 3 * u * u * t * y1 + 3 * u * t * t * y2 + t * t * t * y3;
    }
  }
  LK.on = 0; W.on = 0; flashA = 0; shk = 0; bannerT = 0; ringT = 0; toastT = 0;
}

static void leaveGame(void) { LK.on = 0; W.on = 0; scr = SC_LOBBY; ovl = OV_NONE; musicSet(0); DIRTY(); }

static int nearChest(void) {
  int i, b = -1; float bd = 85;
  for (i = 0; i < MAXC; i++) {
    float d = hypotf(chests[i].x - P.x, chests[i].y - P.y);
    if (chests[i].o == 0 && d < bd) { bd = d; b = i; }
  }
  return b;
}
static void flyOne(float wx, float wy, s64 v, float delay, int boost) {
  int i;
  for (i = 0; i < 64; i++) if (!flys[i].on) {
    Fly *f = &flys[i]; f->on = 1; f->boost = boost; f->v = v; f->t = 0; f->delay = delay; f->dur = rnd(.8f, 1.1f);
    f->sx = wx - camX; f->sy = wy - camY - 20; f->tx = 372; f->ty = 18;
    f->mx = (f->sx + f->tx) / 2 + rnd(-70, 70); f->my = fminf(f->sy, f->ty) - rnd(25, 90);
    G.fly++; return;
  }
  if (!boost) { s64 k = (s64)llround(v * cmul()); S.coins += k; G.gain += k; DIRTY(); } else G.boost = 10;
}
static void flyCoins(float wx, float wy, s64 base) {
  int n = base >= 5000 ? 14 : base >= 500 ? 10 : base >= 75 ? 7 : base >= 10 ? 4 : 1, i;
  s64 v = base / n, r = base - v * n;
  for (i = 0; i < n; i++) flyOne(wx, wy, v + (i ? 0 : r), i * .07f, 0);
}
static void addFT(float x, float y, const char *s) {
  int i;
  for (i = 0; i < 16; i++) if (fts[i].t <= 0) { fts[i].x = x; fts[i].y = y; fts[i].t = .0001f; snprintf(fts[i].s, sizeof fts[i].s, "%s", s); return; }
}
static void toast(const char *s) { snprintf(toastS, sizeof toastS, "%s", s); toastT = .9f; }
static double luckyStart(const char *txt, int big) {
  LK.on = 1; LK.big = big; LK.rev = 0; LK.t = 0; LK.end = big ? 2.6 : 1.8; snprintf(LK.txt, sizeof LK.txt, "%s", txt);
  memset(confs, 0, sizeof confs);
  sfxPlay(SFX_SHAKE);
  return LK.end * 1000.0;
}
static void luckyReveal(void) {
  int i, n = LK.big ? 26 : 12; static const u32 cc[5] = {0xffd41f, 0xffffff, 0xffe65c, 0x3ee0ff, 0xff7ab8};
  LK.rev = 1; sfxPlay(LK.big ? SFX_JACKPOT : SFX_LUCKY);
  for (i = 0; i < n; i++) {
    Conf *c = &confs[i]; c->on = 1; c->x = TOPW / 2; c->y = 90; c->vx = rnd(-170, 170); c->vy = rnd(-190, 60);
    c->t = -rnd(0, .25f); c->m = 1.6f; c->sz = rnd(3, 6); c->c = cc[i % 5];
  }
  if (LK.big) flash(0xffffff);
}

static int rollOdds(double L) {
  OddsCalc c = calcOdds(L); double r = rand01(), a = 0; int i;
  for (i = 0; i < NODDS; i++) { a += c.p[i]; if (r < a) return i; }
  return 6;
}
static void chkFinish(void) {
  if (G.n >= MAXC && !G.pend && !G.fly && !G.done) { G.done = 1; G.finT = G.dl > 0 ? G.dl / 1000.0 : .7; }
}
static void reward(int ci, int m) {
  int t = rollOdds(luckv() * m), big = 0; s64 base = 0; double k = cmul();
  char txt[64], nb[32]; Chest *c = &chests[ci];
  if (t == 6) base = 1; else if (t >= 2) base = ODDV[t];
  if (base) {
    fmtn(nb, sizeof nb, (s64)llround(base * k));
    snprintf(txt, sizeof txt, "+%s %s%s", nb, base * k > 1 ? "pièces" : "pièce", base >= 5000 ? " !!!" : base >= 500 ? " !" : "");
    big = base >= 500;
  } else if (t == 0) {
    S.freeUntil = (s64)time(NULL) + 300; snprintf(txt, sizeof txt, "JETON BOUTIQUE GRATUITE 5 MIN !"); big = 1;
  } else {
    big = 1;
    if (!hasSkin(GOLDSK)) { S.skins |= 1u << GOLDSK; snprintf(txt, sizeof txt, "SKIN D'OR HEBDO !"); }
    else { base = 1000; fmtn(nb, sizeof nb, (s64)llround(1000 * k)); snprintf(txt, sizeof txt, "SKIN D'OR (doublon) +%s", nb); }
  }
  if (base) {
    char ft[24]; fmtn(nb, sizeof nb, (s64)llround(base * k)); snprintf(ft, sizeof ft, "+%s", nb);
    flyCoins(c->x, c->y, base); addFT(c->x, c->y - 42, ft);
  }
  if (big && !(t < NODDS && ((S.hide >> t) & 1))) shake(.4f);
  if (rand01() < BPR) { flyOne(c->x, c->y, 0, .25f, 1); strncat(txt, " +BOOST x2", sizeof txt - strlen(txt) - 1); }
  G.log[t]++; DIRTY();
  {
    double delay = 700; int hidden = (t < NODDS) && ((S.hide >> t) & 1);
    spawn(c->x, c->y - 6, (t == 6 || hidden) ? 6 : big ? 28 : 14, RGB(0xffd41f), t == 6 ? 90 : 160, 260, .8f, 4.5f);
    if (t == 6 || hidden) { sfxPlay(t == 6 ? SFX_COIN : SFX_LUCKY); toast(txt); }
    else delay = luckyStart(txt, big) + 100;
    if (delay > G.dl) G.dl = delay;
  }
  chkFinish();
}

static void startSpin(void) {
  W.ok = rand01() < WPR; W.th = 720 + (W.ok ? -rnd(.6f, 6.6f) : rnd(20, 350)); W.phase = 0; W.t = 0;
  snprintf(W.txt, sizeof W.txt, "×%d ?", W.m * 2); W.sub[0] = 0; sfxPlay(SFX_SHAKE);
}
static void finishWheel(void) { int ci = W.chest; G.pend--; W.on = 0; reward(ci, W.m); }
static void startWheel(int ci) {
  if (S.hideAnim) { int q = 2; while (rand01() < WPR) q *= 2; W.chest = ci; W.m = q; finishWheel(); return; }
  W.on = 1; W.m = 2; W.chest = ci; W.gid = gid; startSpin();
}
static void rareFx(Chest *c) {
  shake(.45f); flash(0xb66bff); sfxPlay(SFX_JACKPOT);
  spawn(c->x, c->y - 10, 40, RGB(0xc46bff), 220, 260, .9f, 5); spawn(c->x, c->y - 10, 20, RGB(0xffffff), 160, 200, .7f, 3);
  bannerT = 1.6f; ringT = .9f; ringX = c->x - camX; ringY = c->y - camY;
}
static void tryOpen(void) {
  int ci; Chest *c;
  if (scr != SC_GAME || ovl != OV_NONE || G.done) return;
  ci = nearChest(); if (ci < 0) return;
  c = &chests[ci]; c->o = gameT > 0 ? gameT : .0001; G.n++; sfxPlay(SFX_OPEN);
  if (c->rare) { G.pend++; if (!S.hideAnim) rareFx(c); startWheel(ci); }
  else reward(ci, 1);
}

static void gameUpdate(float dt) {
  int i; float mx = 0, my = 0, m, SP;
  gameT += dt;
  if (fabsf((float)cp.dx) > 18 || fabsf((float)cp.dy) > 18) { mx = cp.dx / 150.0f; my = -cp.dy / 150.0f; }
  if (kH & KEY_DRIGHT) mx += 1;
  if (kH & KEY_DLEFT) mx -= 1;
  if (kH & KEY_DDOWN) my += 1;
  if (kH & KEY_DUP) my -= 1;
  m = hypotf(mx, my); if (m > 1) { mx /= m; my /= m; m = 1; }
  P.mv = m > .1f ? 1 : 0;
  SP = 200 * SKINS[S.eq].spd;
  if (P.mv) {
    int s2;
    P.x = clampf(P.x + mx * SP * dt, 25, WW - 25); P.y = clampf(P.y + my * SP * dt, 60, WH - 15);
    if (fabsf(mx) > .2f) P.f = mx > 0 ? 1.0f : -1.0f;
    P.t += dt * 11 * (SP / 200);
    s2 = (int)floorf(P.t / PI_F);
    if (s2 != P.sc) { P.sc = s2; sfxPlay(SFX_STEP); spawn(P.x, P.y, 2, RGBA(0xe6dcb4, 200), 22, -10, .45f, 5); }
  }
  P.w += ((P.mv ? fmaxf(.6f, m) : 0) - P.w) * fminf(1, dt * 10); P.fs += (P.f - P.fs) * fminf(1, dt * 14);
  for (i = 0; i < 300; i++) { Part *p = &parts[i]; if (p->l > 0) { p->x += p->vx * dt; p->y += p->vy * dt; p->vy += p->g * dt; p->l -= dt; } }
  for (i = 0; i < 16; i++) if (fts[i].t > 0) { fts[i].t += dt; if (fts[i].t > 1.2f) fts[i].t = 0; }
  if (shk > 0) shk -= dt;
  if (flashA > 0) flashA -= dt;
  if (bannerT > 0) bannerT -= dt;
  if (ringT > 0) ringT -= dt;
  if (toastT > 0) toastT -= dt;
  if (G.boost > 0) G.boost -= dt;
  for (i = 0; i < 64; i++) {
    Fly *f = &flys[i]; if (!f->on) continue;
    if (f->delay > 0) { f->delay -= dt; continue; }
    f->t += dt;
    if (f->t >= f->dur) {
      f->on = 0; G.fly--;
      if (f->boost) { G.boost = 10; sfxPlay(SFX_LUCKY); }
      else { s64 k = (s64)llround(f->v * cmul()); S.coins += k; G.gain += k; sfxPlay(SFX_COIN); DIRTY(); }
    }
  }
  if (LK.on) {
    LK.t += dt;
    if (!LK.rev && LK.t >= .65) luckyReveal();
    if (LK.t >= LK.end) LK.on = 0;
    for (i = 0; i < 48; i++) if (confs[i].on) {
      Conf *c = &confs[i]; c->t += dt;
      if (c->t > 0) { c->x += c->vx * dt; c->y += c->vy * dt; c->vy += 260 * dt; if (c->t > c->m) c->on = 0; }
    }
  }
  if (W.on) {
    W.t += dt;
    if (W.phase == 0 && W.t >= .52) {
      W.t = 0;
      if (W.ok) {
        W.m *= 2; snprintf(W.txt, sizeof W.txt, "×%d !!", W.m); snprintf(W.sub, sizeof W.sub, "TOUTES LES CHANCES");
        sfxPlay(SFX_JACKPOT); shake(.5f); flash(0xffd41f); spawn(P.x, P.y - 40, 30, RGB(0xffd41f), 240, 260, .9f, 5); W.phase = 2;
      } else {
        snprintf(W.txt, sizeof W.txt, "×%d", W.m); snprintf(W.sub, sizeof W.sub, "TOUTES LES CHANCES"); sfxPlay(SFX_LUCKY); W.phase = 3;
      }
    } else if (W.phase == 2 && W.t >= .55) startSpin();
    else if (W.phase == 3 && W.t >= .8) finishWheel();
  }
  chkFinish();
  if (G.finT >= 0) { G.finT -= dt; if (G.finT <= 0) { G.finT = -1; ovl = OV_END; sfxPlay(SFX_FINISH); DIRTY(); } }
}

static u32 darken(u32 c, float f) { return C2D_Color32((int)(((c >> 16) & 255) * f), (int)(((c >> 8) & 255) * f), (int)((c & 255) * f), 255); }
static int inView(float wx, float wy, float mg) { return wx > camX - mg && wx < camX + TOPW + mg && wy > camY - mg && wy < camY + TOPH + mg; }

static void drawSpeck(int gt, float x, float y, u32 c1, u32 c2, int i) {
  switch (gt) {
    case G_GRASS:
      dLine(x - 4, y, x - 5, y - 7, 2, i % 2 ? c1 : c2); dLine(x, y, x, y - 10, 2, i % 2 ? c1 : c2); dLine(x + 4, y, x + 5, y - 7, 2, i % 2 ? c1 : c2);
      if (i % 9 == 0) { static const u32 fc2[3] = {0xffffff, 0xff6a8a, 0xffe14d}; dCirc(x + 8, y - 4, 3, RGB(fc2[i % 3])); } break;
    case G_SAND: dLine(x - 12, y, x - 4, y - 6, 2, i % 2 ? c1 : c2); dLine(x - 4, y - 6, x + 6, y - 6, 2, i % 2 ? c1 : c2); dLine(x + 6, y - 6, x + 12, y, 2, i % 2 ? c1 : c2); break;
    case G_SNOW: dEll(x, y, 9, 4, i % 2 ? c1 : c2); break;
    case G_LAVA: dEll(x, y, 10, 6, c2); if (i % 4 == 0) { dLine(x - 14, y + 8, x, y + 2, 2, RGB(0xff7a1a)); dLine(x, y + 2, x + 12, y + 10, 2, RGB(0xff7a1a)); } break;
    case G_CANDY: { static const u32 cc[4] = {0xff5fa2, 0x7ad8ff, 0xffe14d, 0x9d6bff}; float a = (float)i; dLine(x - cosf(a) * 6, y - sinf(a) * 6, x + cosf(a) * 6, y + sinf(a) * 6, 4, RGB(cc[i % 4])); } break;
    case G_SPACE: { float z = i % 5 ? 2.0f : 4.0f; dRect(x, y, z, z, i % 3 ? RGB(c1) : RGB(0xffe65c)); } break;
    default: dCirc(x, y, 5, i % 2 ? c1 : c2); dCirc(x, y, 3.5f, i % 2 ? c1 : c2); break;
  }
}

static void drawWorld(float ox, float oy) {
  const Map *M = curMap; int i, j; float X0 = -camX + ox, Y0 = -camY + oy;
  u32 pt = RGB(M->pt), psh = darken(M->gr, .82f);
  dRect(X0, Y0, WW, WH, RGB(M->gr));
  for (i = (int)(camX / 80); i <= (int)((camX + TOPW) / 80) && i * 80 < WW; i++)
    for (j = (int)(camY / 80); j <= (int)((camY + TOPH) / 80) && j * 80 < WH; j++)
      if (i >= 0 && j >= 0 && (i + j) % 2 == 0) dRect(X0 + i * 80, Y0 + j * 80, fminf(80, (float)(WW - i * 80)), fminf(80, (float)(WH - j * 80)), RGBA(0, 15));
  for (j = 0; j < 2; j++) {
    for (i = 0; i + 1 < PN; i++) {
      float ax = pathx[i] + X0, ay = pathy[i] + Y0, bx = pathx[i + 1] + X0, by = pathy[i + 1] + Y0;
      if (ax < -70 && bx < -70) continue;
      if (ax > TOPW + 70 && bx > TOPW + 70) continue;
      if (ay < -70 && by < -70) continue;
      if (ay > TOPH + 70 && by > TOPH + 70) continue;
      dLine(ax, ay, bx, by, j ? 50 : 58, j ? pt : psh); dCirc(ax, ay, j ? 25 : 29, j ? pt : psh);
    }
  }
  for (i = 0; i < 320; i++) if (inView(spkx[i], spky[i], 20)) drawSpeck(M->gt, spkx[i] + X0, spky[i] + Y0, M->s1, M->s2, i);
  for (i = 0; i < npond; i++) if (inView(pondx[i], pondy[i], 100)) {
    float x = pondx[i] + X0, y = pondy[i] + Y0;
    dEll(x, y + 5, 88, 50, RGBA(0, 64)); dEll(x, y, 80, 44, RGB(M->pd)); dEll(x - 6, y - 14, 50, 6, RGBA(0xffffff, 110));
  }
  for (i = 0; i < MAXC; i++) if (inView(chests[i].x, chests[i].y, 60)) {
    dEll(chests[i].x + X0, chests[i].y + Y0 + 20, 40, 13, RGBA(0, 64)); dEll(chests[i].x + X0, chests[i].y + Y0 + 19, 34, 10, RGBA(0xffffff, 40));
  }
  { u32 ed = RGB(M->ed);
    dRect(X0 - 15, Y0 - 15, WW + 30, 30, ed); dRect(X0 - 15, Y0 + WH - 15, WW + 30, 30, ed);
    dRect(X0 - 15, Y0 - 15, 30, WH + 30, ed); dRect(X0 + WW - 15, Y0 - 15, 30, WH + 30, ed);
    dOut(X0 + 16, Y0 + 16, WW - 32, WH - 32, 4, RGBA(0xffffff, 180)); }
}

static void drawGameTop(void) {
  int i, n = nearChest(); float ox = 0, oy = 0; char b1[48], b2[48];
  const Map *M = curMap;
  camX = clampf(P.x - TOPW / 2, 0, WW - TOPW); camY = clampf(P.y - TOPH / 2, 0, WH - TOPH);
  if (shk > 0) { ox = rnd(-1, 1) * shk * 14; oy = rnd(-1, 1) * shk * 14; }
  C2D_TargetClear(gTop, RGB(M->sea)); C2D_SceneBegin(gTop);
  drawWorld(ox, oy);
  for (i = 0; i < MAXC; i++) if (inView(chests[i].x, chests[i].y, 70)) {
    Chest *c = &chests[i]; float pr = c->o ? fminf(1, (float)((gameT - c->o) / .36)) : 0, pop = (c->o && pr < 1) ? sinf(pr * PI_F) * .14f : 0;
    drawChest(M, c->x - camX + ox, c->y - camY + oy, 1, pop, pr, c->o != 0, n == i, c->rare, gNow * 1000);
  }
  { int di = 0, pdone = 0;
    while (di < ndecor || !pdone) {
      if (!pdone && (di >= ndecor || P.y <= decor[di].y)) {
        dEll(P.x - camX + ox, P.y - camY + oy, 18, 6, RGBA(0, 75));
        drawFig(&SKINS[S.eq], P.x - camX + ox, P.y - camY + oy, .42f, P.fs, (float)gNow, P.t, P.w, 0, 0); pdone = 1;
      } else {
        if (inView(decor[di].x, decor[di].y, 70)) drawDecor(decor[di].k, decor[di].x - camX + ox, decor[di].y - camY + oy, gNow);
        di++;
      }
    } }
  for (i = 0; i < 300; i++) if (parts[i].l > 0) {
    Part *p = &parts[i]; float a = clampf(p->l / p->m, 0, 1), r = p->r * (p->g < 0 ? (2 - p->l / p->m) : 1);
    dCirc(p->x - camX + ox, p->y - camY + oy, r, (p->c & 0x00ffffff) | ((u32)(a * 255) << 24));
  }
  for (i = 0; i < 16; i++) if (fts[i].t > 0) {
    float a = clampf((1.2f - fts[i].t) * 2.5f, 0, 1), z = 1 + fmaxf(0, .3f - fts[i].t) * 2.5f;
    dTextO(fts[i].s, fts[i].x - camX + ox, fts[i].y - camY + oy - fts[i].t * 55 - 10, .6f * z,
           (RGB(0xffd41f) & 0x00ffffff) | ((u32)(a * 255) << 24), (RGB(0x3a1500) & 0x00ffffff) | ((u32)(a * 255) << 24), 1);
  }
  if (LK.on) {
    float cxp = TOPW / 2, cyp = 92, t = (float)LK.t; int out = LK.t > LK.end - .4;
    float fo = out ? clampf((float)((LK.end - LK.t) / .4), 0, 1) : 1, yo = out ? -(1 - fo) * 20 : 0;
    if (LK.big && LK.rev) {
      for (i = 0; i < 12; i++) {
        float a0 = (float)gNow * 1.0f + i * (2 * PI_F / 12), a1 = a0 + .22f;
        dTri(cxp, cyp + yo, cxp + cosf(a0) * 260, cyp + yo + sinf(a0) * 260, cxp + cosf(a1) * 260, cyp + yo + sinf(a1) * 260, RGBA(0xffd41f, (int)(95 * fo)));
      }
    }
    if (!LK.rev) {
      float sc = clampf(t / .2f, .3f, 1), dx = sinf(t * 40) * 7 * (1 - t / .65f);
      drawChest(M, cxp + dx, cyp + 30 + yo, 1.6f * sc, 0, 0, 0, 0, 0, gNow * 1000);
    } else {
      float k = clampf((t - .65f) / .35f, 0, 1), pop = sinf(k * PI_F) * .2f;
      drawChest(M, cxp, cyp + 30 + yo, 1.6f, pop, 1, 1, 0, 0, gNow * 1000);
      { float cs = clampf((t - .65f) / .5f, 0, 1); float e = 1 - powf(1 - cs, 3); float sc = .2f + .8f * e;
        float sz = LK.big ? .9f : .75f, w = textW(LK.txt, sz * sc) + 30 * sc, h = 34 * sc, x = cxp - w / 2, y = cyp + 62 + yo;
        u32 al = (u32)(255 * fo);
        dRect(x - 3 * sc, y - 3 * sc, w + 6 * sc, h + 6 * sc, (RGB(WHITE) & 0x00ffffff) | (al << 24));
        C2D_DrawRectangle(x, y, 0, w, h, (RGB(0xffd41f) & 0x00ffffff) | (al << 24), (RGB(0xffd41f) & 0x00ffffff) | (al << 24), (RGB(0xff3d7f) & 0x00ffffff) | (al << 24), (RGB(0xff3d7f) & 0x00ffffff) | (al << 24));
        dText(LK.txt, cxp, y + 2 * sc, sz * sc, (RGB(0x3a1500) & 0x00ffffff) | (al << 24), 1); }
    }
    for (i = 0; i < 48; i++) if (confs[i].on && confs[i].t > 0) {
      Conf *c = &confs[i]; float a = clampf(1 - c->t / c->m, 0, 1);
      dCirc(c->x, c->y, c->sz, (c->c & 0x00ffffff) | ((u32)(a * 255) << 24));
    }
  }
  if (W.on) {
    float cxp = TOPW / 2, cyp = 100, R = 46, ph = W.phase == 0 ? clampf((float)(W.t / .5), 0, 1) : 1;
    float ang = W.th * (1 - powf(1 - ph, 3)) * PI_F / 180.0f; int s;
    dCirc(cxp, cyp, R + 5, RGB(WHITE));
    for (s = 0; s < 60; s++) {
      float a0 = ang + s * 6 * PI_F / 180, a1 = a0 + 6 * PI_F / 180; u32 c = s == 0 ? RGB(0xffd41f) : (s < 30 ? RGB(0x33334d) : RGB(0x4a4a6a));
      if (s == 0) { float aa = ang + 7.2f * PI_F / 180; dTri(cxp, cyp, cxp + sinf(ang) * R, cyp - cosf(ang) * R, cxp + sinf(aa) * R, cyp - cosf(aa) * R, RGB(0xffd41f)); continue; }
      if (s == 1) a0 = ang + 7.2f * PI_F / 180;
      dTri(cxp, cyp, cxp + sinf(a0) * R, cyp - cosf(a0) * R, cxp + sinf(a1) * R, cyp - cosf(a1) * R, c);
    }
    dTri(cxp - 9, cyp - R - 12, cxp + 9, cyp - R - 12, cxp, cyp - R + 8, RGB(WHITE));
    dTextO(W.txt, cxp, cyp + R + 8, .8f, RGB(0xffd41f), RGB(0), 1);
    if (W.sub[0]) dTextO(W.sub, cxp, cyp + R + 34, .4f, RGB(WHITE), RGB(0), 1);
  }
  if (ringT > 0) { float u = 1 - ringT / .9f; dRing(ringX, ringY, 8 + u * 260, 8 + u * 260, 5, RGBA(0xe0a8ff, (int)(220 * (1 - u))), 40); }
  if (bannerT > 0) {
    float u = 1 - bannerT / 1.6f, sc = u < .2f ? .3f + 1.1f * (u / .2f) : (u < .3f ? 1.4f - .3f * ((u - .2f) / .1f) : 1), al = u > .8f ? (1 - u) / .2f : 1;
    dTextO("COFFRE RARE", TOPW / 2, 26 - (u > .8f ? (u - .8f) * 100 : 0), .9f * sc, C2D_Color32(240, 200, 255, (int)(255 * al)), C2D_Color32(58, 10, 106, (int)(255 * al)), 1);
    dTextO("×2 SUR TOUTES TES CHANCES", TOPW / 2, 58 - (u > .8f ? (u - .8f) * 100 : 0), .5f * sc, C2D_Color32(255, 255, 255, (int)(255 * al)), C2D_Color32(58, 10, 106, (int)(255 * al)), 1);
  }
  if (toastT > 0) {
    float u = 1 - toastT / .9f, sc = u < .2f ? .4f + .8f * (u / .2f) : 1, al = u > .8f ? (1 - u) / .2f : 1;
    dTextO(toastS, TOPW / 2, 70 - u * 30, .8f * sc, C2D_Color32(255, 212, 31, (int)(255 * al)), C2D_Color32(0, 0, 0, (int)(255 * al)), 1);
  }
  for (i = 0; i < 64; i++) if (flys[i].on && flys[i].delay <= 0) {
    Fly *f = &flys[i]; float u = clampf(f->t / f->dur, 0, 1), v = 1 - u;
    float x = v * v * f->sx + 2 * v * u * f->mx + u * u * f->tx, y = v * v * f->sy + 2 * v * u * f->my + u * u * f->ty, s = .55f + .75f * sinf(u * PI_F);
    if (f->boost) { dCirc(x, y, 11 * s, RGB(WHITE)); dCirc(x, y, 9 * s, RGB(0x7a2bd1)); dTextO("x2", x, y - 9 * s, .4f * s, RGB(WHITE), RGB(0x3a0a6a), 1); }
    else dCoin(x, y, 8 * s);
  }
  if (flashA > 0) dRect(0, 0, TOPW, TOPH, (flashC & 0x00ffffff) | ((u32)(clampf(flashA, 0, .6f) * 255) << 24));
  snprintf(b1, sizeof b1, "%d / %d", G.n, MAXC);
  dRect(6, 6, 74, 24, RGBA(0x0a0c1c, 205)); dText(b1, 43, 5, .6f, RGB(WHITE), 1);
  fmtn(b2, sizeof b2, S.coins);
  { float w = textW(b2, .6f) + 38; dRect(TOPW - 6 - w, 6, w, 24, RGBA(0x0a0c1c, 205)); dCoin(TOPW - w + 2, 18, 8); dText(b2, TOPW - 12, 5, .6f, RGB(WHITE), 2); }
  if (G.boost > 0) { snprintf(b1, sizeof b1, "BOOST x2  %ds", (int)ceil(G.boost)); dRect(TOPW / 2 - 52, 6, 104, 24, RGBA(0x7a2bd1, 225)); dText(b1, TOPW / 2, 5, .5f, RGB(WHITE), 1); }
}

static void btnOpenRect(float *x, float *y, float *w, float *h) { *x = 12; *y = 156; *w = 296; *h = 76; }

static void drawGameBottom(void) {
  int i, near = nearChest(); float bx, by, bw, bh, mx0 = 70, my0 = 36, mw = 180, mh = 119, sx = mw / WW; char b[64], nb[32];
  C2D_TargetClear(gBot, RGB(0x0e1630)); C2D_SceneBegin(gBot);
  dGrad(0, 0, BOTW, 240, RGB(0x1b2a85), RGB(0x0a1442));
  dRect(0, 0, BOTW, 26, RGBA(0x0a0c1c, 225));
  snprintf(b, sizeof b, "%d / %d coffres", G.n, MAXC); dText(b, 8, 1, .5f, RGB(WHITE), 0);
  fmtn(nb, sizeof nb, G.gain); snprintf(b, sizeof b, "+%s", nb); dCoin(BOTW - 14, 13, 7); dText(b, BOTW - 26, 1, .5f, RGB(0xffd41f), 2);
  dText(curMap->n, BOTW / 2, 1, .5f, RGB(WHITE), 1);
  dRect(mx0 - 3, my0 - 3, mw + 6, mh + 6, RGB(0x0a0c1c)); dRect(mx0, my0, mw, mh, RGB(curMap->gr));
  for (i = 0; i < MAXC; i++) {
    u32 c = chests[i].o ? RGB(0x66666e) : (chests[i].rare ? RGB(0xc46bff) : RGB(0xffd41f));
    dCirc(mx0 + chests[i].x * sx, my0 + chests[i].y * sx, chests[i].o ? 2 : 3.2f, c);
  }
  dOut(mx0 + camX * sx, my0 + camY * sx, TOPW * sx, TOPH * sx, 1, RGBA(0xffffff, 200));
  dCirc(mx0 + P.x * sx, my0 + P.y * sx, 3.6f, RGB(0xff3d7f)); dCirc(mx0 + P.x * sx, my0 + P.y * sx, 1.8f, RGB(WHITE));
  dText("ROND : bouger", 4, 40, .38f, RGB(WHITE), 0); dText("START : menu", 4, 56, .38f, RGB(WHITE), 0);
  dText("A : ouvrir", BOTW - 4, 40, .38f, RGB(WHITE), 2);
  if (G.boost > 0) { snprintf(b, sizeof b, "BOOST x2 %ds", (int)ceil(G.boost)); dRect(4, 100, 62, 18, RGB(0x7a2bd1)); dText(b, 35, 99, .32f, RGB(WHITE), 1); }
  btnOpenRect(&bx, &by, &bw, &bh);
  if (near >= 0 && !G.done) {
    float pl = 1 + .03f * sinf((float)gNow * 9);
    dRect(bx - 4, by + 6, bw + 8, bh, RGB(0x8d9600)); dGrad(bx - 4 * pl, by - 4 * pl, bw + 8 * pl, bh + 4 * pl, RGB(0xfffa7a), RGB(0xcfd900));
    dOut(bx - 4 * pl, by - 4 * pl, bw + 8 * pl, bh + 4 * pl, 3, RGB(WHITE));
    dTextO("OUVRIR", BOTW / 2, by + 16, 1.15f, RGB(0x111111), RGB(0xfffa7a), 1);
  } else {
    dRect(bx, by + 4, bw, bh, RGB(0x2a2f48)); dRect(bx, by, bw, bh, RGB(0x3c4262));
    dText(G.done ? "Termine !" : "Approche-toi d'un coffre", BOTW / 2, by + 24, .6f, RGB(0xaab0d0), 1);
  }
}

static void drawCoinsPill(float xr, float y) {
  char b[40]; float w;
  fmtn(b, sizeof b, S.coins); w = textW(b, .6f) + 40;
  dRect(xr - w, y, w, 24, RGBA(0x0a0c1c, 215)); dCoin(xr - w + 14, y + 12, 8); dText(b, xr - 8, y - 1, .6f, RGB(WHITE), 2);
}

static void drawScene(int mapi, int showHero) {
  const u32 *a = LB[mapi]; const Map *mp = &MAPS[mapi]; int k; float t = (float)gNow;
  dGrad(0, 0, TOPW, 190, RGB(a[0]), RGB(a[1]));
  if (((a[0] >> 16) & 255) < 0x60) for (k = 0; k < 16; k++) dRect((float)((k * 37) % 100) * 4, (float)((k * 23) % 55) * 3, 2, 2, RGBA(0xffffff, 150 + (int)(100 * sinf(t * 2 + k))));
  dCirc(300, 56, 48, RGBA(a[2], 40)); dCirc(300, 56, 38, RGBA(a[2], 90)); dCirc(300, 56, 27, RGB(a[2]));
  dEll(70, 196, 190, 64, RGB(a[3])); dEll(340, 200, 190, 56, RGB(a[3])); dEll(200, 214, 260, 46, RGB(a[4]));
  dRect(0, 188, TOPW, 52, RGB(mp->gr)); dRect(0, 188, TOPW, 3, RGBA(0xffffff, 90)); dRect(0, 205, TOPW, 35, RGBA(0, 45));
  for (k = 0; k < 7; k++) drawDecor(mp->dec[k % 5], (float)(34 + k * 55), 210.0f + (k % 2) * 10, gNow);
  dEll(235, 214, 92, 14, RGBA(0x38d8ff, 70)); dEll(235, 214, 72, 10, RGB(0x5de1ff)); dEll(235, 213, 52, 6, RGB(0xe8ffff));
  if (showHero) {
    float m = fmodf(t, 7), wave = m < 1.8f ? sinf(m / 1.8f * PI_F) : 0, m2 = fmodf(t, 11), jump = m2 > 9.4f ? sinf((m2 - 9.4f) / 1.6f * PI_F) * 26 : 0;
    float k2 = 1.2f;
    dEll(235, 214, 30, 8, RGBA(0, 90));
    drawFig(&SKINS[S.eq], 235, 212 - jump * k2, k2, 1, t, 0, 0, wave, jump);
  }
}

static void drawLobbyTop(void) {
  char b[64], a2[32];
  const Skin *sk = &SKINS[S.eq];
  C2D_TargetClear(gTop, RGB(0)); C2D_SceneBegin(gTop);
  drawScene(S.map, 1);
  dTextO("PIRATE", 10, 2, .95f, RGB(0xffd41f), RGB(0x3a1500), 0); dTextO("CHEST", 10, 34, .95f, RGB(0xffd41f), RGB(0x3a1500), 0);
  snprintf(b, sizeof b, "Rebirth %d", S.rb); dRect(8, 76, 112, 18, RGBA(0x0a0c1c, 200)); dText(b, 14, 74, .42f, RGB(WHITE), 0);
  fmtn(a2, sizeof a2, (s64)llround(pow(2, S.rb) * sk->cm)); snprintf(b, sizeof b, "×%s pièces", a2); dRect(8, 97, 112, 18, RGBA(0x0a0c1c, 200)); dText(b, 14, 95, .42f, RGB(WHITE), 0);
  fmtf(a2, sizeof a2, luckv(), 2); snprintf(b, sizeof b, "×%s chance", a2); dRect(8, 118, 112, 18, RGBA(0x0a0c1c, 200)); dText(b, 14, 116, .42f, RGB(WHITE), 0);
  fmtf(a2, sizeof a2, sk->spd, 2); snprintf(b, sizeof b, "×%s vitesse", a2); dRect(8, 139, 112, 18, RGBA(0x0a0c1c, 200)); dText(b, 14, 137, .42f, RGB(WHITE), 0);
  drawCoinsPill(TOPW - 6, 6);
  if (isFree()) { snprintf(b, sizeof b, "BOUTIQUE GRATUITE %ds", (int)(S.freeUntil - (s64)time(NULL))); dRect(8, 214, 170, 20, RGB(0xff3d7f)); dText(b, 93, 211, .45f, RGB(WHITE), 1); }
}

#define ROWH 30
#define LISTY 34
#define LISTN 6

static void drawTabs(int cur) {
  static const char *nm[3] = {"ACCUEIL", "CASIER", "BOUTIQUE"}; int i;
  dRect(0, 0, BOTW, 30, RGB(0x0a0c28));
  for (i = 0; i < 3; i++) {
    float x = i * 106.67f;
    if (i == cur) { dRect(x + 4, 4, 98, 24, RGB(WHITE)); dText(nm[i], x + 53, 3, .55f, RGB(0x111111), 1); }
    else dText(nm[i], x + 53, 3, .55f, RGB(WHITE), 1);
  }
  dRect(0, 30, BOTW, 3, RGBA(0xffd41f, 140));
}

static void drawRow(int idx, int sel, const char *name, const char *sub, const char *right, int coin, int dim, int head, int iconKind) {
  float y = LISTY + idx * ROWH; u32 bg = sel ? RGB(0x2b8cff) : RGB(0x1a2260);
  dRect(4, y, 312, ROWH - 3, bg); if (sel) dOut(4, y, 312, ROWH - 3, 2, RGB(0xfff35a));
  if (head >= 0) { dCirc(24, y + 14, 12, RGBA(0, 80)); drawHead(&SKINS[head], 24, y + 15, .5f); }
  else if (iconKind == 1) { dRect(10, y + 3, 28, 22, RGB(0x2b8cff)); dText("×", 24, y + 1, .5f, RGB(0xfff35a), 1); }
  else { dRect(10, y + 3, 28, 22, RGB(0x8a2be2)); dText("↻", 24, y + 1, .5f, RGB(0xffffff), 1); }
  dText(name, 46, y - 1, .5f, dim ? RGB(0xaab0d0) : RGB(WHITE), 0);
  dText(sub, 46, y + 14, .33f, RGB(0xc8d4ff), 0);
  if (right) {
    float w = textW(right, .45f);
    dText(right, 308, y + 5, .45f, dim ? RGB(0x8ff08f) : RGB(WHITE), 2);
    if (coin) dCoin(308 - w - 9, y + 14, 6.5f);
  }
}

static void ensureVisible(int *cur, int *top, int n) {
  if (*cur < 0) *cur = 0;
  if (*cur > n - 1) *cur = n - 1;
  if (*cur < *top) *top = *cur;
  if (*cur > *top + LISTN - 1) *top = *cur - LISTN + 1;
  if (*top < 0) *top = 0;
}

static int lockerList[NSKINS], nLock;
static void buildLocker(void) { int i; nLock = 0; for (i = 0; i < NSKINS; i++) if (hasSkin(i)) lockerList[nLock++] = i; }

static void drawLockerBottom(void) {
  int i; char sub[96];
  C2D_TargetClear(gBot, RGB(0x0e1630)); C2D_SceneBegin(gBot);
  dGrad(0, 0, BOTW, 240, RGB(0x1b2a85), RGB(0x0a1442));
  drawTabs(1); buildLocker(); ensureVisible(&curLock, &topLock, nLock);
  for (i = 0; i < LISTN && topLock + i < nLock; i++) {
    int si = lockerList[topLock + i];
    bonus(&SKINS[si], sub, sizeof sub);
    drawRow(i, topLock + i == curLock, SKINS[si].n, sub, S.eq == si ? "EQUIPÉ" : NULL, 0, 1, si, 0);
  }
  dText("HAUT/BAS choisir   A équiper   GAUCHE/DROITE onglets", BOTW / 2, 220, .3f, RGB(0xc8d4ff), 1);
}

static void drawSkinBig(int si, float x, float y, float k) {
  dEll(x, y + 4, 38 * k / 1.2f, 9, RGBA(0, 90));
  drawFig(&SKINS[si], x, y, k, 1, (float)gNow, 0, 0, 0, 0);
}

static void drawLockerTop(void) {
  char sub[96]; int si;
  C2D_TargetClear(gTop, RGB(0)); C2D_SceneBegin(gTop);
  dGrad(0, 0, TOPW, TOPH, RGB(0x081038), RGB(0x27428f)); dRect(0, 196, TOPW, 44, RGB(0x2a3347));
  buildLocker(); if (nLock == 0) return;
  if (curLock < 0) curLock = 0;
  if (curLock >= nLock) curLock = nLock - 1;
  si = lockerList[curLock];
  drawSkinBig(si, 130, 200, 1.1f);
  dTextO(SKINS[si].n, 290, 40, .9f, RGB(WHITE), RGB(0x06102b), 1);
  bonus(&SKINS[si], sub, sizeof sub); dText(sub, 290, 84, .45f, RGB(0xffd41f), 1);
  if (S.eq == si) { dRect(230, 120, 120, 26, RGB(0x1a7a1a)); dText("EQUIPÉ", 290, 119, .6f, RGB(WHITE), 1); }
  else dText("Appuie sur A pour équiper", 290, 125, .42f, RGB(WHITE), 1);
  dTextO("CASIER", 10, 4, .7f, RGB(0xffd41f), RGB(0x3a1500), 0);
  drawCoinsPill(TOPW - 6, 6);
}

#define SHOPN (2 + NSKINS - 1)

static void drawShopBottom(void) {
  int i; char sub[96], right[48], a[32], b[32], nm[48];
  C2D_TargetClear(gBot, RGB(0x0e1630)); C2D_SceneBegin(gBot);
  dGrad(0, 0, BOTW, 240, RGB(0x1b2a85), RGB(0x0a1442));
  drawTabs(2); ensureVisible(&curShop, &topShop, SHOPN);
  for (i = 0; i < LISTN && topShop + i < SHOPN; i++) {
    int it = topShop + i, sel = it == curShop;
    if (it == 0) {
      s64 c = isFree() ? 0 : upCost();
      fmtf(a, sizeof a, luckv(), 2); fmtf(b, sizeof b, luckv() * 1.1, 2);
      snprintf(nm, sizeof nm, "Chance niv. %d / %d", S.lvl, MAXLVL);
      if (S.lvl >= MAXLVL) snprintf(sub, sizeof sub, "Niveau maximum (×%s)", a); else snprintf(sub, sizeof sub, "×%s  →  ×%s", a, b);
      if (S.lvl >= MAXLVL) snprintf(right, sizeof right, "MAX"); else if (!c) snprintf(right, sizeof right, "GRATUIT"); else fmtn(right, sizeof right, c);
      drawRow(i, sel, nm, sub, right, S.lvl < MAXLVL && c > 0, S.lvl >= MAXLVL || !c, -1, 1);
    } else if (it == 1) {
      int mx = S.rb >= NMAPS - 1; s64 c = rbCost();
      snprintf(nm, sizeof nm, "Rebirth %d", S.rb);
      if (mx) snprintf(sub, sizeof sub, "Toutes les maps sont débloquées");
      else snprintf(sub, sizeof sub, "×%d pièces + map %s", 1 << (S.rb + 1), MAPS[S.rb + 1].n);
      if (mx) snprintf(right, sizeof right, "MAX"); else if (rbArm) snprintf(right, sizeof right, "CONFIRMER ?"); else fmtn(right, sizeof right, c);
      drawRow(i, sel, nm, sub, right, !mx && !rbArm, mx, -1, 2);
    } else {
      int si = it - 2, own = hasSkin(si); s64 p = isFree() ? 0 : (SKINS[si].p ? SKINS[si].p : PRICE);
      bonus(&SKINS[si], sub, sizeof sub);
      if (own) snprintf(right, sizeof right, "POSSÉDÉ"); else if (!p) snprintf(right, sizeof right, "GRATUIT"); else fmtn(right, sizeof right, p);
      drawRow(i, sel, SKINS[si].n, sub, right, !own && p > 0, own || !p, si, 0);
    }
  }
  if (msgT > 0) { float w = textW(msgTxt, .5f) + 20; dRect(BOTW / 2 - w / 2, 196, w, 20, RGB(0xd9304f)); dText(msgTxt, BOTW / 2, 194, .5f, RGB(WHITE), 1); }
  else dText("HAUT/BAS choisir   A acheter   GAUCHE/DROITE onglets", BOTW / 2, 220, .3f, RGB(0xc8d4ff), 1);
}

static void drawOddsPanel(void) {
  OddsCalc A = calcOdds(luckv()), B = calcOdds(S.lvl >= MAXLVL ? luckv() : luckv() * 1.1); int i; char t1[40], t2[40], p1[40], p2[40];
  dTextO("CHANCES DES COFFRES", 10, 2, .6f, RGB(0xffd41f), RGB(0x3a1500), 0);
  { char l[40]; fmtf(t1, sizeof t1, luckv(), 2); fmtf(t2, sizeof t2, S.lvl >= MAXLVL ? luckv() : luckv() * 1.1, 2); snprintf(l, sizeof l, "×%s → ×%s", t1, t2); dText(l, 390, 2, .5f, RGB(WHITE), 2); }
  dText("Coffre chanceux", 10, 28, .36f, RGB(0xc8d4ff), 0); dText("Actuel", 215, 28, .36f, RGB(0xc8d4ff), 1); dText("Niveau suivant", 320, 28, .36f, RGB(0xc8d4ff), 1);
  for (i = 0; i <= NODDS; i++) {
    float y = 44 + i * 27; int up; double pa, pb;
    dRect(6, y - 2, 388, 25, i & 1 ? RGBA(0x0a0c1c, 120) : RGBA(0x0a0c1c, 190));
    if (i < NODDS) {
      pa = A.p[i]; pb = B.p[i]; up = pb > pa * 1.0001; dCirc(16, y + 10, 4, RGB(ODDS[i].col)); dText(ODDS[i].lab, 26, y - 1, .38f, RGB(WHITE), 0);
      oddtxt(t1, sizeof t1, pa); pctf(p1, sizeof p1, pa); oddtxt(t2, sizeof t2, pb); pctf(p2, sizeof p2, pb);
    } else {
      pa = A.common; pb = B.common; up = 0; dCirc(16, y + 10, 4, RGB(0xcccccc)); dText("Coffre normal (1 pièce)", 26, y - 1, .38f, RGB(WHITE), 0);
      pctf(t1, sizeof t1, pa); p1[0] = 0; pctf(t2, sizeof t2, pb); p2[0] = 0;
    }
    dText(t1, 215, y - 1, .38f, RGB(WHITE), 1); if (p1[0]) dText(p1, 215, y + 11, .27f, RGB(0xaab0d0), 1);
    dText(t2, 320, y - 1, .38f, up ? RGB(0x7dff7d) : (i == NODDS && pb < pa - .00001 ? RGB(0xff8a8a) : RGB(WHITE)), 1);
    if (p2[0]) dText(p2, 320, y + 11, .27f, RGB(0xaab0d0), 1);
    if (up) dTri(372, y + 14, 380, y + 14, 376, y + 6, RGB(0x7dff7d));
    if (i == NODDS && pb < pa - .00001) dTri(372, y + 6, 380, y + 6, 376, y + 14, RGB(0xff8a8a));
  }
}

static void drawShopTop(void) {
  char sub[96], nb[32];
  C2D_TargetClear(gTop, RGB(0)); C2D_SceneBegin(gTop);
  dGrad(0, 0, TOPW, TOPH, RGB(0x081038), RGB(0x27428f));
  if (curShop == 0) { drawOddsPanel(); drawCoinsPill(TOPW - 6, 214); return; }
  dRect(0, 196, TOPW, 44, RGB(0x2a3347));
  if (curShop == 1) {
    int mx = S.rb >= NMAPS - 1;
    dTextO("RENAISSANCE", 10, 4, .8f, RGB(0xe0a8ff), RGB(0x3a0a6a), 0);
    snprintf(sub, sizeof sub, "×%d", 1 << S.rb); dTextO(sub, 200, 50, 1.6f, RGB(0xf0c8ff), RGB(0x3a0a6a), 1);
    snprintf(sub, sizeof sub, "Rebirth %d", S.rb); dText(sub, 200, 120, .6f, RGB(WHITE), 1);
    if (mx) dText("Toutes les maps sont débloquées", 200, 150, .45f, RGB(WHITE), 1);
    else {
      fmtn(nb, sizeof nb, rbCost()); snprintf(sub, sizeof sub, "Prochain : ×%d pièces + map « %s »", 1 << (S.rb + 1), MAPS[S.rb + 1].n);
      dText(sub, 200, 148, .42f, RGB(WHITE), 1); dText("Remet à zéro tes pièces et tes améliorations", 200, 168, .38f, RGB(0xff8a8a), 1);
      dText("(les skins restent)", 200, 184, .34f, RGB(0xc8d4ff), 1); snprintf(sub, sizeof sub, "Coût : %s pièces", nb); dText(sub, 200, 206, .5f, RGB(0xffd41f), 1);
    }
    drawCoinsPill(TOPW - 6, 6); return;
  }
  { int si = curShop - 2; const Skin *s = &SKINS[si]; s64 p = isFree() ? 0 : (s->p ? s->p : PRICE);
    drawSkinBig(si, 120, 200, 1.1f);
    dTextO(s->n, 290, 36, .9f, RGB(WHITE), RGB(0x06102b), 1);
    bonus(s, sub, sizeof sub); dText(sub, 290, 80, .42f, RGB(0xffd41f), 1);
    if (hasSkin(si)) { dRect(230, 120, 120, 26, RGB(0x1a7a1a)); dText("POSSÉDÉ", 290, 119, .6f, RGB(WHITE), 1); }
    else { fmtn(nb, sizeof nb, p); dCoin(240, 138, 9); dText(p ? nb : "GRATUIT", 256, 122, .65f, RGB(WHITE), 0); dText("Appuie sur A pour acheter", 290, 160, .4f, RGB(0xc8d4ff), 1); }
    if (s->nw && !hasSkin(si)) { dRect(10, 44, 70, 18, RGB(0xff4d6d)); dText("NOUVEAU", 45, 42, .4f, RGB(WHITE), 1); }
    dTextO("BOUTIQUE", 10, 4, .7f, RGB(0xffd41f), RGB(0x3a1500), 0);
    drawCoinsPill(TOPW - 6, 214); }
}

static void drawLobbyBottom(void) {
  char b[64];
  C2D_TargetClear(gBot, RGB(0x0e1630)); C2D_SceneBegin(gBot);
  dGrad(0, 0, BOTW, 240, RGB(0x1b2a85), RGB(0x0a1442));
  drawTabs(0);
  dRect(10, 42, 300, 44, RGB(0xd9d9e0));
  dTri(24, 64, 40, 50, 40, 78, RGB(0x333344)); dTri(296, 64, 280, 50, 280, 78, RGB(0x333344));
  snprintf(b, sizeof b, "%s · %d/%d", MAPS[S.map].n, S.map + 1, S.rb + 1); dText(b, BOTW / 2, 46, .65f, RGB(0x111111), 1);
  { float pl = 1 + .03f * sinf((float)gNow * 4);
    dRect(30, 100 + 7, 260, 76, RGB(0xa3ad00)); dGrad(30 - 5 * (pl - 1) * 10, 100, 260 + 10 * (pl - 1) * 10, 76, RGB(0xffee66), RGB(0xc5d400));
    dTextO("JOUER", BOTW / 2, 106, 1.5f, RGB(0x111111), RGB(0xffee66), 1); }
  dText("30 COFFRES · SOLO", BOTW / 2, 186, .45f, RGB(WHITE), 1);
  dText("A jouer   HAUT/BAS ou L/R changer de map", BOTW / 2, 204, .3f, RGB(0xc8d4ff), 1);
  dText("GAUCHE/DROITE onglets   START menu", BOTW / 2, 218, .3f, RGB(0xc8d4ff), 1);
}

#define MI_RESUME 0
#define MI_LOBBY 1
#define MI_SOUND 2
#define MI_SET 3
#define MI_QUIT 4
static int menuItems[5], nMenu;
static void buildMenu(void) {
  nMenu = 0; menuItems[nMenu++] = MI_RESUME;
  if (scr == SC_GAME) menuItems[nMenu++] = MI_LOBBY;
  menuItems[nMenu++] = MI_SOUND; menuItems[nMenu++] = MI_SET;
  if (scr != SC_GAME) menuItems[nMenu++] = MI_QUIT;
}
static void drawMenuBottom(void) {
  int i; static const char *nm[5] = {"Reprendre", "Retourner au lobby", "Son", "Paramètres", "Quitter le jeu"}; char b[48];
  C2D_TargetClear(gBot, RGB(0x0e1630)); C2D_SceneBegin(gBot);
  dGrad(0, 0, BOTW, 240, RGB(0x1b2a85), RGB(0x0a1442));
  dTextO("MENU", BOTW / 2, 6, .9f, RGB(0xffd41f), RGB(0x3a1500), 1);
  buildMenu(); if (curMenu >= nMenu) curMenu = nMenu - 1;
  for (i = 0; i < nMenu; i++) {
    float y = 50 + i * 36; int it = menuItems[i];
    dRect(30, y, 260, 31, i == curMenu ? RGB(0x2b8cff) : RGB(0x1a2260)); if (i == curMenu) dOut(30, y, 260, 31, 2, RGB(0xfff35a));
    if (it == MI_SOUND) snprintf(b, sizeof b, "Son : %s", S.mute ? "COUPÉ" : "ACTIVÉ"); else snprintf(b, sizeof b, "%s", nm[it]);
    dText(b, BOTW / 2, y + 2, .6f, RGB(WHITE), 1);
  }
  dText("HAUT/BAS choisir   A valider   B/START retour", BOTW / 2, 222, .3f, RGB(0xc8d4ff), 1);
}

#define NSET (1 + NODDS + 2)
static void drawSetBottom(void) {
  int i, top; char b[64];
  C2D_TargetClear(gBot, RGB(0x0e1630)); C2D_SceneBegin(gBot);
  dGrad(0, 0, BOTW, 240, RGB(0x1b2a85), RGB(0x0a1442));
  dTextO("PARAMÈTRES", BOTW / 2, 2, .7f, RGB(0xffd41f), RGB(0x3a1500), 1);
  top = curSet > 5 ? curSet - 5 : 0;
  for (i = 0; i < 7 && top + i < NSET; i++) {
    int it = top + i; float y = 34 + i * 26; const char *l, *v = ""; u32 vc = RGB(0x4a4a5a);
    if (it == 0) { l = "Cacher la roue ★ et ses effets"; v = S.hideAnim ? "CACHÉE" : "VISIBLE"; vc = S.hideAnim ? RGB(0xd9304f) : RGB(0x4a4a5a); }
    else if (it <= NODDS) { l = ODDS[it - 1].lab; v = (S.hide >> (it - 1)) & 1 ? "CACHÉE" : "VISIBLE"; vc = (S.hide >> (it - 1)) & 1 ? RGB(0xd9304f) : RGB(0x4a4a5a); }
    else if (it == NODDS + 1) { l = "Son"; v = S.mute ? "COUPÉ" : "ACTIVÉ"; vc = S.mute ? RGB(0xd9304f) : RGB(0x1a7a1a); }
    else { l = "Fermer"; v = ""; }
    dRect(8, y, 304, 24, it == curSet ? RGB(0x2b8cff) : RGB(0x1a2260)); if (it == curSet) dOut(8, y, 304, 24, 2, RGB(0xfff35a));
    snprintf(b, sizeof b, "%s", it == 0 ? "Cacher la roue et ses effets" : l); dText(b, 14, y - 1, .42f, RGB(WHITE), 0);
    if (v[0]) { dRect(222, y + 3, 82, 18, vc); dText(v, 263, y + 2, .36f, RGB(WHITE), 1); }
  }
  dText("Cacher l'animation de chaque coffre chanceux", BOTW / 2, 218, .3f, RGB(0xc8d4ff), 1);
}

static void drawOverlayTop(void) {
  char b[64], nb[32];
  if (ovl == OV_END) {
    dRect(0, 0, TOPW, TOPH, RGBA(0x03081a, 200));
    dRect(80, 40, 240, 160, RGB(0x34343f)); dOut(80, 40, 240, 160, 3, RGB(0xffd41f));
    dTextO("Map terminée !", 200, 46, .85f, RGB(0xffd41f), RGB(0), 1);
    snprintf(b, sizeof b, "%d coffres ouverts", MAXC); dText(b, 200, 92, .55f, RGB(WHITE), 1);
    fmtn(nb, sizeof nb, G.gain); snprintf(b, sizeof b, "+%s pièces", nb); dText(b, 200, 118, .6f, RGB(0xffd41f), 1);
    snprintf(b, sizeof b, "Coffres chanceux : %d", G.log[0] + G.log[1] + G.log[2] + G.log[3] + G.log[4] + G.log[5]); dText(b, 200, 146, .5f, RGB(WHITE), 1);
    dText("A : retour au lobby", 200, 174, .45f, RGB(0xc8d4ff), 1);
  } else if (ovl == OV_MENU || ovl == OV_SET) {
    dRect(0, 0, TOPW, TOPH, RGBA(0x03081a, 150));
    dTextO(ovl == OV_MENU ? "PAUSE" : "PARAMÈTRES", 200, 90, 1.1f, RGB(0xffd41f), RGB(0), 1);
  }
}

static int touchIn(float x, float y, float w, float h) { return (kD & KEY_TOUCH) && tp.px >= x && tp.px < x + w && tp.py >= y && tp.py < y + h; }

static void goTab(int s) { if (s == scr) return; scr = s; sfxPlay(SFX_TAB); musicSet(0); if (s == SC_LOCKER) { buildLocker(); } rbArm = 0; }
static void setSound(void) { S.mute = !S.mute; DIRTY(); }

static void menuAction(int it) {
  switch (it) {
    case MI_RESUME: ovl = OV_NONE; sfxPlay(SFX_CLICK); break;
    case MI_LOBBY: sfxPlay(SFX_QUIT); leaveGame(); break;
    case MI_SOUND: setSound(); sfxPlay(SFX_CLICK); break;
    case MI_SET: ovFrom = OV_MENU; ovl = OV_SET; curSet = 0; sfxPlay(SFX_CLICK); break;
    case MI_QUIT: quitReq = 1; break;
  }
}
static void setAction(int it) {
  if (it == 0) { S.hideAnim = !S.hideAnim; sfxPlay(SFX_CLICK); DIRTY(); }
  else if (it <= NODDS) { S.hide ^= 1u << (it - 1); sfxPlay(SFX_CLICK); DIRTY(); }
  else if (it == NODDS + 1) { setSound(); sfxPlay(SFX_CLICK); }
  else { ovl = OV_MENU; sfxPlay(SFX_CLICK); }
}

static void shopAction(int it) {
  if (it == 0) {
    s64 c = isFree() ? 0 : upCost();
    if (S.lvl >= MAXLVL) { sfxPlay(SFX_DENY); return; }
    if (S.coins < c) { sfxPlay(SFX_DENY); setMsg("Pas assez de pièces"); return; }
    S.coins -= c; S.lvl++; sfxPlay(SFX_BUY); DIRTY();
  } else if (it == 1) {
    s64 c = rbCost();
    if (S.rb >= NMAPS - 1) { sfxPlay(SFX_DENY); return; }
    if (S.coins < c) { sfxPlay(SFX_DENY); setMsg("Pas assez de pièces"); rbArm = 0; return; }
    if (!rbArm) { rbArm = 1; rbArmT = 3; sfxPlay(SFX_CLICK); setMsg("Appuie encore sur A pour confirmer"); return; }
    S.coins = 0; S.lvl = 0; S.rb++; S.map = S.rb; rbArm = 0; sfxPlay(SFX_JACKPOT); DIRTY();
  } else {
    int si = it - 2; s64 p = isFree() ? 0 : (SKINS[si].p ? SKINS[si].p : PRICE);
    if (hasSkin(si)) { sfxPlay(SFX_CLICK); return; }
    if (S.coins < p) { sfxPlay(SFX_DENY); setMsg("Pas assez de pièces"); return; }
    S.coins -= p; S.skins |= 1u << si; sfxPlay(SFX_BUY); DIRTY();
  }
}

static void changeMap(int d) { S.map = (S.map + d + (S.rb + 1)) % (S.rb + 1); sfxPlay(SFX_CLICK); DIRTY(); }

static void handleInput(void) {
  int i;
  if (kD & KEY_START) {
    if (ovl == OV_NONE) { ovl = OV_MENU; curMenu = 0; sfxPlay(SFX_CLICK); return; }
    if (ovl == OV_MENU) { ovl = OV_NONE; sfxPlay(SFX_CLICK); return; }
  }
  if (ovl == OV_MENU) {
    buildMenu();
    if (kD & KEY_DOWN) { curMenu = (curMenu + 1) % nMenu; sfxPlay(SFX_CLICK); }
    if (kD & KEY_UP) { curMenu = (curMenu + nMenu - 1) % nMenu; sfxPlay(SFX_CLICK); }
    if (kD & KEY_A) menuAction(menuItems[curMenu]);
    if (kD & KEY_B) { ovl = OV_NONE; sfxPlay(SFX_CLICK); }
    for (i = 0; i < nMenu; i++) if (touchIn(30, 50 + i * 36, 260, 31)) { curMenu = i; menuAction(menuItems[i]); break; }
    return;
  }
  if (ovl == OV_SET) {
    if (kD & KEY_DOWN) { curSet = (curSet + 1) % NSET; sfxPlay(SFX_CLICK); }
    if (kD & KEY_UP) { curSet = (curSet + NSET - 1) % NSET; sfxPlay(SFX_CLICK); }
    if (kD & KEY_A) setAction(curSet);
    if (kD & KEY_B) { ovl = ovFrom == OV_MENU ? OV_MENU : OV_NONE; sfxPlay(SFX_CLICK); }
    { int top = curSet > 5 ? curSet - 5 : 0;
      for (i = 0; i < 7 && top + i < NSET; i++) if (touchIn(8, 34 + i * 26, 304, 24)) { curSet = top + i; setAction(curSet); break; } }
    return;
  }
  if (ovl == OV_END) {
    if ((kD & KEY_A) || touchIn(0, 0, 320, 240)) { sfxPlay(SFX_CLICK); leaveGame(); }
    return;
  }
  switch (scr) {
    case SC_GAME: {
      float bx, by, bw, bh; btnOpenRect(&bx, &by, &bw, &bh);
      if ((kD & KEY_A) || touchIn(bx - 4, by - 4, bw + 8, bh + 8)) tryOpen();
    } break;
    case SC_LOBBY:
      if (kD & KEY_RIGHT) goTab(SC_LOCKER); else if (kD & KEY_LEFT) goTab(SC_SHOP);
      else if (kD & KEY_A) startGame();
      else if ((kD & (KEY_DDOWN | KEY_R))) changeMap(1);
      else if ((kD & (KEY_DUP | KEY_L))) changeMap(-1);
      else if (touchIn(30, 100, 260, 76)) startGame();
      else if (touchIn(10, 42, 70, 44)) changeMap(-1);
      else if (touchIn(240, 42, 70, 44)) changeMap(1);
      break;
    case SC_LOCKER:
      if (kD & KEY_RIGHT) goTab(SC_SHOP); else if (kD & KEY_LEFT) goTab(SC_LOBBY);
      buildLocker();
      if (kD & KEY_DOWN) { curLock++; sfxPlay(SFX_CLICK); }
      if (kD & KEY_UP) { curLock--; sfxPlay(SFX_CLICK); }
      if (curLock < 0) curLock = nLock - 1;
      if (curLock >= nLock) curLock = 0;
      if (kD & KEY_A) { S.eq = lockerList[curLock]; sfxPlay(SFX_EQUIP); DIRTY(); }
      for (i = 0; i < LISTN && topLock + i < nLock; i++) if (touchIn(4, LISTY + i * ROWH, 312, ROWH - 3)) { curLock = topLock + i; S.eq = lockerList[curLock]; sfxPlay(SFX_EQUIP); DIRTY(); }
      break;
    case SC_SHOP:
      if (kD & KEY_RIGHT) goTab(SC_LOBBY); else if (kD & KEY_LEFT) goTab(SC_LOCKER);
      if (kD & KEY_DOWN) { curShop = (curShop + 1) % SHOPN; sfxPlay(SFX_CLICK); rbArm = 0; }
      if (kD & KEY_UP) { curShop = (curShop + SHOPN - 1) % SHOPN; sfxPlay(SFX_CLICK); rbArm = 0; }
      if (kD & KEY_A) shopAction(curShop);
      for (i = 0; i < LISTN && topShop + i < SHOPN; i++) if (touchIn(4, LISTY + i * ROWH, 312, ROWH - 3)) { if (curShop != topShop + i) rbArm = 0; curShop = topShop + i; shopAction(curShop); }
      break;
  }
  if (scr != SC_GAME) {
    if (touchIn(0, 0, 106, 30)) goTab(SC_LOBBY); else if (touchIn(107, 0, 106, 30)) goTab(SC_LOCKER); else if (touchIn(214, 0, 106, 30)) goTab(SC_SHOP);
  }
}

int main(int argc, char **argv) {
  u64 last; (void)argc; (void)argv;
  gfxInitDefault();
  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(20000);
  C2D_Prepare();
  gTop = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
  gBot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
  tbuf = C2D_TextBufNew(8192);
  srand((unsigned)osGetTime());
  loadFile();
  audioInit();
  musicSet(0);
  last = osGetTime();

  while (aptMainLoop() && !quitReq) {
    u64 nowms = osGetTime(); float dt = (nowms - last) / 1000.0f; last = nowms;
    if (dt > .05f) dt = .05f;
    gNow += dt; wall += dt;
    hidScanInput(); kD = hidKeysDown(); kH = hidKeysHeld();
    hidCircleRead(&cp); if (kH & KEY_TOUCH) hidTouchRead(&tp);
    handleInput();
    if (rbArm) { rbArmT -= dt; if (rbArmT <= 0) rbArm = 0; }
    if (msgT > 0) msgT -= dt;
    if (scr == SC_GAME && ovl == OV_NONE) gameUpdate(dt);
    if (S.freeUntil && !isFree()) { S.freeUntil = 0; DIRTY(); }
    if (saveDirty && wall - lastSaveT > 3) { saveFile(); lastSaveT = wall; }
    audioPump();

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TextBufClear(tbuf);
    if (scr == SC_GAME) { drawGameTop(); if (ovl != OV_NONE) drawOverlayTop(); }
    else {
      if (scr == SC_LOBBY) drawLobbyTop(); else if (scr == SC_LOCKER) drawLockerTop(); else drawShopTop();
      if (ovl != OV_NONE) drawOverlayTop();
    }
    if (ovl == OV_MENU) drawMenuBottom();
    else if (ovl == OV_SET) drawSetBottom();
    else if (ovl == OV_END) { C2D_TargetClear(gBot, RGB(0x0e1630)); C2D_SceneBegin(gBot); dGrad(0, 0, BOTW, 240, RGB(0x1b2a85), RGB(0x0a1442)); dRect(30, 80, 260, 80, RGB(0xffee66)); dTextO("RETOUR AU LOBBY", BOTW / 2, 100, .75f, RGB(0x111111), RGB(0xffee66), 1); dText("A ou toucher l'écran", BOTW / 2, 190, .4f, RGB(WHITE), 1); }
    else if (scr == SC_GAME) drawGameBottom();
    else if (scr == SC_LOBBY) drawLobbyBottom();
    else if (scr == SC_LOCKER) drawLockerBottom();
    else drawShopBottom();
    C3D_FrameEnd(0);
  }

  saveFile();
  audioExit();
  C2D_TextBufDelete(tbuf);
  C2D_Fini(); C3D_Fini(); gfxExit();
  return 0;
}
