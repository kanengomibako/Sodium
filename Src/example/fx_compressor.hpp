#ifndef FX_COMPRESSOR_HPP
#define FX_COMPRESSOR_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_filter.hpp"

class fx_compressor : public fx_base
{
private:
  const string name = "COMPRESSOR";
  const uint16_t color = COLOR_RGB; // 白
  const string paramName[20] = {"LEVEL", "THRE", "RATIO", "ATK", "REL", "KNEE"};
  enum paramName {LEVEL, THRESHOLD, RATIO, ATTACK, RELEASE, KNEE};
  float param[20] = {1, 1, 1, 1, 1, 1};
  const int16_t paramMax[20] = {100,  0, 11,100,400, 20};
  const int16_t paramMin[20] = {  0,-90,  2,  4,  5,  0};
  const uint8_t paramNumMax = 6;

  signalSw bypass;
  lpf lpfEnv;    // エンベロープ用ローパスフィルタ
  lpf lpfAtkRel; // アタック・リリース用ローパスフィルタ

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

    lpfEnv.set(500.0f); // エンベロープ用ローパスフィルタ設定
    lpfAtkRel.set(100.0f);
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
        break;
      case 1:
        fxParamStr[THRESHOLD] = std::to_string(fxParam[THRESHOLD]);
        break;
      case 2:
        if (fxParam[RATIO] > 10) fxParamStr[RATIO] = "Inf";
        else fxParamStr[RATIO] = std::to_string(fxParam[RATIO]);
        break;
      case 3:
        fxParamStr[ATTACK] = std::to_string(fxParam[ATTACK]);
        break;
      case 4:
        fxParamStr[RELEASE] = std::to_string(fxParam[RELEASE]);
        break;
      case 5:
        fxParamStr[KNEE] = std::to_string(fxParam[KNEE]);
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
        param[LEVEL] = logPot(fxParam[LEVEL], 0.0f, 30.0f); // LEVEL 0～+30dB
        break;
      case 1:
        param[THRESHOLD] = (float)fxParam[THRESHOLD]; // THRESHOLD -90～0dB
        break;
      case 2:
        if (fxParam[RATIO] > 10) param[RATIO] = 0.0f;     // RATIO 1:Infinity
        else param[RATIO] = 1.0f / (float)fxParam[RATIO]; //       1:2～1:10
        break;
      case 3:
        param[ATTACK] = 100.0f / (float)fxParam[ATTACK]; // ATTACK 4～100ms(LPF周波数1～25Hz)
        break;
      case 4:
        param[RELEASE] = 400.0f / (float)fxParam[RELEASE]; // RELEASE 5～400ms(LPF周波数1～80Hz)
        break;
      case 5:
        param[KNEE] = (float)fxParam[KNEE]; // KNEE 0～20dB
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
      float th = param[THRESHOLD]; // スレッショルド 一時変数
      float dbGain = 0.0f; // コンプレッション(音量圧縮)幅 dB

      float env = abs(fxL);      // エンベロープ
      env = lpfEnv.process(env); // 絶対値とLPFでエンベロープ取得
      env = gainToDb(env);       // dB換算

      if (env < th - param[KNEE])
      {
        dbGain = 0.0f;
        lpfAtkRel.set(param[RELEASE]);      // リリース用LPF周波数を設定
        dbGain = lpfAtkRel.process(dbGain); // リリース用LPFで音量変化を遅らせる
      }
      else
      {
        if (env < th + param[KNEE]) // THRESHOLD ± KNEE の範囲では2次関数の曲線を使用
        {
          th = -0.25f / param[KNEE] * (env - th - param[KNEE]) * (env - th - param[KNEE]) + th;
        }
        dbGain = (env - th) * param[RATIO] + th - env; // 音量圧縮幅計算
        lpfAtkRel.set(param[ATTACK]);       // アタック用LPF周波数を設定
        dbGain = lpfAtkRel.process(dbGain); // アタック用LPFで音量変化を遅らせる
      }

      fxL = dbToGain(dbGain) * fxL; // コンプレッション実行
      fxL = param[LEVEL] * fxL;     // LEVEL

      xL[i] = bypass.process(xL[i], fxL, fxOn);
    }

  }

};

#endif // FX_COMPRESSOR_HPP
