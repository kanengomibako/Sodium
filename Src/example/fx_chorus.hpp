#ifndef FX_CHORUS_HPP
#define FX_CHORUS_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_filter.hpp"
#include "lib_osc.hpp"

class fx_chorus : public fx_base
{
private:
  enum paramName {LEVEL, MIX, FBACK, RATE, DEPTH, TONE,
    P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19};
  float param[20] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  const int16_t paramMax[20] = {100,100, 99,100,100,100,
      1,1,1,1,1,1,1,1,1,1,1,1,1,1};
  const int16_t paramMin[20] = {  0,  0,  0,  0,  0,  0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  const string paramName[20] = {
      "LEVEL", "MIX", "F.BACK",
      "RATE", "DEPTH", "TONE"
      "","","","","","","","","","","","","",""};
  const uint8_t paramIndexMax = 5;

  signalSw bypass;
  triangleWave tri1;
  delayBuf del1;
  hpf hpf1;
  lpf2nd lpf2nd1, lpf2nd2;

public:
  fx_chorus()
  {
    fxNameList[CE] = "CHORUS";
    fxColorList[CE] = 0b0000000000011111; // 青
  }

  virtual void init()
  {
    fxParamIndexMax = paramIndexMax;
    for (int i = 0; i < 20; i++)
    {
      fxParamName[i] = paramName[i];
      fxParamMax[i] = paramMax[i];
      fxParamMin[i] = paramMin[i];
      if (fxAllData[fxNum][i] < paramMin[i] || fxAllData[fxNum][i] > paramMax[i]) fxParam[i] = paramMin[i];
      else fxParam[i] = fxAllData[fxNum][i];
    }

    del1.set(20.0f); // 最大ディレイタイム設定
    hpf1.set(100.0f); // ウェット音のローカット設定
  }

  virtual void deinit()
  {
    del1.erase();
  }

  virtual void setParamStr(uint8_t paramIndex)
  {
    switch(paramIndex)
    {
      case LEVEL:
        fxParamStr[LEVEL] = std::to_string(fxParam[LEVEL]);
        break;
      case MIX:
        fxParamStr[MIX] = std::to_string(fxParam[MIX]);
        break;
      case FBACK:
        fxParamStr[FBACK] = std::to_string(fxParam[FBACK]);
        break;
      case RATE:
        fxParamStr[RATE] = std::to_string(fxParam[RATE]);
        break;
      case DEPTH:
        fxParamStr[DEPTH] = std::to_string(fxParam[DEPTH]);
        break;
      case TONE:
        fxParamStr[TONE] = std::to_string(fxParam[TONE]);
        break;
      default:
        fxParamStr[paramIndex] = "";
        break;
    }
  }

  virtual void setParam()
  {
    static uint8_t count = 0;
    count = (count + 1) % 10; // 負荷軽減のためパラメータ計算を分散させる
    switch(count)
    {
      case 0:
        param[LEVEL] = logPot(fxParam[LEVEL], -20.0f, 20.0f);  // LEVEL -20 ～ 20dB
        break;
      case 1:
        param[MIX] = mixPot(fxParam[MIX], -20.0f);  // MIX
        break;
      case 2:
        param[FBACK] = (float)fxParam[FBACK] / 100.0f; // Feedback 0～0.99
        break;
      case 3:
        param[RATE] = 0.02f * (105.0f - (float)fxParam[RATE]); // Rate 2s
        break;
      case 4:
        param[DEPTH] = 0.1f * (float)fxParam[DEPTH]; // Depth 10ms
        break;
      case 5:
        param[TONE] = 800.0f * logPot(fxParam[TONE], 0.0f, 20.0f); // HI CUT FREQ 800 ～ 8000 Hz
        break;
      case 6:
        lpf2nd1.set(param[TONE]);
        break;
      case 7:
        lpf2nd2.set(param[TONE]);
        break;
      case 8:
        tri1.set(param[RATE]);
        break;
      default:
        break;
    }
  }

  virtual void process(float xL[], float xR[])
  {
    float fxL[BLOCK_SIZE] = {};

    setParam();

    for (uint16_t i = 0; i < BLOCK_SIZE; i++)
    {
      float dtime = param[DEPTH] * tri1.output() + 5.0f; // ディレイタイム5~15ms
      fxL[i] = del1.readLerp(dtime);
      fxL[i] = lpf2nd1.process(fxL[i]);
      fxL[i] = lpf2nd2.process(fxL[i]);
      del1.write(hpf1.process(xL[i]) + param[FBACK] * fxL[i]);
      fxL[i] = (1.0f - param[MIX]) * xL[i] + param[MIX] * fxL[i];
      xL[i] = bypass.process(xL[i], fxL[i] * param[LEVEL], fxOn);
    }
  }

};

#endif // FX_CHORUS_HPP
