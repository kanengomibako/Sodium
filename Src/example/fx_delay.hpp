#ifndef FX_DELAY_HPP
#define FX_DELAY_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_filter.hpp"
#include "lib_delay.hpp"

class fx_delay : public fx_base
{
private:
  enum paramName {DTIME, ELEVEL, FBACK, TONE, OUTPUT,
      P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19};
  float param[20] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  const int16_t paramMax[20] = {150,100, 99,100,100,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  const int16_t paramMin[20] = {  1,  0,  0,  0,  0,  0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  const string paramName[20] = {
      "D.TIME", "E.LEVEL", "F.BACK",
			"TONE", "OUTPUT",
			"","","","","","","","","","","","","","",""};
  const uint8_t paramIndexMax = 4;

  // 最大ディレイタイム 16bit モノラルで2.5秒程度まで
  const float maxDelayTime = 1500.0f;

  signalSw bypassIn, bypassOut;
  delayBuf del1;
  lpf2nd lpf2ndTone;

public:
  fx_delay()
  {
    fxNameList[DD] = "DELAY";
    fxColorList[DD] = 0b1111100000011111; // 赤青
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

    del1.set(maxDelayTime); // 最大ディレイタイム設定
  }

  virtual void deinit()
  {
    del1.erase();
  }

  virtual void setParamStr(uint8_t paramIndex)
  {
    switch(paramIndex)
    {
      case DTIME:
        fxParamStr[DTIME] = std::to_string(fxParam[DTIME]) + "0";
        break;
      case ELEVEL:
        fxParamStr[ELEVEL] = std::to_string(fxParam[ELEVEL]);
        break;
      case FBACK:
        fxParamStr[FBACK] = std::to_string(fxParam[FBACK]);
        break;
      case TONE:
        fxParamStr[TONE] = std::to_string(fxParam[TONE]);
        break;
      case OUTPUT:
        fxParamStr[OUTPUT] = std::to_string(fxParam[OUTPUT]);
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
        param[DTIME] = (float)fxParam[DTIME] * 10.0f; // DELAYTIME 10 ～ 1500 ms
        break;
      case 1:
        param[ELEVEL] = logPot(fxParam[ELEVEL], -20.0f, 20.0f);  // EFFECT LEVEL -20 ～ +20dB
        break;
      case 2:
        param[FBACK] = (float)fxParam[FBACK] / 100.0f; // Feedback 0 ～ 0.99 %
        break;
      case 3:
        param[TONE] = 800.0f * logPot(fxParam[TONE], 0.0f, 20.0f); // HI CUT FREQ 800 ～ 8000 Hz
        break;
      case 4:
        param[OUTPUT] = logPot(fxParam[OUTPUT], -20.0f, 20.0f);  // OUTPUT LEVEL -20 ～ +20dB
        break;
      case 5:
        lpf2ndTone.set(param[TONE]);
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
      fxL[i] = del1.read(param[DTIME]); // ディレイ音読み込み
      fxL[i] = lpf2ndTone.process(fxL[i]); // ディレイ音のTONE（ハイカット）

      // ディレイ音と原音をディレイバッファに書き込み、原音はエフェクトオン時のみ書き込む
      del1.write(bypassIn.process(0.0f, xL[i], fxOn) + param[FBACK] * fxL[i]);

      fxL[i] = param[OUTPUT] * (xL[i] + fxL[i] * param[ELEVEL]); // マスターボリューム ディレイ音レベル
      xL[i] = bypassOut.process(xL[i], fxL[i], fxOn);
    }
  }
};

#endif // FX_DELAY_HPP
