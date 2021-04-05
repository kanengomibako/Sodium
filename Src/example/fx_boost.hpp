#ifndef FX_BOOST_HPP
#define FX_BOOST_HPP

#include "common.h"

// ライブラリ読込
#include "lib_calc.hpp"

class fx_boost : public fx_base // クリーンブースター /////////////////////////////////
{
private:
  // 基本設定 ----------------------------------------------
  const string name = "BOOSTER";      // エフェクト名
  const uint16_t color = COLOR_R;     // エフェクトON時のLED色
  const string paramName[20] = {"G"}; // パラメータ名
  enum paramName {GAIN};              // パラメータ名（配列の添字 識別用）
  float param[20] = {1}; // 信号処理に使うfloatパラメータ(基本編集しない)
  const int16_t paramMax[20] = {20};  // パラメータ設定最大値
  const int16_t paramMin[20] = {-20}; // パラメータ設定最小値
  const uint8_t paramNumMax = 1;      // パラメータ数

  // インスタンス生成 フィルタ等 ---------------------------
  signalSw bypass;

public:
  fx_boost() // コンストラクタ ///////////////////////////////////////////////////////
  {
  }

  virtual void init() // 初期化処理 //////////////////////////////////////////////////
  {
    // 現在のエフェクト用の変数に呼び出したエフェクトの設定を読み込ませる
    fxName = name;
    fxColor = color;
    fxParamNumMax = paramNumMax;
    for (int i = 0; i < 20; i++)
    {
      fxParamName[i] = paramName[i];
      fxParamMax[i] = paramMax[i];
      fxParamMin[i] = paramMin[i];

      // 読み込んだデータが範囲外数値の時、最小値へ変更
      if (fxAllData[fxNum][i] < paramMin[i] || fxAllData[fxNum][i] > paramMax[i]) fxParam[i] = paramMin[i];
      else fxParam[i] = fxAllData[fxNum][i];
    }

    // ディレイタイム最大値やフィルタ周波数の初期値などをここで設定

  }

  virtual void deinit() // 終了処理 /////////////////////////////////////////////////
  {
  }

  virtual void setParamStr(uint8_t paramNum) // パラメータ数値を文字列へ変換 ////////////
  {
    switch(paramNum)
    {
      case 0: // GAINを±●dBで表記
        fxParamStr[GAIN] = std::to_string(fxParam[GAIN]) + "dB";
        if (fxParam[GAIN] > 0) fxParamStr[GAIN] = "+" + fxParamStr[GAIN];
        break;
      case 1:
        fxParamStr[paramNum] = "";
        break;
      case 2:
        fxParamStr[paramNum] = "";
        break;
      case 3:
        fxParamStr[paramNum] = "";
        break;
      case 4:
        fxParamStr[paramNum] = "";
        break;
      case 5:
        fxParamStr[paramNum] = "";
        break;
      default:
        fxParamStr[paramNum] = ""; // 使っていないパラメータの場合は空白にする
        break;
    }
  }

  virtual void setParam() // パラメータ数値を実際の信号処理に使うfloat数値へ変換 //////////
  {
    static uint8_t count = 0;
    count = (count + 1) % 10; // 負荷軽減のためパラメータ計算を分散させる
    switch(count)
    {
      case 0:
        param[GAIN] = dbToGain(fxParam[GAIN]); // GAIN -20...+20 dB
        break;
      case 1:
        param[1] = 0.0f;
        break;
      case 2:
        param[2] = 0.0f;
        break;
      case 3:
        param[3] = 0.0f;
        break;
      case 4:
        param[4] = 0.0f;
        break;
      case 5:
        param[5] = 0.0f;
        break;
      default:
        break;
    }
  }

  virtual void process(float xL[], float xR[]) // エフェクト処理 ///////////////////////
  {
    float fxL[BLOCK_SIZE] = {}; // エフェクト処理を行う方の配列

    setParam(); // パラメータ数値計算

    for (uint16_t i = 0; i < BLOCK_SIZE; i++)
    {
      fxL[i] = param[GAIN] * xL[i]; // ゲイン調整

      xL[i] = bypass.process(xL[i], fxL[i], fxOn); // バイパス
    }
  }

};

#endif // FX_BOOST_HPP
