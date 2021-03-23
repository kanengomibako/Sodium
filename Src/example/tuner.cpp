#include "tuner.hpp"
#include "ssd1306.hpp"
#include "lib_filter.hpp"

/*
 * ギター チューナー(ベースでの動作未確認)
 * ブロックサイズ 16, 32, 64 サンプリング周波数 44.1kHz 48kHz を想定
 *
 * 下記ページのコードを改変して使用
 * https://www.cycfi.com/2018/03/fast-and-efficient-pitch-detection-bitstream-autocorrelation/
*/

// 各設定
const float muteOff = 0.0f; // 0:出力ミュート 1:ミュートしない
const float freqA = 440.0f; // 基準A音周波数 Hz
const float minFreq = 70.0f; // 最低周波数 ギター 6弦ドロップD音 73Hz
const float maxFreq = 400.0f; // 最高周波数 ギター 1弦5フレットA音 440Hz
const float tunerSamplingFreq = SAMPLING_FREQ; // チューナーでのサンプリング周波数
const uint16_t minPeriod = tunerSamplingFreq / maxFreq; // 最小周期サンプル区間
const uint16_t maxPeriod = tunerSamplingFreq / minFreq; // 最大周期サンプル区間
const uint16_t inDataSize = 64 * ((maxPeriod * 2) / 64) + 64; // 入力音配列 データ数 64の倍数にする
const uint16_t bitStreamSize = inDataSize / 32; // ビットストリーム配列 データ数
const float noiseThrethold = 0.02f; // ノイズ除去用閾値

// 各変数
float inData[2][inDataSize] = {};          // 入力音の配列 2つ準備し交互に利用
uint32_t bitStream[2][bitStreamSize] = {}; // ビットストリーム配列 2つ準備し交互に利用

uint32_t corrArray[inDataSize / 2]; // correlation(相間) 配列
uint32_t maxCorr = 0;               // correlation(相間) 最大値
uint32_t minCorr = UINT32_MAX;      // correlation(相間) 最小値

uint16_t estimatedIndex = minPeriod; // 推定周期 サンプル数
float estimatedFreq = 999.0f; // 推定周波数

// 周波数算出 --------------------------------------------------------
void estimateFreq(float inData[])
{
  /*
   * <実際より低い音として判定される問題を解決＞
   * 短い周期(高い音)の音が入力されると、周期が数倍の点にも谷が現れる
   * →例)周期4ms(250Hz)の音が12ms(83.3Hz)と判定
   * 実際より長く判定された周期を整数で割って整数倍したもの(1/4, 2/4, 3/4, 1/3, 2/3等)
   * それぞれについて相関値を調べ、整数倍全てに深い谷がある(allStrong)場合の除数を採用する
   */
  uint32_t subThreshold = 0.3f * (float)maxCorr; // 谷の強さ判定閾値
  uint16_t maxDiv = estimatedIndex / minPeriod;  // 除数最大値
  uint16_t estimatedDiv = 1;                     // 採用する除数
  for (int16_t div = maxDiv; div != 0; div--)
  {
    bool allStrong = true;

    for (uint16_t k = 1; k != div; k++)
    {
      uint16_t subPeriod = estimatedIndex * k / (float)div;
      if (corrArray[subPeriod] > subThreshold)
      {
        allStrong = false;
        break;
      }
    }

    if (allStrong)
    {
      estimatedDiv = div; // 除数決定、最終的な周波数計算時に使用
      break;
    }
  }

  // ゼロクロス（-→+） スタート点 検出
  uint16_t startIndex = 1;
  while (startIndex < inDataSize)
  {
    if (inData[startIndex - 1] <= 0.0f && inData[startIndex] > 0.0f) break;
    startIndex++;
  }
  float dy = inData[startIndex] - inData[startIndex - 1]; // 線形補間 y
  float dx1 = -inData[startIndex - 1] / dy;               // 線形補間 x1

  // ゼロクロス（-→+） 目的の点 検出
  uint16_t nextIndex = estimatedIndex - 1;
  while (nextIndex < inDataSize)
  {
    if (inData[nextIndex - 1] <= 0.0f && inData[nextIndex] > 0.0f) break;
    nextIndex++;
  }
  dy = inData[nextIndex] - inData[nextIndex - 1]; // 線形補間 y
  float dx2 = -inData[nextIndex - 1] / dy;        // 線形補間 x2

  float estimatedPeriod = (nextIndex - startIndex) + (dx2 - dx1); // 推定周期
  estimatedPeriod = estimatedPeriod / (float)estimatedDiv; // 予め計算した除数で割る

  // 推定周波数が3回連続で近い値となった時に周波数確定
  static float tmpFreq[3] = {}; // 推定周波数 一時保管用
  static uint8_t n = 0; // 上記tmpFreqの添字 0→1→2→0で循環させる
  if (estimatedPeriod > (float)minPeriod) tmpFreq[n] = tunerSamplingFreq / estimatedPeriod;

  if (tmpFreq[n] * 0.97f < tmpFreq[(n + 1) % 3] && tmpFreq[(n + 1) % 3] < 1.03f * tmpFreq[n] &&
      tmpFreq[n] * 0.97f < tmpFreq[(n + 2) % 3] && tmpFreq[(n + 2) % 3] < 1.03f * tmpFreq[n])
  {
    estimatedFreq = (tmpFreq[n] + tmpFreq[(n + 1) % 3] + tmpFreq[(n + 2) % 3]) / 3.0f;
  }
  n = (n + 1) % 3;
}

// 自己相関計算 ------------------------------------------------------------------
void bitstreamAutocorrelation(uint16_t startCnt, uint32_t bitData[])
{
  // pos：position ビットストリーム配列をズラした位置
  // pos → minPeriod ～ inDataSize/2 まで相間を計算するが、計算負荷軽減のため分割する
  for (uint16_t pos = startCnt; (pos >= minPeriod) && (pos < startCnt + BLOCK_SIZE / 2); pos++)
  {
    uint16_t midBitStreamSize = (bitStreamSize / 2) - 1; // ビットストリーム配列データ数の半分
    uint16_t index = startCnt / 32; // ビットストリーム配列の何番目の整数か
    uint16_t shift = startCnt % 32; // ビットストリーム配列内の整数 シフト数
    uint32_t corr = 0; // correlation(相間)

    if (shift == 0)
    {
      for (uint16_t i = 0; i < midBitStreamSize; i++)
      { // ^(XOR)、ビットが1になっている所を数え相間を算出
        corr += __builtin_popcount(bitData[i] ^ bitData[i + index]);
      }
    }
    else
    {
      for (uint16_t i = 0; i < midBitStreamSize; i++)
      {
        uint32_t tmp = bitData[i + index] >> shift;    // ビットストリーム配列をズラしたもの
        tmp |= bitData[i + 1 + index] << (32 - shift); // 右シフトしたものと左シフトしたものを合わせる
        corr += __builtin_popcount(bitData[i] ^ tmp);
      }
    }

    shift++;
    if (shift == 32) // shiftが32まで来たらindexを増やす
    {
      shift = 0;
      index++;
    }

    corrArray[pos] = corr; // correlation(相間) 配列
    maxCorr = max(maxCorr, corr); // 最大値を記録
    if (corr < minCorr)
    {
      minCorr = corr; // 最小値を記録
      estimatedIndex = pos; // 相間が最小となる時の位置→周期計算に利用
    }

  }

}

// 入力音配列をビットストリームへ変換 --------------------------------------------
void bitStreamSet(uint16_t cnt, float x, float inData[], uint32_t bitData[])
{
  uint32_t val = 0; // 入力音がゼロ以上か判定 0または1
  uint32_t mask = 0; // マスクビット

  // 入力音配列にデータを保存
  inData[cnt] = x;

  // 入力音配列をビットストリーム配列へ変換
  if (inData[cnt] < -noiseThrethold) val = 0;
  else if (inData[cnt] > 0.0f) val = 1;
  mask = 1 << (cnt % 32);
  bitData[cnt / 32] ^= (-val ^ bitData[cnt / 32]) & mask; // -val は 111...111 または 000...000（解説省略）
}

// 信号処理 ----------------------------------------------------------------------
void tunerProcess(float xL[], float xR[])
{
  float fxL[BLOCK_SIZE] = {}; // 入力フィルタ後の配列
  static lpf2nd lpf(maxFreq); // 入力2次LPF
  static uint16_t inDataCnt = 0; // 入力音配列の添字カウント
  static bool arraySel = false; // 2つの配列 入れ替え用

  // 配列準備と自己相間の計算が終了したデータを使い周波数を算出、変数初期化 //////////
  if (inDataCnt == 0)
  {
    estimateFreq(inData[arraySel]);

    maxCorr = 0;
    minCorr = UINT32_MAX;
    for (uint16_t i = 0; i < inDataSize / 2; i++) corrArray[i] = 0;
  }

  // 自己相関計算 ////////////////////////////////////////////////////////////////
  bitstreamAutocorrelation(inDataCnt / 2, bitStream[!arraySel]);

  // 入力音配列とビットストリーム配列を準備 //////////////////////////////////////
  for (uint16_t i = 0; i < BLOCK_SIZE; i++)
  {
    fxL[i] = lpf.process(xL[i]);

    bitStreamSet(inDataCnt, fxL[i], inData[arraySel], bitStream[arraySel]);
    inDataCnt++;
    if (inDataCnt == inDataSize) // 配列 準備完了
    {
      inDataCnt = 0;
      arraySel = !arraySel;
    }

    xL[i] = muteOff * xL[i]; // 出力ミュート
  }

}

// 画面表示 ----------------------------------------------------------------------
void tunerDisp()
{

  const float cent1 = powf(2.0f, 1.0f / 1200.0f); // 1セント(半音の100分の1)
  const float st = powf(2.0f , 1.0f / 12.0f);     // semitone(半音)

  // 周波数のズレ 5段階
  const float freqError[5] = {powf(cent1, 2), powf(cent1, 14), powf(cent1, 21), powf(cent1, 30), powf(cent1, 50)};

  // D～C#の基準周波数 13個目は該当なしの場合
  const float noteFreq[13] = {freqA/powf(st,43), freqA/powf(st,42), freqA/powf(st,41), freqA/powf(st,40),
      freqA/powf(st,39), freqA/powf(st,38), freqA/powf(st,37), freqA/powf(st,36), freqA/powf(st,35),
      freqA/powf(st,34), freqA/powf(st,33), freqA/powf(st,32), 9999};

  // 計算済周波数を一時保存(表示用計算の途中で値が変更されるのを防ぐ)
  float dispFreq = estimatedFreq; // 表示用周波数

  // 周波数表示
  if (estimatedFreq < 1000.0f && estimatedFreq > 10.0f)
  {
    string freqStr = std::to_string((uint16_t)(100.0f * estimatedFreq)); // 周波数文字列
    if (estimatedFreq < 100.0f) freqStr.insert(2, "."); // 小数点を挿入
    else freqStr.insert(3, ".");
    ssd1306_R_xyWriteStrWT(121, 0, freqStr + "Hz", Font_7x10);
  }


  // 検出周波数の音名、ズレを判別 //////////////////////////////////////////////

  uint8_t noteNum = 12; // 音名番号 0～11(D～C#) 該当なしの場合は12
  int8_t errorNum = 5; // ズレがどのくらいか -4～4 該当なしの場合は5
  const float pow2[4] = {1.0f, 2.0f, 4.0f, 8.0f}; // 2の0乗～3乗
  float freqOct = 1.0f; // オクターブ計算済の基準周波数

  // 音名を判定
  for (uint8_t octNum = 0; octNum <= 3; octNum++) // 3オクターブ上まで判定
  {
    for (uint8_t i = 0; i < 12; i++) // 12音のどれか判定
    {
      freqOct = noteFreq[i] * pow2[octNum];
      for (int8_t k = 0; k <= 4; k++) // 基準周波数からのズレを判定
      {
        if ((freqOct / freqError[k] < dispFreq) && (dispFreq < freqOct * freqError[k]))
        {
          errorNum = k; // ズレを記録
          if (dispFreq < freqOct)  errorNum = -k; // マイナス側にズレていた場合 負の値にする
          noteNum = i; // 判定された音名番号を記録
          break;
        }
      }
      if (errorNum != 5) break; // ズレ判定済の場合、ループを抜ける
    }
    if (noteNum != 12) break; // 音名判定済の場合、ループを抜ける
  }


  // 四角枠 描画 ///////////////////////////////////////////////////////////////

  uint8_t x = 58; // x座標
  uint8_t y = 12; // y座標

  // 中央の四角枠
  for (uint8_t i = 0; i < 12; i++)
  {
    ssd1306_DrawPixel(x+i, y, White);
    ssd1306_DrawPixel(x+i, y+26, White);
  }
  for (uint8_t j = 0; j < 27; j++)
  {
    ssd1306_DrawPixel(x, y+j, White);
    ssd1306_DrawPixel(x+11, y+j, White);
  }

  // 左側の四角枠×4 右側の四角枠×4
  for (uint8_t m = 2; m < 76; m += 73)
  {
    for (uint8_t k = 0; k < 4; k++)
    {
      x = m + k * 14;
      y = 16;
      for (uint8_t i = 0; i < 9; i++)
      {
        ssd1306_DrawPixel(x+i, y, White);
        ssd1306_DrawPixel(x+i, y+18, White);
      }
      for (uint8_t j = 0; j < 19; j++)
      {
        ssd1306_DrawPixel(x, y+j, White);
        ssd1306_DrawPixel(x+8, y+j, White);
      }
    }
  }


  // 四角形・三角形・音名 描画 /////////////////////////////////////////////////////////

  // 中央の四角形描画
  if (errorNum == 0)
  {
    x = 58;
    y = 12;
    for (uint8_t i = 0; i < 12; i++)
    {
      for (uint8_t j = 0; j < 27; j++) ssd1306_DrawPixel(x+i, y+j, White);
    }
  }

  // 左側の四角形・三角形描画
  if (-5 < errorNum && errorNum <= 0)
  {
    x = 15 + (3 + errorNum) * 14;
    y = 16;
    for (uint8_t i = 0; i < 9; i++)
    {
      if (errorNum != 0) for (uint8_t j = 0; j < 19; j++) ssd1306_DrawPixel(x+i, y+j, White);
    }

    for (uint8_t i = 0; i < 9; i++)
    {
      for (int8_t j = i; j < 17 - i; j++) ssd1306_DrawPixel(32+i, 43+j, White);
    }
  }

  // 右側の四角形・三角形描画
  if (0 <= errorNum && errorNum < 5)
  {
    x = 62 + errorNum * 14;
    y = 16;
    for (uint8_t i = 0; i < 9; i++)
    {
      if (errorNum != 0) for (uint8_t j = 0; j < 19; j++) ssd1306_DrawPixel(x+i, y+j, White);
    }

    for (uint8_t i = 0; i < 9; i++)
    {
      for (int8_t j = i; j < 17 - i; j++) ssd1306_DrawPixel(95-i, 43+j, White);
    }
  }

  // 音名を描画
  const string note[13] = {"D", "D", "E", "F", "F", "G", "G", "A", "A", "B", "C", "C", ""};
  ssd1306_xyWriteStrWT(55, 40, note[noteNum], Font_16x26);

  // シャープを描画
  if (noteNum == 1 || noteNum == 4 || noteNum == 6 || noteNum == 8 || noteNum == 11)
  {
    ssd1306_xyWriteStrWT(72, 40, "#", Font_11x18);
  }

  ssd1306_xyWriteStrWT(0, 0, "TUNER", Font_7x10); // 左上の表示
  HAL_Delay(100);
}

