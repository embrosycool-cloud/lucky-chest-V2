// Pirate Chest 3DS - audio synthetise
#ifndef AUDIO_H
#define AUDIO_H
#include "common.h"

#define SR     22050
#define ABUF   1024
#define NWB    4
#define NVOICE 40
#define NIL    999

enum { SFX_CLICK=0, SFX_TAB, SFX_START, SFX_BUY, SFX_DENY, SFX_EQUIP, SFX_STEP, SFX_COIN, SFX_OPEN,
       SFX_SHAKE, SFX_LUCKY, SFX_JACKPOT, SFX_FINISH, SFX_QUIT };
enum { W_SINE=0, W_SQUARE, W_TRI, W_SAW, N_WHITE, N_LOW, N_HIGH };

typedef struct { int on, wave, bus, delay; float f, f2, ph, t, dur, vol, lp, fc, fc2; } Voice;

static Voice vs[NVOICE];
static ndspWaveBuf wbuf[NWB];
static s16 *abufmem = NULL;
static int audioOK = 0;
static int trackWant = 0, trackCur = -1, seqStep = 0;
static float seqCnt = 0;
static unsigned noiseSeed = 12345;

static float hz(float n) { return 440.0f * powf(2.0f, n / 12.0f); }
static float nrand(void) { noiseSeed = noiseSeed * 1664525u + 1013904223u; return ((noiseSeed >> 8) & 0xffff) / 32768.0f - 1.0f; }

static void vAdd(int bus, int wave, float f, float dur, float vol, float delay, float f2, float fc2) {
  int i, k = 0; Voice *v;
  for (i = 0; i < NVOICE; i++) if (!vs[i].on) { k = i; break; }
  if (i == NVOICE) k = 0;
  v = &vs[k];
  memset(v, 0, sizeof *v);
  v->on = 1; v->wave = wave; v->bus = bus; v->f = f; v->f2 = f2 > 0 ? f2 : f;
  v->dur = dur; v->vol = vol; v->delay = (int)(delay * SR); v->fc = f; v->fc2 = fc2 > 0 ? fc2 : f;
}
#define TONE(w, f, d, vol, dl, f2)  vAdd(0, w, f, d, vol, dl, f2, 0)
#define NOISE(m, d, vol, dl, f1, f2) vAdd(0, m, f1, d, vol, dl, 0, f2)

static void sfxPlay(int n) {
  int i;
  if (!audioOK || S.mute) return;
  switch (n) {
    case SFX_CLICK: TONE(W_SQUARE, 700, .07f, .12f, 0, 1000); break;
    case SFX_TAB:   TONE(W_TRI, 500, .09f, .3f, 0, 780); break;
    case SFX_START: { static const float fq[5] = {330, 440, 554, 659, 880}; for (i = 0; i < 5; i++) TONE(W_SQUARE, fq[i], .16f, .14f, i * .06f, 0); } break;
    case SFX_BUY:   TONE(W_SQUARE, 988, .1f, .14f, 0, 0); TONE(W_SQUARE, 1319, .28f, .14f, .08f, 0); break;
    case SFX_DENY:  TONE(W_SAW, 180, .15f, .2f, 0, 120); TONE(W_SAW, 150, .22f, .2f, .12f, 90); break;
    case SFX_EQUIP: TONE(W_TRI, 440, .1f, .35f, 0, 0); TONE(W_TRI, 660, .16f, .35f, .08f, 0); break;
    case SFX_STEP:  NOISE(N_LOW, .06f, .5f, 0, 260 + rnd(0, 220), 0); break;
    case SFX_COIN:  TONE(W_SQUARE, 1319, .07f, .13f, 0, 0); TONE(W_SQUARE, 1760, .2f, .13f, .06f, 0); break;
    case SFX_OPEN:  NOISE(N_LOW, .4f, .35f, 0, 250, 950); TONE(W_SINE, 90, .18f, .6f, .32f, 45); break;
    case SFX_SHAKE: TONE(W_SAW, 180, .6f, .07f, 0, 360); NOISE(N_LOW, .6f, .15f, 0, 400, 200); break;
    case SFX_LUCKY: { static const float fq[4] = {523, 659, 784, 1047}; for (i = 0; i < 4; i++) { TONE(W_SQUARE, fq[i], .18f, .14f, i * .07f, 0); TONE(W_TRI, fq[i] * 2, .14f, .1f, i * .07f, 0); } } break;
    case SFX_JACKPOT: { static const float fq[7] = {523, 659, 784, 1047, 1319, 1568, 2093}; for (i = 0; i < 7; i++) { TONE(W_SQUARE, fq[i], .28f, .14f, i * .08f, 0); TONE(W_TRI, fq[i] * 1.5f, .2f, .1f, i * .08f, 0); } NOISE(N_HIGH, .9f, .18f, .3f, 5000, 0); } break;
    case SFX_FINISH: { static const float fq[6] = {523, 523, 784, 659, 784, 1047}, tm[6] = {0, .15f, .3f, .5f, .65f, .85f}; for (i = 0; i < 6; i++) { TONE(W_SQUARE, fq[i], .3f, .15f, tm[i], 0); TONE(W_TRI, fq[i] / 2, .3f, .2f, tm[i], 0); } } break;
    case SFX_QUIT:  TONE(W_TRI, 400, .22f, .3f, 0, 200); break;
  }
}

static const int MEL_LOBBY[48] = {0,NIL,0,3,NIL,3, 7,NIL,7,5,NIL,3, 0,NIL,0,3,NIL,5, 7,NIL,5,3,NIL,0,
                                  5,NIL,5,7,NIL,8, 10,NIL,8,7,NIL,5, 3,NIL,7,5,NIL,3, 0,NIL,NIL,0,NIL,NIL};
static const int MEL_GAME[48]  = {0,0,3,0,5,3, 7,7,10,7,5,3, 0,0,3,0,5,7, 8,7,5,3,NIL,0,
                                  5,5,8,5,7,5, 3,3,7,3,5,3, 2,2,5,2,3,2, 0,NIL,3,NIL,0,NIL};
static const int BASS[8] = {-24, -24, -26, -24, -28, -26, -29, -24};

static void musicStep(void) {
  int i = seqStep % 48, b = i % 6, game = trackCur == 1;
  const int *mel = game ? MEL_GAME : MEL_LOBBY;
  float d = game ? .16f : .28f;
  if (mel[i] != NIL) vAdd(1, game ? W_SQUARE : W_TRI, hz((float)mel[i]), d * 1.7f, game ? .2f : .45f, 0, 0, 0);
  if (b == 0 || b == 3) vAdd(1, W_TRI, hz((float)(BASS[(i / 6) & 7] + (b ? 7 : 0))), d * 2.4f, .55f, 0, 0, 0);
  if (game) {
    if (b == 0) vAdd(1, W_SINE, 130, .14f, .9f, 0, 40, 0);
    else if (b == 3) vAdd(1, N_HIGH, 1800, .12f, .5f, 0, 0, 0);
    else vAdd(1, N_HIGH, 7000, .04f, .15f, 0, 0, 0);
  }
  seqStep++;
}
static void musicSet(int t) { trackWant = t; }

static void audioFill(s16 *out, int n) {
  int i, k;
  for (i = 0; i < n; i++) {
    float mix = 0;
    if (!S.mute) {
      if (trackCur != trackWant) { trackCur = trackWant; seqStep = 0; seqCnt = 0; }
      seqCnt -= 1.0f;
      if (seqCnt <= 0) { musicStep(); seqCnt += (trackCur == 1 ? .16f : .28f) * SR; }
      for (k = 0; k < NVOICE; k++) {
        Voice *v = &vs[k]; float e, x, u, a;
        if (!v->on) continue;
        if (v->delay > 0) { v->delay--; continue; }
        if (v->t >= v->dur) { v->on = 0; continue; }
        u = v->t / v->dur;
        e = v->t < .012f ? v->t / .012f : (1 - (v->t - .012f) / (v->dur - .012f));
        if (e < 0) e = 0;
        e *= e;
        if (v->wave >= N_WHITE) {
          float r = nrand();
          float fc = v->fc + (v->fc2 - v->fc) * u;
          a = 1.0f - expf(-2.0f * PI_F * fc / SR);
          v->lp += a * (r - v->lp);
          x = v->wave == N_WHITE ? r : (v->wave == N_LOW ? v->lp * 1.6f : r - v->lp);
        } else {
          float f = v->f + (v->f2 - v->f) * u;
          v->ph += f / SR; if (v->ph >= 1) v->ph -= 1;
          switch (v->wave) {
            case W_SINE:   x = sinf(v->ph * 2 * PI_F); break;
            case W_SQUARE: x = v->ph < .5f ? .6f : -.6f; break;
            case W_TRI:    x = v->ph < .5f ? (4 * v->ph - 1) : (3 - 4 * v->ph); break;
            default:       x = 2 * v->ph - 1; break;
          }
        }
        mix += x * e * v->vol * (v->bus ? .17f : .6f);
        v->t += 1.0f / SR;
      }
    }
    mix *= 3.0f;
    if (mix > 1) mix = 1; else if (mix < -1) mix = -1;
    out[i] = (s16)(mix * 30000);
  }
}

static void audioInit(void) {
  int i;
  if (R_FAILED(ndspInit())) return;
  ndspSetOutputMode(NDSP_OUTPUT_STEREO);
  ndspChnReset(0);
  ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
  ndspChnSetRate(0, (float)SR);
  ndspChnSetFormat(0, NDSP_FORMAT_MONO_PCM16);
  { float mx[12]; memset(mx, 0, sizeof mx); mx[0] = 1.0f; mx[1] = 1.0f; ndspChnSetMix(0, mx); }
  abufmem = (s16 *)linearAlloc(ABUF * NWB * sizeof(s16));
  if (!abufmem) { ndspExit(); return; }
  memset(abufmem, 0, ABUF * NWB * sizeof(s16));
  memset(wbuf, 0, sizeof wbuf);
  for (i = 0; i < NWB; i++) { wbuf[i].data_pcm16 = abufmem + i * ABUF; wbuf[i].nsamples = ABUF; wbuf[i].status = NDSP_WBUF_DONE; }
  audioOK = 1;
}
static void audioPump(void) {
  int i;
  if (!audioOK) return;
  for (i = 0; i < NWB; i++) {
    if (wbuf[i].status == NDSP_WBUF_DONE) {
      audioFill(wbuf[i].data_pcm16, ABUF);
      DSP_FlushDataCache(wbuf[i].data_pcm16, ABUF * sizeof(s16));
      ndspChnWaveBufAdd(0, &wbuf[i]);
    }
  }
}
static void audioExit(void) {
  if (!audioOK) return;
  ndspChnReset(0); ndspExit();
  if (abufmem) linearFree(abufmem);
  audioOK = 0;
}
#endif
