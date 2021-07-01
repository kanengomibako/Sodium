#ifndef FX_REVERB_HPP
#define FX_REVERB_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_filter.hpp"
#include "lib_delay.hpp"

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

  // ディレイタイム配列
  const float dt[10] = {43.5337, 25.796, 19.392, 16.364, 7.645, 4.2546, 58.6435, 69.4325, 74.5234, 86.1244};

  signalSw bypassIn, bypassOutL, bypassOutR;
  delayBufF del[10];
  lpf lpfIn, lpfFB[4];
  hpf hpfOutL, hpfOutR;

public:
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

    for (int i = 0; i < 10; i++) del[i].set(dt[i]); // ディレイタイム設定
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
    setParam();

    for (uint16_t i = 0; i < BLOCK_SIZE; i++)
    {
      float fxL, fxR; // fxR(Rch) 不使用

      float ap, am, bp, bm, cp, cm, dp, dm, ep, em,
      fp, fm, gp, gm, hd, id, jd, kd, outL, outR;

      fxL = bypassIn.process(0.0f, xL[i], fxOn);
      fxL = 0.25f * lpfIn.process(fxL);

      // Early Reflection

      del[0].write(fxL);
      ap = fxL + del[0].readLerp(dt[0]);
      am = fxL - del[0].readLerp(dt[0]);
      del[1].write(am);
      bp = ap + del[1].readLerp(dt[1]);
      bm = ap - del[1].readLerp(dt[1]);
      del[2].write(bm);
      cp = bp + del[2].readLerp(dt[2]);
      cm = bp - del[2].readLerp(dt[2]);
      del[3].write(cm);
      dp = cp + del[3].readLerp(dt[3]);
      dm = cp - del[3].readLerp(dt[3]);
      del[4].write(dm);
      ep = dp + del[4].readLerp(dt[4]);
      em = dp - del[4].readLerp(dt[4]);
      del[5].write(em);

      // Late Reflection & High Freq Dumping

      hd = del[6].readLerp(dt[6]);
      hd = lpfFB[0].process(hd);

      id = del[7].readLerp(dt[7]);
      id = lpfFB[1].process(id);

      jd = del[8].readLerp(dt[8]);
      jd = lpfFB[2].process(jd);

      kd = del[9].readLerp(dt[9]);
      kd = lpfFB[3].process(kd);

      outL = ep + hd * param[FBACK];
      outR = del[5].readLerp(dt[5]) + id * param[FBACK];

      fp = outL + outR;
      fm = outL - outR;
      gp = jd * param[FBACK] + kd * param[FBACK];
      gm = jd * param[FBACK] - kd * param[FBACK];
      del[6].write(fp + gp);
      del[7].write(fm + gm);
      del[8].write(fp - gp);
      del[9].write(fm - gm);

      fxL = (1.0f - param[MIX]) * xL[i] + param[MIX] * hpfOutL.process(outL);
      fxR = (1.0f - param[MIX]) * xL[i] + param[MIX] * hpfOutR.process(outR);

      xL[i] = bypassOutL.process(xL[i], param[LEVEL] * fxL, fxOn);
      xR[i] = bypassOutR.process(xR[i], param[LEVEL] * fxR, fxOn);
    }
  }

};

#endif // FX_REVERB_HPP
