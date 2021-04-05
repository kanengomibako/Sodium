#ifndef FX_REVERB_HPP
#define FX_REVERB_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_filter.hpp"
#include "lib_delayPrimeNum.hpp"

class fx_reverb : public fx_base
{
private:
  const string name = "REVERB"; // Pure Data [rev2~] に基づいたFDNリバーブ
  const uint16_t color = COLOR_RGB; // 赤緑青
  const string paramName[20] = {"LEVEL", "MIX", "F.BACK", "HiCUT", "LoCUT", "HiDUMP"};
  enum paramName {LEVEL, MIX, FBACK, HICUT, LOCUT, HIDUMP};
  float param[20] = {1, 1, 1, 1, 1, 1};
  const int16_t paramMax[20] = {100,100, 99,100,100,100};
  const int16_t paramMin[20] = {  0,  0,  0,  0,  0,  0};
  const uint8_t paramNumMax = 6;

  const uint8_t dt[10] = {44, 26, 19, 16, 8, 4, 59, 69, 75, 86}; // ディレイタイム配列

  signalSw bypassIn, bypassOutL, bypassOutR;
  delayBufPrimeNum del[10];
  lpf lpfIn, lpfFB[4];
  hpf hpfOutL, hpfOutR;

public:
  fx_reverb()
  {
  }

  virtual void init()
  {
    fxName = name;
    fxColor = color;
    fxParamNumMax = paramNumMax;
    for (int i = 0; i < 20; i++)
    {
      fxParamName[i] = paramName[i];
      fxParamMax[i] = paramMax[i];
      fxParamMin[i] = paramMin[i];
      if (fxAllData[fxNum][i] < paramMin[i] || fxAllData[fxNum][i] > paramMax[i]) fxParam[i] = paramMin[i];
      else fxParam[i] = fxAllData[fxNum][i];
    }

    for (int i = 0; i < 10; i++) del[i].set(dt[i]); // 最大ディレイタイム設定
  }

  virtual void deinit()
  {
    for (int i = 0; i < 10; i++) del[i].erase();
  }

  virtual void setParamStr(uint8_t paramNum)
  {
    switch(paramNum)
    {
      case 0:
        fxParamStr[LEVEL] = std::to_string(fxParam[LEVEL]);
        break;
      case 1:
        fxParamStr[MIX] = std::to_string(fxParam[MIX]);
        break;
      case 2:
        fxParamStr[FBACK] = std::to_string(fxParam[FBACK]);
        break;
      case 3:
        fxParamStr[HICUT] = std::to_string(fxParam[HICUT]);
        break;
      case 4:
        fxParamStr[LOCUT] = std::to_string(fxParam[LOCUT]);
        break;
      case 5:
        fxParamStr[HIDUMP] = std::to_string(fxParam[HIDUMP]);
        break;
      default:
        fxParamStr[paramNum] = "";
        break;
    }
  }

  virtual void setParam()
  {
    static uint8_t count = 0;
    count = (count + 1) % 13; // 負荷軽減のためパラメータ計算を分散させる
    switch(count)
    {
      case 0:
        param[LEVEL] = logPot(fxParam[LEVEL], -20.0f, 20.0f);  // LEVEL -20 ～ +20dB
        break;
      case 1:
        param[MIX] = mixPot(fxParam[MIX], -20.0f); // MIX
        break;
      case 2:
        param[FBACK] = (float)fxParam[FBACK] / 200.0f; // Feedback 0～0.495
        break;
      case 3:
        param[HICUT] = 600.0f * logPot(fxParam[HICUT], 20.0f, 0.0f); // HI CUT FREQ 600 ~ 6000 Hz
        break;
      case 4:
        param[LOCUT] = 100.0f * logPot(fxParam[LOCUT], 0.0f, 20.0f); // LOW CUT FREQ 100 ~ 1000 Hz
        break;
      case 5:
        param[HIDUMP] = 600.0f * logPot(fxParam[HIDUMP], 20.0f, 0.0f); // Feedback HI CUT FREQ 600 ~ 6000 Hz
        break;
      case 6:
        lpfIn.set(param[HICUT]);
        break;
      case 7:
        hpfOutL.set(param[LOCUT]);
        break;
      case 8:
        hpfOutR.set(param[LOCUT]);
        break;
      case 9:
        lpfFB[0].set(param[HIDUMP]);
        break;
      case 10:
        lpfFB[1].set(param[HIDUMP]);
        break;
      case 11:
        lpfFB[2].set(param[HIDUMP]);
        break;
      case 12:
        lpfFB[3].set(param[HIDUMP]);
        break;
      default:
        break;
    }
  }

  virtual void process(float xL[], float xR[])
  {
    float fxL[BLOCK_SIZE] = {};
    float fxR[BLOCK_SIZE] = {}; // Rch 不使用

    float ap, am, bp, bm, cp, cm, dp, dm, ep, em,
    fp, fm, gp, gm, hd, id, jd, kd, out_l, out_r;

    setParam();

    for (uint16_t i = 0; i < BLOCK_SIZE; i++)
    {
      fxL[i] = bypassIn.process(0.0f, xL[i], fxOn);
      fxL[i] = 0.25f * lpfIn.process(fxL[i]);

      // Early Reflection

      del[0].write(fxL[i]);
      ap = fxL[i] + del[0].readFixed();
      am = fxL[i] - del[0].readFixed();
      del[1].write(am);
      bp = ap + del[1].readFixed();
      bm = ap - del[1].readFixed();
      del[2].write(bm);
      cp = bp + del[2].readFixed();
      cm = bp - del[2].readFixed();
      del[3].write(cm);
      dp = cp + del[3].readFixed();
      dm = cp - del[3].readFixed();
      del[4].write(dm);
      ep = dp + del[4].readFixed();
      em = dp - del[4].readFixed();
      del[5].write(em);

      // Late Reflection & High Freq Dumping

      hd = del[6].readFixed();
      hd = lpfFB[0].process(hd);

      id = del[7].readFixed();
      id = lpfFB[1].process(id);

      jd = del[8].readFixed();
      jd = lpfFB[2].process(jd);

      kd = del[9].readFixed();
      kd = lpfFB[3].process(kd);

      out_l = ep + hd * param[FBACK];
      out_r = del[5].readFixed() + id * param[FBACK];

      fp = out_l + out_r;
      fm = out_l - out_r;
      gp = jd * param[FBACK] + kd * param[FBACK];
      gm = jd * param[FBACK] - kd * param[FBACK];
      del[6].write(fp + gp);
      del[7].write(fm + gm);
      del[8].write(fp - gp);
      del[9].write(fm - gm);

      fxL[i] = (1.0f - param[MIX]) * xL[i] + param[MIX] * hpfOutL.process(out_l);
      fxR[i] = (1.0f - param[MIX]) * xL[i] + param[MIX] * hpfOutR.process(out_r);

      xL[i] = bypassOutL.process(xL[i], param[LEVEL] * fxL[i], fxOn);
      xR[i] = bypassOutR.process(xR[i], param[LEVEL] * fxR[i], fxOn);
    }
  }

};

#endif // FX_REVERB_HPP
