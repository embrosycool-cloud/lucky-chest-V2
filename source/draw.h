// Pirate Chest 3DS - dessin (tout est dessine par code, aucune image)
#ifndef DRAW_H
#define DRAW_H
#include "common.h"

#define DARK 0x06102b
#define WHITE 0xffffff

static C2D_TextBuf tbuf;
static double gNow = 0;

static void dRect(float x, float y, float w, float h, u32 c) { if (w > 0 && h > 0) C2D_DrawRectSolid(x, y, 0, w, h, c); }
static void dGrad(float x, float y, float w, float h, u32 t, u32 b) { C2D_DrawRectangle(x, y, 0, w, h, t, t, b, b); }
static void dCirc(float x, float y, float r, u32 c) { if (r > 0) C2D_DrawCircleSolid(x, y, 0, r, c); }
static void dEll(float x, float y, float rx, float ry, u32 c) { if (rx > 0 && ry > 0) C2D_DrawEllipseSolid(x - rx, y - ry, 0, rx * 2, ry * 2, c); }
static void dLine(float x0, float y0, float x1, float y1, float t, u32 c) { if (t < 1) t = 1; C2D_DrawLine(x0, y0, c, x1, y1, c, t, 0); }
static void dTri(float x0, float y0, float x1, float y1, float x2, float y2, u32 c) { C2D_DrawTriangle(x0, y0, c, x1, y1, c, x2, y2, c, 0); }
static void dOut(float x, float y, float w, float h, float t, u32 c) {
  if (t < 1) t = 1;
  dRect(x, y, w, t, c); dRect(x, y + h - t, w, t, c); dRect(x, y, t, h, c); dRect(x + w - t, y, t, h, c);
}
static void dRing(float cx, float cy, float rx, float ry, float t, u32 c, int n) {
  int i; float px = cx + rx, py = cy;
  for (i = 1; i <= n; i++) {
    float a = i * 2 * PI_F / n, x = cx + cosf(a) * rx, y = cy + sinf(a) * ry;
    dLine(px, py, x, y, t, c); px = x; py = y;
  }
}
static void dStar(float cx, float cy, float R, float r, u32 c, float rot) {
  int i; float px[10], py[10];
  for (i = 0; i < 10; i++) {
    float a = rot - PI_F / 2 + i * PI_F / 5, rr = (i & 1) ? r : R;
    px[i] = cx + cosf(a) * rr; py[i] = cy + sinf(a) * rr;
  }
  for (i = 0; i < 10; i++) dTri(cx, cy, px[i], py[i], px[(i + 1) % 10], py[(i + 1) % 10], c);
}
static void dCoin(float x, float y, float r) {
  dCirc(x, y + 1, r, RGB(0x8a5f00)); dCirc(x, y, r, RGB(0xc78f00)); dCirc(x - r * .08f, y - r * .08f, r * .82f, RGB(0xffd41f));
  dCirc(x - r * .2f, y - r * .25f, r * .35f, RGBA(0xfff7b0, 200));
}

static void dText(const char *s, float x, float y, float sc, u32 c, int al) {
  C2D_Text t; float w, h;
  C2D_TextParse(&t, tbuf, s); C2D_TextOptimize(&t);
  C2D_TextGetDimensions(&t, sc, sc, &w, &h);
  if (al == 1) x -= w / 2; else if (al == 2) x -= w;
  C2D_DrawText(&t, C2D_WithColor, x, y, 0, sc, sc, c);
}
static void dTextO(const char *s, float x, float y, float sc, u32 c, u32 oc, int al) {
  C2D_Text t; float w, h, o = sc > .7f ? 2.0f : 1.2f;
  C2D_TextParse(&t, tbuf, s); C2D_TextOptimize(&t);
  C2D_TextGetDimensions(&t, sc, sc, &w, &h);
  if (al == 1) x -= w / 2; else if (al == 2) x -= w;
  C2D_DrawText(&t, C2D_WithColor, x - o, y, 0, sc, sc, oc);
  C2D_DrawText(&t, C2D_WithColor, x + o, y, 0, sc, sc, oc);
  C2D_DrawText(&t, C2D_WithColor, x, y - o, 0, sc, sc, oc);
  C2D_DrawText(&t, C2D_WithColor, x, y + o, 0, sc, sc, oc);
  C2D_DrawText(&t, C2D_WithColor, x, y, 0, sc, sc, c);
}
static float textW(const char *s, float sc) {
  C2D_Text t; float w, h;
  C2D_TextParse(&t, tbuf, s); C2D_TextGetDimensions(&t, sc, sc, &w, &h);
  return w;
}

typedef struct { float p; u32 c; } Stop;
static const Stop TX_GAL[]  = {{0,0x2b1a8f},{.35f,0xb24cff},{.6f,0xff5ad0},{1,0x3ee0ff}};
static const Stop TX_GOLD[] = {{0,0xfff6b0},{.25f,0xffd41f},{.5f,0xb8860b},{.72f,0xffe680},{1,0xe0a800}};
static const Stop TX_LAVA[] = {{0,0x2a0500},{.3f,0xc21a00},{.55f,0xff6a00},{.8f,0xffd23a},{1,0x7a1000}};
static const Stop TX_ICE[]  = {{0,0xe8fbff},{.3f,0x7fd6ff},{.6f,0x2f8fe8},{1,0xb9f0ff}};
static const Stop TX_NEON[] = {{0,0x00fff0},{.35f,0xff00e5},{.7f,0x7a00ff},{1,0x00ff7a}};

static u32 texCol(int tex, float u) {
  const Stop *s; int n, i;
  switch (tex) {
    case T_GAL: s = TX_GAL; n = 4; break;
    case T_GOLD: s = TX_GOLD; n = 5; break;
    case T_LAVA: s = TX_LAVA; n = 5; break;
    case T_ICE: s = TX_ICE; n = 4; break;
    default: s = TX_NEON; n = 4; break;
  }
  u = u - floorf(u);
  for (i = 0; i < n - 1; i++) {
    if (u <= s[i + 1].p) {
      float f = (u - s[i].p) / (s[i + 1].p - s[i].p);
      u32 a = s[i].c, b = s[i + 1].c;
      int r = (int)(((a >> 16) & 255) + (((int)((b >> 16) & 255)) - (int)((a >> 16) & 255)) * f);
      int g = (int)(((a >> 8) & 255) + (((int)((b >> 8) & 255)) - (int)((a >> 8) & 255)) * f);
      int bl = (int)((a & 255) + (((int)(b & 255)) - (int)(a & 255)) * f);
      return C2D_Color32(r, g, bl, 255);
    }
  }
  return RGB(s[n - 1].c);
}
static u32 skinCol(const Skin *s, float u) { return s->tex ? texCol(s->tex, u + (float)gNow * .25f) : RGB(s->c); }

static float Hx, Hy, Hk, Hf, Hnk, Hrot;
static void hpt(float hx, float hy, float *ox, float *oy) {
  float lx = hx - 50, ly = hy - 50, c = cosf(Hrot), s = sinf(Hrot);
  float X = lx * c - ly * s + 50, Y = lx * s + ly * c + Hnk;
  *ox = Hx + (X - 50) * Hk * Hf; *oy = Hy + (Y - 130) * Hk;
}
static void hTri(float a, float b, float c2, float d, float e, float f, u32 col) {
  float x0, y0, x1, y1, x2, y2;
  hpt(a, b, &x0, &y0); hpt(c2, d, &x1, &y1); hpt(e, f, &x2, &y2);
  dTri(x0, y0, x1, y1, x2, y2, col);
}
static void hRect(float x, float y, float w, float h, u32 c) { hTri(x, y, x + w, y, x + w, y + h, c); hTri(x, y, x + w, y + h, x, y + h, c); }
static void hCirc(float cx, float cy, float r, u32 c) { float X, Y; hpt(cx, cy, &X, &Y); dEll(X, Y, r * Hk * fabsf(Hf), r * Hk, c); }
static void hEll(float cx, float cy, float rx, float ry, u32 c) { float X, Y; hpt(cx, cy, &X, &Y); dEll(X, Y, rx * Hk * fabsf(Hf), ry * Hk, c); }
static void hLine(float x0, float y0, float x1, float y1, float w, u32 c) {
  float a, b, c2, d, t = w * Hk;
  hpt(x0, y0, &a, &b); hpt(x1, y1, &c2, &d);
  dLine(a, b, c2, d, t, c);
  if (t > 3) { dCirc(a, b, t / 2, c); dCirc(c2, d, t / 2, c); }
}
static void hPoly(const float *p, int n, u32 c) {
  float cx = 0, cy = 0; int i;
  for (i = 0; i < n; i++) { cx += p[i * 2]; cy += p[i * 2 + 1]; }
  cx /= n; cy /= n;
  for (i = 0; i < n; i++) { int j = (i + 1) % n; hTri(cx, cy, p[i * 2], p[i * 2 + 1], p[j * 2], p[j * 2 + 1], c); }
}
static void hEdge(const float *p, int n, float w, u32 c) {
  int i; for (i = 0; i < n; i++) { int j = (i + 1) % n; hLine(p[i * 2], p[i * 2 + 1], p[j * 2], p[j * 2 + 1], w, c); }
}
static void hRing(float cx, float cy, float rx, float ry, float w, u32 c, int n) {
  int i; float px = cx + rx, py = cy;
  for (i = 1; i <= n; i++) {
    float a = i * 2 * PI_F / n, x = cx + cosf(a) * rx, y = cy + sinf(a) * ry;
    hLine(px, py, x, y, w, c); px = x; py = y;
  }
}
static void hStar(float cx, float cy, float R, float r, u32 c) {
  int i; float p[20];
  for (i = 0; i < 10; i++) { float a = -PI_F / 2 + i * PI_F / 5, rr = (i & 1) ? r : R; p[i * 2] = cx + cosf(a) * rr; p[i * 2 + 1] = cy + sinf(a) * rr; }
  for (i = 0; i < 10; i++) { int j = (i + 1) % 10; hTri(cx, cy, p[i * 2], p[i * 2 + 1], p[j * 2], p[j * 2 + 1], c); }
}

static void headParts(const Skin *sk) {
  int i;
  u32 col = skinCol(sk, .25f);
  hCirc(50, 34, 21, RGB(DARK));
  hCirc(50, 34, 19, col);
  if (sk->tex) hCirc(46, 40, 12, (skinCol(sk, .6f) & 0x00ffffff) | 0x90000000u);
  hCirc(58, 36, 4, RGB(DARK)); hCirc(44, 36, 4, RGB(DARK));
  { float mx[5] = {45, 48.4f, 51.75f, 55, 58}, my[5] = {45, 46.9f, 47.5f, 46.9f, 45};
    for (i = 0; i < 4; i++) hLine(mx[i], my[i], mx[i + 1], my[i + 1], 3, RGB(DARK)); }
  if (sk->tex) hEll(44, 27, 6, 3.5f, RGBA(0xffffff, 115));
  switch (sk->acc) {
    case A_NINJA:
      hRect(31, 26, 38, 8, RGB(0xd6212f)); hLine(69, 29, 85, 23, 4, RGB(0xd6212f)); hLine(69, 32, 83, 40, 4, RGB(0xd6212f)); break;
    case A_ROBOT:
      hLine(50, 15, 50, 2, 3, RGB(0x888888)); hCirc(50, 2, 4, RGB(0xff3333)); hRect(34, 28, 32, 10, RGBA(0x00aaff, 150)); break;
    case A_ASTRO:
      hCirc(50, 34, 24, RGBA(0x96dcff, 90)); hRing(50, 34, 24, 24, 3, RGB(WHITE), 22); break;
    case A_COWBOY: {
      static const float o[14] = {34,22, 36,9, 44,3, 50,2, 56,3, 64,9, 66,22};
      static const float d[14] = {36,21, 38,10, 44,6, 50,5, 56,6, 62,10, 64,21};
      hEll(50, 21, 32, 8, RGB(0x3b1d05)); hPoly(o, 7, RGB(0x3b1d05));
      hEll(50, 21, 30, 6, RGB(0x8b5a2b)); hPoly(d, 7, RGB(0x8b5a2b)); } break;
    case A_CROWN: {
      u32 g = RGB(0xffd41f), e = RGB(0x8a5f00);
      static const float oc[14] = {32,26, 36,6, 44,18, 50,2, 56,18, 64,6, 68,26};
      hTri(32,26, 44,18, 68,26, g); hTri(44,18, 56,18, 68,26, g);
      hTri(44,18, 50,2, 56,18, g); hTri(32,26, 36,6, 44,18, g); hTri(56,18, 64,6, 68,26, g);
      hEdge(oc, 7, 2.5f, e); } break;
    case A_CHEF:
      hCirc(40, 10, 10.5f, RGB(0x999999)); hCirc(60, 10, 10.5f, RGB(0x999999)); hCirc(50, 5, 11.5f, RGB(0x999999));
      hRect(34, 13, 32, 11, RGB(0x999999));
      hCirc(40, 10, 9, RGB(WHITE)); hCirc(60, 10, 9, RGB(WHITE)); hCirc(50, 5, 10, RGB(WHITE)); hRect(35, 14, 30, 9, RGB(WHITE)); break;
    case A_ZOMBIE: {
      static const float zx[5] = {36, 44, 50, 58, 64}, zy[5] = {22, 26, 22, 26, 22};
      for (i = 0; i < 4; i++) hLine(zx[i], zy[i], zx[i + 1], zy[i + 1], 3, RGB(0x1d3a1d));
      hLine(42, 44, 54, 42, 3, RGB(0x1d3a1d)); } break;
    case A_WIZARD:
      hTri(31, 24, 52, -16, 69, 24, RGB(0x2a0f7a)); hEll(50, 22, 29, 7, RGB(0x2a0f7a));
      hTri(34, 22, 52, -12, 66, 22, RGB(0x5b2bd1)); hEll(50, 22, 27, 5, RGB(0x5b2bd1)); break;
    case A_PIRATE:
      hTri(26, 26, 50, 4, 74, 26, RGB(0x1a1a1a)); hLine(26, 26, 50, 4, 2, RGB(WHITE)); hLine(50, 4, 74, 26, 2, RGB(WHITE));
      hEll(50, 24, 24, 3, RGB(0x1a1a1a)); hCirc(50, 16, 3, RGB(WHITE)); break;
    case A_BOLT: {
      u32 y = RGB(0xfff35a);
      hTri(54,-8, 42,8, 52,4, y); hTri(42,8, 50,8, 52,4, y); hTri(50,8, 45,18, 60,4, y); hTri(50,8, 60,4, 52,4, y); } break;
    case A_WINGS: {
      u32 w = RGB(WHITE), e = RGB(DARK);
      hTri(31,30, 8,16, 6,36, w); hTri(31,30, 6,36, 31,38, w);
      hTri(69,30, 92,16, 94,36, w); hTri(69,30, 94,36, 69,38, w);
      hLine(31,30, 8,16, 2, e); hLine(8,16, 6,36, 2, e); hLine(69,30, 92,16, 2, e); hLine(92,16, 94,36, 2, e); } break;
    case A_SAILOR: {
      static const float s[14] = {30,24, 34,13, 42,7, 50,5, 58,7, 66,13, 70,24};
      hPoly(s, 7, RGB(WHITE)); hRect(30, 22, 40, 5, RGB(0x1d4fd1)); hEdge(s, 7, 1.5f, RGB(DARK)); } break;
    case A_STARS:
      hCirc(36, 28, 2, RGB(WHITE)); hCirc(62, 26, 1.6f, RGB(WHITE)); hCirc(56, 46, 1.4f, RGB(WHITE));
      hStar(50, -1, 10, 4.8f, RGB(0x8a5f00)); hStar(50, -1, 8, 3.5f, RGB(0xffe65c)); break;
    case A_HORNS:
      hTri(34,22, 36,0, 42,18, RGB(0xff5a1a)); hTri(34,22, 27,10, 36,0, RGB(0xff5a1a));
      hTri(66,22, 64,0, 58,18, RGB(0xff5a1a)); hTri(66,22, 73,10, 64,0, RGB(0xff5a1a)); break;
    case A_CRYSTAL: {
      u32 c = RGB(0xc8f4ff);
      hTri(38,22, 42,4, 47,20, c); hTri(47,20, 52,0, 57,20, c); hTri(57,20, 62,6, 64,24, c);
      hTri(38,22, 47,20, 64,24, c); hTri(47,20, 57,20, 64,24, c);
      hLine(42,4, 47,20, 1.5f, RGB(0x2a6f9a)); hLine(52,0, 57,20, 1.5f, RGB(0x2a6f9a)); hLine(62,6, 64,24, 1.5f, RGB(0x2a6f9a)); } break;
    case A_HALO: hRing(50, 8, 16, 5, 3.5f, RGB(0xffd41f), 24); break;
    case A_VISOR:
      hRect(31, 28, 38, 11, RGB(WHITE)); hRect(32.5f, 29.5f, 35, 8, RGBA(0x00fff0, 225)); break;
    case A_VIKING: {
      static const float d[14] = {31,24, 35,14, 42,8, 50,6, 58,8, 65,14, 69,24};
      static const float hl[10] = {33,20, 21,13, 22,2, 28,9, 36,14};
      static const float hr[10] = {67,20, 79,13, 78,2, 72,9, 64,14};
      hPoly(hl, 5, RGB(WHITE)); hPoly(hr, 5, RGB(WHITE)); hPoly(d, 7, RGB(0x9aa4b5)); hEdge(d, 7, 1.5f, RGB(0x3b4455)); } break;
    case A_SAMURAI: {
      static const float d[14] = {30,26, 34,14, 42,6, 50,3, 58,6, 66,14, 70,26};
      hPoly(d, 7, RGB(0x8a1c1c)); hLine(50,-4, 40,12, 3, RGB(0xffd41f)); hLine(50,-4, 60,12, 3, RGB(0xffd41f));
      hRect(30, 24, 40, 5, RGB(0x2a0606)); } break;
    default: break;
  }
}

static void drawHead(const Skin *sk, float cx, float cy, float k) {
  Hx = cx; Hk = k; Hf = 1; Hnk = 50; Hrot = 0; Hy = cy + 96 * k;
  headParts(sk);
}

static void drawFig(const Skin *sk, float x, float y, float k, float f, float t, float ph, float w, float wv, float jp) {
  float br = sinf(t * 2.6f) * 1.5f * (1 - w), bn = w * fabsf(sinf(ph)) * 5 + jp;
  float hip = 95 - bn, nk = 50 - bn + br * .6f, shy = 62 - bn + br * .5f;
  float P[5][3][2]; int NP[5], i, p, j;
  P[0][0][0] = 50; P[0][0][1] = nk; P[0][1][0] = 50; P[0][1][1] = hip; NP[0] = 2;
  for (i = 0; i < 2; i++) {
    float q = ph + i * PI_F, a = (i ? .1f : -.1f) * (1 - w) + sinf(q) * .75f * w;
    float b = fmaxf(0, cosf(q)) * .9f * w + jp * .05f, a2 = a - b;
    P[1 + i][0][0] = 50; P[1 + i][0][1] = hip;
    P[1 + i][1][0] = 50 + sinf(a) * 17.5f; P[1 + i][1][1] = hip + cosf(a) * 17.5f;
    P[1 + i][2][0] = P[1 + i][1][0] + sinf(a2) * 17.5f; P[1 + i][2][1] = P[1 + i][1][1] + cosf(a2) * 17.5f; NP[1 + i] = 3;
  }
  for (i = 0; i < 2; i++) {
    float side = i ? 1.0f : -1.0f;
    float sw = -sinf(ph + i * PI_F) * .75f * w + sinf(t * 2.6f + i * 1.7f) * .06f * (1 - w);
    float a1 = side * .42f + sw + side * jp * .04f, a2 = a1 + .3f + w * .4f * (1 + sinf(ph + i * PI_F)) / 2;
    if (i == 1 && wv > 0) { a1 = a1 * (1 - wv) + 2.55f * wv; a2 = a1 + sinf(t * 14) * .6f * wv; }
    P[3 + i][0][0] = 50; P[3 + i][0][1] = shy;
    P[3 + i][1][0] = 50 + sinf(a1) * 17; P[3 + i][1][1] = shy + cosf(a1) * 17;
    P[3 + i][2][0] = P[3 + i][1][0] + sinf(a2) * 17; P[3 + i][2][1] = P[3 + i][1][1] + cosf(a2) * 17; NP[3 + i] = 3;
  }
  { for (j = 0; j < 2; j++) {
      float lw = (j == 0 ? 12.0f : 6.0f) * k;
      for (p = 0; p < 5; p++) {
        for (i = 0; i + 1 < NP[p]; i++) {
          float x0 = x + (P[p][i][0] - 50) * k * f, y0 = y + (P[p][i][1] - 130) * k;
          float x1 = x + (P[p][i + 1][0] - 50) * k * f, y1 = y + (P[p][i + 1][1] - 130) * k;
          u32 c = j == 0 ? RGB(DARK) : skinCol(sk, (P[p][i][1] + 14) / 154.0f);
          dLine(x0, y0, x1, y1, lw, c);
          dCirc(x0, y0, lw / 2, c); dCirc(x1, y1, lw / 2, c);
        }
      }
    }
  }
  Hx = x; Hy = y; Hk = k; Hf = f; Hnk = nk;
  Hrot = sinf(ph) * .07f * w + sinf(t * 1.3f) * .04f * (1 - w) + wv * .08f;
  headParts(sk);
}

static float cX, cY, cSX, cSY;
static float CPX(float dx) { return cX + dx * cSX; }
static float CPY(float dy) { return cY + dy * cSY; }
static void cR(float dx, float dy, float w, float h, u32 c) { dRect(CPX(dx), CPY(dy), w * cSX, h * cSY, c); }
static void cRO(float dx, float dy, float w, float h, float t, u32 c) { dOut(CPX(dx), CPY(dy), w * cSX, h * cSY, t * cSX, c); }
static void cC(float dx, float dy, float r, u32 c) { dCirc(CPX(dx), CPY(dy), r * cSX, c); }
static void cE(float dx, float dy, float rx, float ry, u32 c) { dEll(CPX(dx), CPY(dy), rx * cSX, ry * cSY, c); }
static void cL(float x0, float y0, float x1, float y1, float t, u32 c) { dLine(CPX(x0), CPY(y0), CPX(x1), CPY(y1), t * cSX, c); }
static void cT(float x0, float y0, float x1, float y1, float x2, float y2, u32 c) { dTri(CPX(x0), CPY(y0), CPX(x1), CPY(y1), CPX(x2), CPY(y2), c); }

static void chestTex(int st, float x, float y, float w, float h, double now) {
  int i, j;
  switch (st) {
    case ST_WOOD: for (i = 1; i < 3; i++) cR(x, y + h * i / 3 - 1, w, 2, RGBA(0, 70)); break;
    case ST_BRICK:
      for (j = 0; j < 3; j++) {
        float yy = y + h * j / 3; cR(x, yy, w, 2, RGBA(0x503208, 115));
        for (i = 0; i < 6; i++) { float xx = x + i * w / 5 + (j % 2 ? w / 10 : 0); if (xx < x + w - 1) cR(xx, yy, 2, h / 3, RGBA(0x503208, 115)); }
      } break;
    case ST_ICE:
      for (i = 0; i < 4; i++) cT(x + i * w / 4, y + h, x + i * w / 4 + w / 8, y + h * .2f, x + (i + 1) * w / 4, y + h, RGBA(WHITE, 125)); break;
    case ST_LAVA: {
      int g = 130 + (int)(60 * sinf((float)now / 180.0f)); u32 c = C2D_Color32(255, g, 20, 255);
      cL(x + 4, y + h * .3f, x + w * .3f, y + h * .65f, 3, c); cL(x + w * .3f, y + h * .65f, x + w * .5f, y + h * .35f, 3, c);
      cL(x + w * .5f, y + h * .35f, x + w * .75f, y + h * .7f, 3, c); cL(x + w * .75f, y + h * .7f, x + w - 4, y + h * .4f, 3, c); } break;
    case ST_VINE:
      for (i = 0; i < 4; i++) { float vx = x + w * (i + .5f) / 4; cL(vx, y, vx + 3, y + h * .4f, 3, RGB(0x2fa84a)); cL(vx + 3, y + h * .4f, vx - 3, y + h * .8f, 3, RGB(0x2fa84a)); cC(vx - 3, y + h * .8f, 3.5f, RGB(0x5fe07a)); } break;
    case ST_CANDY:
      for (i = -2; i < 8; i++) {
        float x0 = x + i * 10, lo = fmaxf(0, (x - x0) / h), hi = fminf(1, (x + w - x0) / h);
        if (lo < hi) cL(x0 + lo * h, y + h - lo * h, x0 + hi * h, y + h - hi * h, 4, RGBA(WHITE, 215));
      } break;
    case ST_SPACE:
      for (i = 0; i < 10; i++) { int a = (int)(100 + 155 * fabsf(sinf((float)now / 300.0f + i * 2))); cR(x + (float)((i * 37) % (int)w), y + (float)((i * 23) % (int)h), 2.5f, 2.5f, C2D_Color32(255, 255, 255, a)); } break;
    default: {
      static const u32 gc[3] = {0x3ee0ff, 0xff3d7f, 0x5dff8a};
      for (i = 0; i < 3; i++) {
        float gx = x + w * (i + .5f) / 3, gy = y + h * .5f;
        cT(gx, gy - 8, gx + 6, gy, gx, gy + 8, RGB(gc[i])); cT(gx, gy - 8, gx, gy + 8, gx - 6, gy, RGB(gc[i]));
        cL(gx, gy - 8, gx + 6, gy, 1.5f, RGBA(WHITE, 200)); cL(gx + 6, gy, gx, gy + 8, 1.5f, RGBA(WHITE, 200));
        cL(gx, gy + 8, gx - 6, gy, 1.5f, RGBA(WHITE, 200)); cL(gx - 6, gy, gx, gy - 8, 1.5f, RGBA(WHITE, 200));
      } } break;
  }
}

static void drawChest(const Map *m, float x, float y, float sc, float pop, float pr, int open, int hl, int rare, double nowms) {
  u32 E = RGB(0x241004), L = RGB(m->cl), B = RGB(m->cb), M = RGB(m->cm), K = RGB(m->ck);
  int bi; static const float BX[2] = {-21, 14};
  cSX = sc * (1 + pop); cSY = sc * (1 - .6f * pop); cX = x; cY = y + 20 * sc - 20 * cSY;
  if (rare && !open) {
    float a = .5f + .2f * sinf((float)nowms / 200.0f);
    dCirc(x, y, 52 * sc, RGB(0xf0c8ff)); dCirc(x, y, 49 * sc, C2D_Color32(200, 110, 255, (int)(a * 255)));
  }
  if (hl && !open) dCirc(x, y, (46 + sinf((float)nowms / 150.0f) * 4) * sc, RGBA(0xfff478, 140));
  if (open) {
    float lh = 30 * pr;
    cR(-27, -8 - lh, 54, lh, L); cRO(-27, -8 - lh, 54, lh, 3, E);
    cR(-22, -4 - lh, 44, fmaxf(0, lh - 4), RGBA(0, 100));
  }
  cR(-28, -8, 56, 28, B);
  chestTex(m->cst, -28, -8, 56, 28, nowms);
  cR(-28, -8, 56, 5, RGBA(WHITE, 56)); cR(-28, 5, 56, 2, RGBA(0, 50)); cR(-28, 13, 56, 7, RGBA(0, 76));
  for (bi = 0; bi < 2; bi++) {
    float bx = BX[bi];
    cR(bx, -8, 7, 28, M); cRO(bx, -8, 7, 28, 2.5f, E); cR(bx + 1.5f, -8, 2, 28, RGBA(WHITE, 125));
    cC(bx + 3.5f, -3, 1.5f, RGBA(0, 100)); cC(bx + 3.5f, 14, 1.5f, RGBA(0, 100));
  }
  cR(-28, 12, 7, 8, M); cRO(-28, 12, 7, 8, 2.5f, E); cR(21, 12, 7, 8, M); cRO(21, 12, 7, 8, 2.5f, E);
  cRO(-28, -8, 56, 28, 3, E);
  if (open) {
    cE(0, -8, 25, 6, RGB(0x2a1608));
  } else {
    float d, py = 0;
    for (d = -28; d < 28; d += 2) {
      float dm = d + 1, hh = 19 * (1 - (dm / 28) * (dm / 28)); int band = (fabsf(dm + 17.5f) < 3.5f) || (fabsf(dm - 17.5f) < 3.5f);
      cR(d, -8 - hh, 2.2f, hh, band ? M : L);
      if (hh > 7) cR(d, -14, 2.2f, 6, RGBA(0, 56));
    }
    for (d = -28, py = 0; d <= 28; d += 4) {
      float hh = 19 * (1 - (d / 28) * (d / 28));
      if (d > -28) cL(d - 4, -8 - py, d, -8 - hh, 3, E);
      py = hh;
    }
    cL(-22, -14, -15, -22, 3, RGBA(WHITE, 140)); cL(-15, -22, -5, -25, 3, RGBA(WHITE, 140));
    cR(-8, -16, 16, 18, K); cRO(-8, -16, 16, 18, 2.5f, E); cR(-6, -14, 12, 4, RGBA(WHITE, 125));
    cC(0, -6, 2.6f, E); cR(-1, -6, 2, 6, E);
  }
  if (rare && !open) {
    float sx = CPX(0), sy = CPY(-50 + sinf((float)nowms / 200.0f) * 3);
    dStar(sx, sy, 8 * sc, 3.5f * sc, RGB(0xf6d6ff), 0);
  }
}

static void drawDecor(int k, float x, float y, double now) {
  int i;
  dEll(x, y + 2, 20, 7, RGBA(0, 75));
  switch (k) {
    case D_PALM: {
      float bx = x + 5, by = y - 34;
      dLine(x, y, bx, by, 5, RGB(0x7a4a1a));
      for (i = 0; i < 5; i++) {
        float a = -PI_F + i * PI_F / 4, tx = bx + cosf(a) * 25, ty = by + sinf(a) * 14 + fabsf(cosf(a)) * 10;
        dLine(bx, by, tx, ty, 5, RGB(0x2f8f3a)); dLine(bx, by, (bx + tx) / 2, (by + ty) / 2 - 1, 2, RGB(0x5fd06a));
      }
      dCirc(bx - 3, by + 4, 3, RGB(0x5a3512)); dCirc(bx + 3, by + 5, 3, RGB(0x5a3512)); } break;
    case D_ROCK: dEll(x, y - 8, 17, 11, RGB(0x8a8a92)); dEll(x - 4, y - 11, 9, 5, RGB(0xb0b0b8)); dEll(x + 6, y - 4, 7, 4, RGB(0x74747c)); break;
    case D_BUSH: dCirc(x - 9, y - 8, 9, RGB(0x2f8f3a)); dCirc(x + 9, y - 8, 9, RGB(0x2f8f3a)); dCirc(x, y - 14, 11, RGB(0x3fae4a)); dCirc(x - 3, y - 17, 4, RGB(0x6fd06a)); break;
    case D_CACTUS:
      dRect(x - 5, y - 38, 10, 38, RGB(0x3f9a3a)); dCirc(x, y - 38, 5, RGB(0x3f9a3a));
      dRect(x - 15, y - 25, 11, 5, RGB(0x3f9a3a)); dRect(x - 15, y - 34, 5, 14, RGB(0x3f9a3a)); dCirc(x - 12.5f, y - 34, 2.5f, RGB(0x3f9a3a));
      dRect(x + 4, y - 19, 11, 5, RGB(0x3f9a3a)); dRect(x + 10, y - 28, 5, 14, RGB(0x3f9a3a)); dCirc(x + 12.5f, y - 28, 2.5f, RGB(0x3f9a3a));
      dRect(x - 1, y - 36, 2, 34, RGB(0x2b7a2a)); break;
    case D_DUNE: dEll(x, y - 6, 26, 10, RGB(0xe8c878)); dEll(x + 6, y - 9, 15, 6, RGB(0xf5dc96)); break;
    case D_CAMEL:
      dLine(x - 9, y - 16, x - 10, y, 3, RGB(0x9a7040)); dLine(x - 4, y - 16, x - 4, y, 3, RGB(0x9a7040));
      dLine(x + 5, y - 16, x + 5, y, 3, RGB(0x9a7040)); dLine(x + 10, y - 16, x + 11, y, 3, RGB(0x9a7040));
      dEll(x, y - 22, 16, 8, RGB(0xc89a5a)); dCirc(x - 5, y - 30, 5, RGB(0xc89a5a)); dCirc(x + 5, y - 30, 5, RGB(0xc89a5a));
      dLine(x + 12, y - 24, x + 19, y - 38, 5, RGB(0xc89a5a)); dEll(x + 23, y - 40, 7, 4, RGB(0xc89a5a)); dCirc(x + 24, y - 42, 1.2f, RGB(DARK)); break;
    case D_PINE:
      dRect(x - 3, y - 9, 6, 9, RGB(0x6b4a1a));
      dTri(x - 17, y - 8, x + 17, y - 8, x, y - 30, RGB(0x1f6a3a)); dTri(x - 13, y - 22, x + 13, y - 22, x, y - 42, RGB(0x2a7a46));
      dTri(x - 9, y - 34, x + 9, y - 34, x, y - 54, RGB(0x35905a));
      dTri(x - 4, y - 48, x + 4, y - 48, x, y - 54, RGB(WHITE)); dTri(x - 8, y - 29, x + 8, y - 29, x, y - 36, RGBA(WHITE, 200)); break;
    case D_SNOWMAN:
      dCirc(x, y - 10, 13, RGB(0x9ab0c0)); dCirc(x, y - 10, 12, RGB(0xeaf2f8)); dCirc(x, y - 28, 10, RGB(0x9ab0c0)); dCirc(x, y - 28, 9, RGB(0xeaf2f8));
      dCirc(x, y - 42, 8, RGB(0x9ab0c0)); dCirc(x, y - 42, 7, RGB(0xeaf2f8));
      dCirc(x - 2.5f, y - 43.5f, 1.2f, RGB(DARK)); dCirc(x + 2.5f, y - 43.5f, 1.2f, RGB(DARK)); dTri(x, y - 41, x + 9, y - 39, x, y - 38, RGB(0xff8a1a));
      dCirc(x, y - 30, 1.3f, RGB(DARK)); dCirc(x, y - 24, 1.3f, RGB(DARK)); break;
    case D_FLAKE:
      for (i = 0; i < 3; i++) { float a = i * PI_F / 3; dLine(x + cosf(a) * 14, y - 20 + sinf(a) * 14, x - cosf(a) * 14, y - 20 - sinf(a) * 14, 2.5f, RGB(0xbfe8ff)); }
      dCirc(x, y - 20, 3, RGB(WHITE)); break;
    case D_VOLCANO:
      dTri(x - 32, y, x + 32, y, x - 10, y - 42, RGB(0x5a3a2a)); dTri(x + 32, y, x - 10, y - 42, x + 10, y - 42, RGB(0x5a3a2a));
      dEll(x, y - 42, 10, 3.5f, RGB(0xff6a1a)); dLine(x - 3, y - 42, x - 9, y - 14, 3, RGB(0xff8a1a)); dCirc(x, y - 49, 6 + sinf((float)now * 4) * 1.5f, RGBA(0xffa030, 150)); break;
    case D_FIRE:
      dTri(x - 10, y, x + 10, y, x, y - 32, RGB(0xff6a1a)); dTri(x + 5, y, x + 13, y, x + 10, y - 18, RGB(0xff8a1a));
      dTri(x - 5, y, x + 5, y, x, y - 18, RGB(0xffd23a)); break;
    case D_SKULL:
      dCirc(x, y - 19, 11, RGB(0xf2f2ea)); dRect(x - 6, y - 13, 12, 9, RGB(0xf2f2ea));
      dCirc(x - 4, y - 19, 3, RGB(DARK)); dCirc(x + 4, y - 19, 3, RGB(DARK)); dTri(x - 1.5f, y - 14, x + 1.5f, y - 14, x, y - 11, RGB(DARK));
      dRect(x - 2.5f, y - 8, 1, 4, RGB(0x999999)); dRect(x + 1.5f, y - 8, 1, 4, RGB(0x999999)); break;
    case D_FLOWER:
      dLine(x, y, x, y - 22, 3, RGB(0x2f8f3a)); dEll(x + 6, y - 9, 6, 3, RGB(0x3fae4a));
      for (i = 0; i < 5; i++) { float a = i * 2 * PI_F / 5; dCirc(x + cosf(a) * 6, y - 27 + sinf(a) * 6, 4.5f, RGB(0xff6a9a)); }
      dCirc(x, y - 27, 3.5f, RGB(0xffe14d)); break;
    case D_PARROT:
      dLine(x - 14, y, x + 14, y - 2, 3, RGB(0x6b4a1a));
      dLine(x + 2, y - 10, x + 7, y + 1, 4, RGB(0x2b8cff)); dEll(x, y - 20, 8, 12, RGB(0xe8343a)); dEll(x + 2, y - 18, 5, 8, RGB(0x2b8cff));
      dCirc(x - 2, y - 34, 6.5f, RGB(0xe8343a)); dTri(x - 7, y - 36, x - 14, y - 32, x - 7, y - 30, RGB(0xffd41f)); dCirc(x - 3, y - 35, 1.5f, RGB(WHITE)); dCirc(x - 3, y - 35, .7f, RGB(DARK)); break;
    case D_LOLLI:
      dLine(x, y, x, y - 26, 3, RGB(WHITE)); dCirc(x, y - 35, 13, RGB(0xff5fa2)); dRing(x, y - 35, 8, 8, 2.5f, RGB(WHITE), 16); dRing(x, y - 35, 3.5f, 3.5f, 2.5f, RGB(WHITE), 10); break;
    case D_CANDY:
      dTri(x - 8, y - 14, x - 19, y - 22, x - 19, y - 6, RGB(WHITE)); dTri(x + 8, y - 14, x + 19, y - 22, x + 19, y - 6, RGB(WHITE));
      dEll(x, y - 14, 10, 7, RGB(0xff5fa2)); dLine(x - 4, y - 19, x - 1, y - 9, 2.5f, RGB(WHITE)); dLine(x + 3, y - 19, x + 6, y - 9, 2.5f, RGB(WHITE)); break;
    case D_CUPCAKE:
      dTri(x - 11, y - 14, x + 11, y - 14, x + 8, y, RGB(0xe8a050)); dTri(x - 11, y - 14, x + 8, y, x - 8, y, RGB(0xe8a050));
      dLine(x - 4, y - 13, x - 3, y - 1, 1.5f, RGB(0xc07830)); dLine(x + 3, y - 13, x + 3, y - 1, 1.5f, RGB(0xc07830));
      dCirc(x - 7, y - 18, 7, RGB(0xff9ac8)); dCirc(x + 7, y - 18, 7, RGB(0xff9ac8)); dCirc(x, y - 25, 8.5f, RGB(0xffb6d8)); dCirc(x, y - 34, 3.5f, RGB(0xe8232f)); break;
    case D_DONUT:
      dCirc(x, y - 13, 15, RGB(0xd9a05a)); dCirc(x, y - 14, 13, RGB(0xff7ab8)); dCirc(x, y - 14, 4.5f, RGB(0xb07840));
      dLine(x - 8, y - 18, x - 5, y - 20, 2, RGB(0xffe14d)); dLine(x + 6, y - 20, x + 9, y - 17, 2, RGB(0x7ad8ff)); dLine(x - 9, y - 10, x - 6, y - 8, 2, RGB(WHITE)); dLine(x + 5, y - 8, x + 9, y - 10, 2, RGB(0x9d6bff)); break;
    case D_ICECREAM:
      dTri(x - 10, y - 22, x + 10, y - 22, x, y, RGB(0xe0a860)); dLine(x - 6, y - 21, x + 2, y - 4, 1.5f, RGB(0xb88040)); dLine(x + 6, y - 21, x - 2, y - 4, 1.5f, RGB(0xb88040));
      dCirc(x, y - 27, 11, RGB(0xffd1e6)); dCirc(x, y - 38, 9, RGB(0xfff0f5)); dCirc(x, y - 47, 3, RGB(0xe8232f)); break;
    case D_STAR:
      dStar(x, y - 22, 20, 10, RGB(0x8a5f00), (float)now * .6f); dStar(x, y - 22, 17, 8, RGB(0xffe65c), (float)now * .6f); break;
    case D_PLANET:
      dRing(x, y - 22, 24, 7, 3, RGB(0xffe0a0), 24); dCirc(x, y - 22, 14, RGB(0xff9a4a)); dEll(x - 3, y - 25, 9, 4, RGB(0xffb878)); dRect(x - 12, y - 20, 24, 3, RGBA(0xc2410c, 150)); break;
    case D_COMET:
      dTri(x - 34, y - 6, x - 5, y - 30, x - 3, y - 14, RGBA(0x9be8ff, 150)); dTri(x - 26, y - 16, x - 6, y - 32, x, y - 20, RGBA(0xdff4ff, 170));
      dCirc(x, y - 22, 10, RGB(0xdff4ff)); dCirc(x - 2, y - 24, 5, RGB(WHITE)); break;
    case D_UFO:
      dTri(x - 8, y - 14, x + 8, y - 14, x, y, RGBA(0x9bf0ff, 90));
      dEll(x, y - 26, 10, 9, RGBA(0x9bf0ff, 210)); dEll(x, y - 20, 20, 7, RGB(0x9aa4b5)); dEll(x, y - 22, 20, 5, RGB(0xc0c8d6));
      dCirc(x - 11, y - 19, 2, RGB(0xffe65c)); dCirc(x, y - 17, 2, RGB(0xff5a5a)); dCirc(x + 11, y - 19, 2, RGB(0xffe65c)); break;
    case D_MOON:
      dCirc(x, y - 22, 15, RGB(0xfff0b0)); dCirc(x - 5, y - 25, 3.5f, RGB(0xe0cf90)); dCirc(x + 5, y - 17, 3, RGB(0xe0cf90)); dCirc(x + 6, y - 27, 2, RGB(0xe0cf90)); break;
    case D_GEM:
      dTri(x - 15, y - 26, x + 15, y - 26, x, y - 2, RGB(0x3ee0ff)); dTri(x - 15, y - 26, x - 8, y - 38, x + 8, y - 38, RGB(0x8af0ff)); dTri(x - 15, y - 26, x + 8, y - 38, x + 15, y - 26, RGB(0x8af0ff));
      dTri(x - 15, y - 26, x, y - 26, x - 4, y - 8, RGBA(WHITE, 90)); dLine(x - 15, y - 26, x + 15, y - 26, 1.5f, RGBA(WHITE, 190)); dLine(x - 4, y - 26, x - 8, y - 38, 1.5f, RGBA(WHITE, 190));
      dCirc(x - 6, y - 32, 2, RGB(WHITE)); break;
    case D_CROWN:
      dRect(x - 15, y - 15, 30, 11, RGB(0xffd41f)); dTri(x - 15, y - 15, x - 11, y - 32, x - 4, y - 15, RGB(0xffd41f));
      dTri(x - 6, y - 15, x, y - 36, x + 6, y - 15, RGB(0xffd41f)); dTri(x + 4, y - 15, x + 11, y - 32, x + 15, y - 15, RGB(0xffd41f));
      dRect(x - 15, y - 6, 30, 2, RGB(0xc78f00)); dCirc(x - 8, y - 10, 2.5f, RGB(0xe8232f)); dCirc(x, y - 10, 2.5f, RGB(0x3ee0ff)); dCirc(x + 8, y - 10, 2.5f, RGB(0xe8232f)); break;
    case D_BAG:
      dTri(x - 6, y - 25, x + 6, y - 25, x, y - 36, RGB(0xb89040)); dCirc(x, y - 14, 14, RGB(0xc8a050)); dRect(x - 6, y - 26, 12, 3, RGB(0x8a5f00));
      dCirc(x, y - 14, 7, RGB(0xffd41f)); dRect(x - 1, y - 19, 2, 10, RGB(0x8a5f00)); break;
    case D_TROPHY:
      dRing(x - 12, y - 28, 5, 6, 2.5f, RGB(0xe0a800), 12); dRing(x + 12, y - 28, 5, 6, 2.5f, RGB(0xe0a800), 12);
      dTri(x - 11, y - 38, x + 11, y - 38, x + 6, y - 18, RGB(0xffd41f)); dTri(x - 11, y - 38, x + 6, y - 18, x - 6, y - 18, RGB(0xffd41f));
      dRect(x - 2.5f, y - 18, 5, 8, RGB(0xe0a800)); dRect(x - 9, y - 10, 18, 5, RGB(0xb88400)); dRect(x - 8, y - 35, 3, 12, RGBA(WHITE, 130)); break;
    default: break;
  }
}
#endif
