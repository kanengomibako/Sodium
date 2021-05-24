#ifndef FX_BQFILTER_HPP
#define FX_BQFILTER_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_filter.hpp"

class fx_bqfilter : public fx_base
{
private:
  const string name = "BQ FILTER";
  const uint16_t color = COLOR_BG; // 青緑
  const string paramName[20] = {"LV", "TYPE", "Fc", "Q", "dB"};
  enum paramName {LEVEL, TYPE, FREQ, Q, GAIN};
  float param[20] = {1, 1, 1, 1, 1};
  const int16_t paramMax[20] = { 20,  8,999, 99, 15};
  const int16_t paramMin[20] = {-20,  0,  2,  1,-15};
  const uint8_t paramNumMax = 5;

  const string typeName[9] = {"OFF","PF","LSF","HSF","LPF","HPF","BPF","NF","APF"};
  signalSw bypass;
  biquadFilter bqf1;

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
  }

  virtual void deinit()
  {
  }

  virtual void setParamStr(uint8_t paramNum)
  {
    switch(paramNum)
    {
      case 0:
        fxParamStr[LEVEL] = std::to_string(fxParam[LEVEL]);
        if (fxParam[LEVEL] > 0) fxParamStr[LEVEL] = "+" + fxParamStr[LEVEL];
        break;
      case 1:
        fxParamStr[TYPE] = typeName[fxParam[TYPE]];
        break;
      case 2:
        fxParamStr[FREQ] = std::to_string(fxParam[FREQ] * 10);
        break;
      case 3:
        fxParamStr[Q] = std::to_string(fxParam[Q]);
        if (fxParam[Q] < 10) fxParamStr[Q].insert(0,"0.");
        else fxParamStr[Q].insert(1,".");
        break;
      case 4:
        fxParamStr[GAIN] = std::to_string(fxParam[GAIN]);
        if (fxParam[GAIN] > 0) fxParamStr[GAIN] = "+" + fxParamStr[GAIN];
        break;
      default:
        fxParamStr[paramNum] = "";
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
        param[LEVEL] = dbToGain(fxParam[LEVEL]); // LEVEL -20...+20 dB
        break;
      case 1:
        param[TYPE] = (float)fxParam[TYPE]; // フィルタタイプ
        break;
      case 2:
        param[FREQ] = (float)fxParam[FREQ] * 10.0f; // フィルタ 周波数 20...9990 Hz
        break;
      case 3:
        param[Q] = (float)fxParam[Q] * 0.1f; // フィルタ Q 0.1...9.9
        break;
      case 4:
        param[GAIN] = (float)fxParam[GAIN]; // フィルタ GAIN -15...+15 dB
        break;
      case 5:
        bqf1.setCoef(fxParam[TYPE], param[FREQ], param[Q], param[GAIN]); // フィルタ 係数設定
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
      float fxL = xL[i];
      fxL = bqf1.process(fxL); // フィルタ実行
      fxL = param[LEVEL] * fxL; // LEVEL
      xL[i] = bypass.process(xL[i], fxL, fxOn);
    }
  }

};

#endif // FX_BQFILTER_HPP
