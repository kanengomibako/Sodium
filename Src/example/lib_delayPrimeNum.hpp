#ifndef LIB_DELAY_PRIMENUM_HPP
#define LIB_DELAY_PRIMENUM_HPP

#include "common.h"
#include "table_primeNum.h"

/* 素数サンプル数ディレイバッファ 255msまで float仕様----------------------------------------*/
class delayBufPrimeNum
{
private:
  float *delayArray;
  uint16_t wpos = 0; // write position 書き込み位置
  uint32_t maxDelaySample = 1;

public:
  delayBufPrimeNum()
  {
    set(1);
  }

  ~delayBufPrimeNum()
  {
    erase();
  }

  void set(uint8_t maxDelayTime) // 最大ディレイ時間 ms
  {
    maxDelaySample = primeNum[maxDelayTime]; // 最大サンプル数
    delayArray = new float[maxDelaySample]; // バッファ配列メモリ確保
    for (uint16_t i = 0; i < maxDelaySample; i++) delayArray[i] = 0.0f; // 配列0埋め
  }

  void erase()
  {
    delete[] delayArray; // バッファ配列メモリ削除
  }

  void write(float x)
  {
    delayArray[wpos] = x; // バッファ配列へfloatで書き込み
    wpos++; // 書込位置を1つ進める
    if (wpos == maxDelaySample) wpos = 0; // 最大サンプル数までで0に戻す
  }

  float read(uint8_t delayTime)
  {
    uint16_t interval = primeNum[delayTime]; // 書込位置と読出位置の間隔
    if (interval > maxDelaySample) interval = maxDelaySample;
    uint16_t rpos;
    if (wpos >= interval) rpos = wpos - interval;  // 読出位置を取得
    else rpos = wpos - interval + maxDelaySample;
    return delayArray[rpos]; // バッファ配列から読み出し
  }

  float readFixed() // 最大ディレイ時間で読み出し
  {
    return delayArray[wpos];
  }

};

#endif // LIB_DELAY_PRIMENUM_HPP
