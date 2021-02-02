#include "main.h"
#include "user_main.h"
#include "ssd1306.h"
#include "fonts.h"
#include <math.h> // powf関数用

extern I2C_HandleTypeDef hi2c1;
extern I2S_HandleTypeDef hi2s2;
extern I2S_HandleTypeDef hi2s3;

int32_t RxBuffer[BLOCK_SIZE*4] = {}; // 音声信号受信バッファ
int32_t TxBuffer[BLOCK_SIZE*4] = {}; // 音声信号送信バッファ

uint8_t sw[10] = {}; // スイッチオン・オフ状態 0→左上 1→左下 2→右上 3→右下 4→フットスイッチ 5～9不使用
uint8_t callbackCount = 0; // I2Sの割り込みごとにカウントアップ

uint16_t shortPushCount = 10;  // スイッチ短押し カウント数
uint16_t longPushCount = 300;  // スイッチ長押し カウント数

uint8_t paramHigh = 5;  // 高域調整0 ～ 10 LPFカットオフ周波数 500 ～  5k Hz
uint8_t paramLow = 5;   // 低域調整0 ～ 10 HPFカットオフ周波数  1k ～ 100 Hz
uint8_t paramLevel = 5; // 音量調整0 ～ 10 (-10 ～ +10 dB)

uint8_t cursorPosition = 0; // パラメータカーソル位置 0 ～ 2
uint8_t statusNum = 7; // ステータス表示用番号

void mainInit() // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<最初に1回のみ行う処理
{
  // スイッチ短押し、長押しのカウント数を計算
  float interruptInterval = 1000.0f / SAMPLING_FREQ * (float)BLOCK_SIZE; // I2Sの割り込み間隔 ミリ秒
  shortPushCount = 1.0f + (float)SHORT_PUSH_MSEC / (5.0f * interruptInterval); // 1つのスイッチは5回に1回の読取のため5をかけている
  longPushCount = 1.0f + (float)LONG_PUSH_MSEC / (5.0f * interruptInterval);

  // ディスプレイ点灯確認
  ssd1306_Init(&hi2c1);
  HAL_Delay(500);
  ssd1306_Fill(White);
  ssd1306_UpdateScreen(&hi2c1);

  // I2SのDMA開始
  HAL_I2S_Transmit_DMA(&hi2s2,(uint16_t*)TxBuffer,BLOCK_SIZE*4);
  HAL_I2S_Receive_DMA(&hi2s3,(uint16_t*)RxBuffer,BLOCK_SIZE*4);

  // オーディオコーデック リセット
  HAL_GPIO_WritePin(CODEC_RST_GPIO_Port, CODEC_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(CODEC_RST_GPIO_Port, CODEC_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(100);

  // I2Sのフレームエラー発生の場合、リセットを繰り返す
  while(__HAL_I2S_GET_FLAG(&hi2s2, I2S_FLAG_FRE) || __HAL_I2S_GET_FLAG(&hi2s3, I2S_FLAG_FRE))
  {
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("ERROR",Font_11x18,Black);
    ssd1306_UpdateScreen(&hi2c1);
    HAL_GPIO_WritePin(CODEC_RST_GPIO_Port, CODEC_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(CODEC_RST_GPIO_Port, CODEC_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
  }

  // LED点灯確認
  HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
  HAL_Delay(700);
  HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
  HAL_Delay(700);
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET);
  HAL_Delay(700);
  HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);

}

void mainLoop() // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<メインループ
{
  char *str[11] = {" 0", " 1", " 2", " 3", " 4", " 5", " 6", " 7", " 8", " 9", "10"}; // 数値描画用文字列
  char *status[8] = {"  SW0 LONG PUSH   ",
                     "  SW1 LONG PUSH   ",
                     "  SW2 LONG PUSH   ",
                     "  SW3 LONG PUSH   ",
                     "  SW4 LONG PUSH   ",
                     "123456789012345678",
                     "123456789012345678",
                     PEDAL_NAME          }; // ステータス表示描画用文字列

  // 一旦画面表示を全て消す
  ssd1306_Fill(Black);

  // パラメータ名称表示
  ssd1306_SetCursor(16, 11);
  ssd1306_WriteString("HIGH", Font_11x18, White);
  ssd1306_SetCursor(16, 28);
  ssd1306_WriteString("LOW", Font_11x18, White);
  ssd1306_SetCursor(16, 45);
  ssd1306_WriteString("LEVEL", Font_11x18, White);

  // パラメータ数値表示
  ssd1306_SetCursor(82, 11);
  ssd1306_WriteString(str[paramHigh], Font_11x18, White); // HIGH
  ssd1306_SetCursor(82, 28);
  ssd1306_WriteString(str[paramLow], Font_11x18, White); // LOW
  ssd1306_SetCursor(82, 45);
  ssd1306_WriteString(str[paramLevel], Font_11x18, White); // LEVEL

  // カーソル表示
  ssd1306_SetCursor(0, cursorPosition * 18 + 9);
  ssd1306_WriteString(">", Font_11x18, White);

  // ステータス表示
  ssd1306_SetCursor(0, 0);
  ssd1306_WriteString(status[statusNum], Font_7x10, White);

  // 画面更新
  ssd1306_UpdateScreen(&hi2c1);

  // ステータス表示一時変更
  if (statusNum != 7)
  {
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(status[statusNum], Font_7x10, White);
    ssd1306_UpdateScreen(&hi2c1);
    HAL_Delay(1000); // 表示を変更し一定時間経過後、ペダル名表示に戻す
    statusNum = 7;
  }

}

void swProcess(uint8_t num) // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<スイッチ処理
{
  static uint16_t swcount[10] = {}; // スイッチが押されている間カウントアップ

  switch(num)
  {
    case 0: // 左上スイッチ ----------------------------
      if (!HAL_GPIO_ReadPin(SW0_UPPER_L_GPIO_Port, SW0_UPPER_L_Pin))
      {
        swcount[num]++;
        if (swcount[num] == longPushCount) // 長押し
        {
          statusNum = 0;
        }
      }
      else
      {
        if (swcount[num] >= shortPushCount && swcount[num] < longPushCount) // 短押し 離した時の処理
        {
          if (cursorPosition <= 0) cursorPosition = 2; // カーソル位置変更
          else cursorPosition--;
        }
        swcount[num] = 0;
      }
      break;
    case 1: // 左下スイッチ ----------------------------
      if (!HAL_GPIO_ReadPin(SW1_LOWER_L_GPIO_Port, SW1_LOWER_L_Pin))
      {
        swcount[num]++;
        if (swcount[num] == longPushCount) // 長押し
        {
          statusNum = 1;
        }
      }
      else
      {
        if (swcount[num] >= shortPushCount && swcount[num] < longPushCount) // 短押し 離した時の処理
        {
          if (cursorPosition >= 2) cursorPosition = 0; // カーソル位置変更
          else cursorPosition++;
        }
        swcount[num] = 0;
      }
      break;
    case 2: // 右上スイッチ ----------------------------
      if (!HAL_GPIO_ReadPin(SW2_UPPER_R_GPIO_Port, SW2_UPPER_R_Pin))
      {
        swcount[num]++;
        if (swcount[num] == longPushCount) // 長押し
        {
          statusNum = 2;
        }
      }
      else
      {
        if (swcount[num] >= shortPushCount && swcount[num] < longPushCount) // 短押し 離した時の処理
        {
          switch (cursorPosition) // パラメータ数値変更
          {
            case 0:
              if (paramHigh < 10) paramHigh++;
              break;
            case 1:
              if (paramLow < 10) paramLow++;
              break;
            case 2:
              if (paramLevel < 10) paramLevel++;
              break;
            default:
              break;
          }
        }
        swcount[num] = 0;
      }
      break;
    case 3: // 右下スイッチ ----------------------------
      if (!HAL_GPIO_ReadPin(SW3_LOWER_R_GPIO_Port, SW3_LOWER_R_Pin))
      {
        swcount[num]++;
        if (swcount[num] == longPushCount) // 長押し
        {
          statusNum = 3;
        }
      }
      else
      {
        if (swcount[num] >= shortPushCount && swcount[num] < longPushCount) // 短押し 離した時の処理
        {
          switch (cursorPosition) // パラメータ数値変更
          {
            case 0:
              if (paramHigh > 0) paramHigh--;
              break;
            case 1:
              if (paramLow > 0) paramLow--;
              break;
            case 2:
              if (paramLevel > 0) paramLevel--;
              break;
            default:
              break;
          }
        }
        swcount[num] = 0;
      }
      break;
    case 4: // フットスイッチ ----------------------------
      if (!HAL_GPIO_ReadPin(SW4_FOOT_GPIO_Port, SW4_FOOT_Pin))
      {
        swcount[num]++;
        if (swcount[num] == longPushCount) // 長押し
        {
          statusNum = 4;
        }
      }
      else
      {
        if (swcount[num] >= shortPushCount && swcount[num] < longPushCount) // 短押し 離した時の処理
        {
          if (sw[num])
          {
            sw[num] = 0;
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
          }
          else
          {
            sw[num] = 1;
            HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET);
          }
        }
        swcount[num] = 0;
      }
      break;
    default:
      break;
  }
}

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<DMA用に上位16ビットと下位16ビットを入れ替える
// 負の値の場合に備えて右シフトの場合0埋めする
int32_t swap16(int32_t x)
{
  return (0x0000FFFF & x >> 16) | x << 16;
}

void mainProcess(uint16_t start_sample)
{
  float lpfFreq = 500.0f * powf(1.259f, (float)paramHigh); // ローパスフィルタ周波数計算
  float hpfFreq = 1000.0f * powf(0.794f, (float)paramLow); // ハイパスフィルタ周波数計算
  float gain = powf(10.0f, (2.0f * (float)paramLevel - 10.0f) / 20.0f); // 音量調整 dB計算

  float xL[BLOCK_SIZE] = {}; // Lch float計算用データ

  for (uint16_t i = 0; i < BLOCK_SIZE; i++)
  {
     uint16_t m = (start_sample + i) * 2;  // Lchデータ配列の偶数添字計算（Rch不使用）

    // 受信データを計算用データ配列へ 値を-1～+1(float)へ変更
     xL[i] = (float)swap16(RxBuffer[m]) / 2147483648.0f;

    // エフェクト処理
    if (sw[4])
    {
      xL[i] = lpf(xL[i], lpfFreq);  // ローパスフィルタ
      xL[i] = hpf(xL[i], hpfFreq); // ハイパスフィルタ
      xL[i] = xL[i] * gain; // 音量調整
    }

    // オーバーフロー防止
    if (xL[i] < -1.0f) xL[i] = -1.0f;
    if (xL[i] > 0.99f) xL[i] = 0.99f;

    // 計算済データを送信バッファへ 値を32ビット整数へ戻す
    TxBuffer[m] = swap16((int32_t)(2147483648.0f * xL[i]));
  }
}

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<I2Sの受信バッファに半分データがたまったときの割り込み
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  mainProcess(0); // 0 ～ 15 を処理(0 ～ BLOCK_SIZE-1)
  swProcess(callbackCount % 5); // 割り込みごとにスイッチ処理するが、スイッチ1つずつを順番に行う
  callbackCount++;
}

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<I2Sの受信バッファに全データがたまったときの割り込み
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  mainProcess(BLOCK_SIZE); // 16 ～ 31 を処理(BLOCK_SIZE ～ BLOCK_SIZE*2-1)
  swProcess(callbackCount % 5); // 割り込みごとにスイッチ処理するが、スイッチ1つずつを順番に行う
  callbackCount++;
}

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<テストプログラム用LPF、HPF
// 1次LPF、HPF用の係数計算、50～10kHzでの近似曲線
float lpfCoef(float fc)
{
  float w = 2.0f * PI * fc / SAMPLING_FREQ;
  return 0.99983075f - 0.99678388f * w + 0.49528899f *w*w - 0.10168296f *w*w*w;
}

float lpf(float x,float fc)
{
  static float y1 = 0.0f;
  float a1 = lpfCoef(fc);
  float b0 = 1.0f - a1;
  float y = b0 * x + a1 * y1;
  y1 = y;
  return y;
}

float hpf(float x,float fc)
{
  static float x1 = 0.0f;
  static float y1 = 0.0f;
  float a1 = lpfCoef(fc);
  float b0 = 0.5f * (1.0f + a1);
  float y = b0 * x - b0 * x1 + a1 * y1;
  x1 = x;
  y1 = y;
  return y;
}
