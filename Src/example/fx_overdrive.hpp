#ifndef FX_OVERDRIVE_HPP
#define FX_OVERDRIVE_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_filter.hpp"

class fx_overdrive : public fx_base
{
private:
  enum paramName {LEVEL, GAIN, TREBLE, BASS,
    P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19};
  float param[20] = {0.0f, 0.0f, 0.0f, 0.0f,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  const int16_t paramMax[20] = {100,100,100,100,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  const int16_t paramMin[20] = {  0,  0,  0,  0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  const string paramName[20] = {
      "LEVEL", "GAIN", "TREBLE",
      "BASS",
      "","","","","","","","","","","","","","","",""};
  const uint8_t paramIndexMax = 3;

  signalSw bypass;
  hpf hpfFixed, hpfBass;
  lpf lpfFixed, lpfTreble;

public:
  fx_overdrive()
  {
    fxNameList[OD] = "OVERDRIVE";
    fxColorList[OD] = 0b1111111111100000; // 赤緑
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

    hpfFixed.set(10.0f);
    lpfFixed.set(5000.0f);
  }

  virtual void deinit()
  {
  }

  virtual void setParamStr(uint8_t paramIndex)
  {
    switch(paramIndex)
    {
      case LEVEL:
        fxParamStr[LEVEL] = std::to_string(fxParam[LEVEL]);
        break;
      case GAIN:
        fxParamStr[GAIN] = std::to_string(fxParam[GAIN]);
        break;
      case TREBLE:
        fxParamStr[TREBLE] = std::to_string(fxParam[TREBLE]);
        break;
      case BASS:
        fxParamStr[BASS] = std::to_string(fxParam[BASS]);
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
        param[LEVEL] = logPot(fxParam[LEVEL], -40.0f, 10.0f);  // LEVEL -40...10 dB
        break;
      case 1:
        param[GAIN] = logPot(fxParam[GAIN], -6.0f, 40.0f); // GAIN -6...+40 dB
        break;
      case 2:
        param[TREBLE] = 10000.0f * logPot(fxParam[TREBLE], -28.0f, 0.0f); // TREBLE LPF 400 ~ 10k Hz
        break;
      case 3:
        param[BASS] = 1000.0f * logPot(fxParam[BASS], 0.0f, -20.0f); // BASS HPF 100 ~ 1000 Hz
        break;
      case 4:
        lpfTreble.set(param[TREBLE]);
        break;
      case 5:
        hpfBass.set(param[BASS]);
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
      fxL[i] = hpfBass.process(xL[i]); // BASS
      fxL[i] = lpfFixed.process(fxL[i]); // 高域カット
  	  fxL[i] = 5.0f * fxL[i]; // 初期固定ゲイン

  	  if (fxL[i] < -0.5f) fxL[i] = -0.25f; // 2次関数による波形の非対称変形
  	  else fxL[i] = fxL[i] * fxL[i] + fxL[i];

  	  fxL[i] = param[GAIN] * hpfFixed.process(fxL[i]); // GAIN、直流カット

  	  if (fxL[i] < -1.0f) fxL[i] = -1.0f; // 2次関数による対称ソフトクリップ
  	  else if (fxL[i] < 0.0f) fxL[i] = fxL[i] * fxL[i] + 2.0f * fxL[i];
      else if (fxL[i] < 1.0f) fxL[i] = 2.0f * fxL[i] - fxL[i] * fxL[i];
      else fxL[i] = 1.0f;

  	  fxL[i] = param[LEVEL] * lpfTreble.process(fxL[i]); // LEVEL, TREBLE

      xL[i] = bypass.process(xL[i], fxL[i], sw[4]);
    }
  }

};

#endif // FX_OVERDRIVE_HPP
