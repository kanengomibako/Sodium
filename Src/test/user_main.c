#include "main.h"
#include "user_main.h"
#include "ssd1306.h"
#include "fonts.h"
#include <math.h> // powf関数用

extern I2C_HandleTypeDef hi2c1;
extern I2S_HandleTypeDef hi2s2;
extern I2S_HandleTypeDef hi2s3;

int32_t RX_BUFFER[BLOCK_SIZE*2] = {}; // 音声信号受信バッファ
int32_t TX_BUFFER[BLOCK_SIZE*2] = {}; // 音声信号送信バッファ

uint8_t sw[10] = {}; // スイッチオン・オフ状態 0→左上　1→左下 2→右上　3→右下 4→フットスイッチ 5～9不使用
uint8_t callback_count = 0; // I2Sの割り込みごとにカウントアップ

uint16_t short_push_count = 10;  // スイッチ短押し カウント数
uint16_t long_push_count = 300;  // スイッチ長押し カウント数

uint8_t param_high = 5;  // 高域調整0 ～ 10 LPFカットオフ周波数 500 ～  5k Hz
uint8_t param_low = 5;   // 低域調整0 ～ 10 HPFカットオフ周波数  1k ～ 100 Hz
uint8_t param_level = 5; // 音量調整0 ～ 10 (-10 ～ +10 dB)

uint8_t cursor_position = 0; // パラメータカーソル位置 0 ～ 2
uint8_t status_num = 7; // ステータス表示用番号

void mainInit() // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<最初に1回のみ行う処理
{
  // スイッチ短押し、長押しのカウント数を計算
  float interrupt_interval = 1000.0f / SAMPLING_FREQ * (float)BLOCK_SIZE / 2.0f; // I2Sの割り込み間隔 ミリ秒
  short_push_count = 1.0f + (float)SHORT_PUSH_MSEC / (5.0f * interrupt_interval); // 1つのスイッチは5回に1回の読取のため5をかけている
  long_push_count = 1.0f + (float)LONG_PUSH_MSEC / (5.0f * interrupt_interval);

  // I2SのDMA開始
  HAL_I2S_Transmit_DMA(&hi2s2,(uint16_t*)TX_BUFFER,BLOCK_SIZE*2);
  HAL_I2S_Receive_DMA(&hi2s3,(uint16_t*)RX_BUFFER,BLOCK_SIZE*2);

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
    HAL_GPIO_WritePin(CODEC_RST_GPIO_Port, CODEC_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(CODEC_RST_GPIO_Port, CODEC_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
  }

  // ディスプレイ点灯確認
  ssd1306_Init(&hi2c1);
  HAL_Delay(500);
  ssd1306_Fill(White);
  ssd1306_UpdateScreen(&hi2c1);

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

  // パラメータ画面
  ssd1306_Fill(Black);
  ssd1306_SetCursor(16, 11);
  ssd1306_WriteString("HIGH", Font_11x18, White);
  ssd1306_SetCursor(16, 28);
  ssd1306_WriteString("LOW", Font_11x18, White);
  ssd1306_SetCursor(16, 45);
  ssd1306_WriteString("LEVEL", Font_11x18, White);
  ssd1306_UpdateScreen(&hi2c1);

}

void mainLoop() // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<メインループ
{
  // 前回の各変数の状態を保存しておき、値が変化したときのみ処理を行う
  static uint16_t last_param_high = 999;
  static uint16_t last_param_low = 999;
  static uint16_t last_param_level = 999;
  static uint8_t last_cursor_position = 99;
  static uint8_t last_status_num = 99;
  char *buf[11] = {" 0", " 1", " 2", " 3", " 4", " 5", " 6", " 7", " 8", " 9", "10"}; // 数値描画用文字列
  char *status[8] = {"  SW0 LONG PUSH   ",
                     "  SW1 LONG PUSH   ",
                     "  SW2 LONG PUSH   ",
                     "  SW3 LONG PUSH   ",
                     "  SW4 LONG PUSH   ",
                     "123456789012345678",
                     "123456789012345678",
                     PEDAL_NAME          }; // ステータス表示描画用文字列

  // パラメータ数値表示変更
  if (last_param_high != param_high) // HIGH
  {
    ssd1306_SetCursor(82, 11);
    ssd1306_WriteString(buf[param_high], Font_11x18, White);
    last_param_high = param_high;
  }
  if (last_param_low != param_low) // LOW
  {
    ssd1306_SetCursor(82, 28);
    ssd1306_WriteString(buf[param_low], Font_11x18, White);
    last_param_low = param_low;
  }
  if (last_param_level != param_level) // LEVEL
  {
    ssd1306_SetCursor(82, 45);
    ssd1306_WriteString(buf[param_level], Font_11x18, White);
    last_param_level = param_level;
  }

  // カーソル表示変更
  if (last_cursor_position != cursor_position)
  {
    ssd1306_SetCursor(0, last_cursor_position * 18 + 9);
    ssd1306_WriteString(" ", Font_11x18, White);
    ssd1306_SetCursor(0, cursor_position * 18 + 9);
    ssd1306_WriteString(">", Font_11x18, White);
    last_cursor_position = cursor_position;
  }

  // ステータス表示変更
  if (last_status_num != status_num)
  {
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(status[status_num], Font_7x10, White);
    ssd1306_UpdateScreen(&hi2c1);
    HAL_Delay(1000); // 1000ms 表示を変更した後、ペダル名表示に戻す
    status_num = 7;
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(status[status_num], Font_7x10, White);
    ssd1306_UpdateScreen(&hi2c1);
    last_status_num = status_num;
  }

  ssd1306_UpdateScreen(&hi2c1);
  HAL_Delay(10);

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
        if (swcount[num] > long_push_count) // 長押し
        {
          status_num = 0;
        }
      }
      else
      {
        if (swcount[num] >= short_push_count && swcount[num] < long_push_count) // 短押し 離した時の処理
        {
          if (cursor_position <= 0) cursor_position = 2; // カーソル位置変更
          else cursor_position--;
        }
        swcount[num] = 0;
      }
      break;
    case 1: // 左下スイッチ ----------------------------
      if (!HAL_GPIO_ReadPin(SW1_LOWER_L_GPIO_Port, SW1_LOWER_L_Pin))
      {
        swcount[num]++;
        if (swcount[num] > long_push_count) // 長押し
        {
          status_num = 1;
        }
      }
      else
      {
        if (swcount[num] >= short_push_count && swcount[num] < long_push_count) // 短押し 離した時の処理
        {
          if (cursor_position >= 2) cursor_position = 0; // カーソル位置変更
          else cursor_position++;
        }
        swcount[num] = 0;
      }
      break;
    case 2: // 右上スイッチ ----------------------------
      if (!HAL_GPIO_ReadPin(SW2_UPPER_R_GPIO_Port, SW2_UPPER_R_Pin))
      {
        swcount[num]++;
        if (swcount[num] > long_push_count) // 長押し
        {
          status_num = 2;
        }
      }
      else
      {
        if (swcount[num] >= short_push_count && swcount[num] < long_push_count) // 短押し 離した時の処理
        {
          switch (cursor_position) // パラメータ数値変更
          {
            case 0:
              if (param_high < 10) param_high++;
              break;
            case 1:
              if (param_low < 10) param_low++;
              break;
            case 2:
              if (param_level < 10) param_level++;
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
        if (swcount[num] > long_push_count) // 長押し
        {
          status_num = 3;
        }
      }
      else
      {
        if (swcount[num] >= short_push_count && swcount[num] < long_push_count) // 短押し 離した時の処理
        {
          switch (cursor_position) // パラメータ数値変更
          {
            case 0:
              if (param_high > 0) param_high--;
              break;
            case 1:
              if (param_low > 0) param_low--;
              break;
            case 2:
              if (param_level > 0) param_level--;
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
        if (swcount[num] > long_push_count) // 長押し
        {
          status_num = 4;
        }
      }
      else
      {
        if (swcount[num] >= short_push_count && swcount[num] < long_push_count) // 短押し 離した時の処理
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
  float lpf_freq = 500.0f * powf(1.259f, (float)param_high); // ローパスフィルタ周波数計算
  float hpf_freq = 1000.0f * powf(0.794f, (float)param_low); // ハイパスフィルタ周波数計算
  float gain = powf(10.0f, (2.0f * (float)param_level - 10.0f) / 20.0f); // 音量調整 dB計算

  float xL[BLOCK_SIZE] = {}; // Lch float計算用データ

  for (uint16_t i = start_sample; i < start_sample + BLOCK_SIZE/2; i++)
  {
    // 受信データを計算用データ配列へ 値を-1～+1(float)へ変更
    // Lchデータは配列の偶数添字 Rch不使用
    xL[i] = (float)swap16(RX_BUFFER[i*2]) / 2147483648.0f;

    // エフェクト処理
    if (sw[4])
    {
      xL[i] = lpf(xL[i], lpf_freq);  // ローパスフィルタ
      xL[i] = hpf(xL[i], hpf_freq); // ハイパスフィルタ
      xL[i] = xL[i] * gain; // 音量調整
    }

    // オーバーフロー防止
    if (xL[i] < -1.0f) xL[i] = -1.0f;
    if (xL[i] > 0.99f) xL[i] = 0.99f;

    // 計算済データを送信バッファへ 値を32ビット整数へ戻す
    TX_BUFFER[i*2] = swap16((int32_t)(2147483648.0f * xL[i]));
  }
}

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<I2Sの受信バッファに半分データがたまったときの割り込み
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  mainProcess(0); // 0 ～ 15 を処理(0 ～ BLOCK_SIZE/2-1)
  swProcess(callback_count % 5); // 割り込みごとにスイッチ処理するが、スイッチ1つずつを順番に行う
  callback_count++;
}

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<I2Sの受信バッファに全データがたまったときの割り込み
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  mainProcess(BLOCK_SIZE/2); // 16 ～ 31 を処理(BLOCK_SIZE/2 ～ BLOCK_SIZE-1)
  swProcess(callback_count % 5); // 割り込みごとにスイッチ処理するが、スイッチ1つずつを順番に行う
  callback_count++;
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
