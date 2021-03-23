#include "common.h"
#include "user_main.h"
#include "main.h"
#include "ssd1306.hpp"
#include "fx.hpp"

#if TUNER_ENABLED
#include "tuner.hpp"
#endif

extern I2C_HandleTypeDef hi2c1;
extern I2S_HandleTypeDef hi2s2;
extern I2S_HandleTypeDef hi2s3;

int32_t RxBuffer[BLOCK_SIZE*4] = {}; // 音声信号受信バッファ配列 Lch前半 Lch後半 Rch前半 Rch後半
int32_t TxBuffer[BLOCK_SIZE*4] = {}; // 音声信号送信バッファ配列

bool fxOn = false; // エフェクトオン・オフ状態
uint32_t callbackCount = 0; // I2Sの割り込みごとにカウントアップ タイマとして利用

uint32_t cpuUsageCycleMax[MAX_FX_NUM] = {}; // CPU使用サイクル数 各エフェクトごとに最大値を記録
const float i2sInterruptInterval = (float)BLOCK_SIZE / SAMPLING_FREQ; // I2Sの割り込み間隔時間

// スイッチ短押し、スイッチ長押し、ステータス情報表示時間のカウント数
const uint32_t shortPushCount = 1 + SHORT_PUSH_MSEC / (4 * 1000 * i2sInterruptInterval); // 1つのスイッチは4回に1回の読取のため4をかける
const uint32_t longPushCount = 1 + LONG_PUSH_MSEC / (4 * 1000 * i2sInterruptInterval);
const uint32_t statusDispCount = 1 + STATUS_DISP_MSEC / (1000 * i2sInterruptInterval);

int16_t fxParam[20] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}; // 現在のエフェクトパラメータ
string fxParamStr[20] = {};  // 現在のエフェクトパラメータ数値 文字列

int16_t fxParamMax[20] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}; // エフェクトパラメータ最大値
int16_t fxParamMin[20] = {}; // エフェクトパラメータ最小値

string fxParamName[20] = {}; // エフェクトパラメータ名 LEVEL, GAIN等

uint8_t fxParamIndex = 0; // エフェクトパラメータ 現在何番目か ※0から始まる
uint8_t fxParamIndexMax = 0; // エフェクトパラメータ数 最大値-1

uint8_t fxNum = 0; // 現在のエフェクト番号
int8_t fxChangeFlag = 0; // エフェクト種類変更フラグ 次エフェクトへ: 1 前エフェクトへ: -1

int16_t fxAllData[MAX_FX_NUM][20] = {}; // 全てのエフェクトパラメータデータ配列

const bool fxEnabled[MAX_FX_NUM] = FX_ENABLE_SETTING; // エフェクト有効・無効リスト common.hで設定

uint8_t cursorPosition = 0; // パラメータ選択カーソル位置 0 ～ 5
string statusStr = PEDAL_NAME; // ステータス表示文字列

enum modeName {NORMAL, TAP, TUNER};
uint8_t mode = NORMAL; // 動作モード 0:通常 1:タップテンポ 2:チューナー

float tapTime = 0.0f; // タップテンポ入力時間 ms

const uint8_t fxNameXY[2] = {4,0}; // エフェクト名 ステータス 表示位置
const uint8_t fxPageXY[2] = {88,0}; // エフェクトパラメータ ページ 表示位置
const uint8_t percentXY[2] = {107,0}; // 処理時間% 表示位置
const uint8_t cursorPositionXY[6][2] = {{0,11},{0,29},{0,47},{65,11},{65,29},{65,47}}; // カーソル位置
const uint8_t fxParamNameXY[6][2] = {{0,17},{0,35},{0,53},{65,17},{65,35},{65,53}}; // エフェクトパラメータ名表示位置
const uint8_t fxParamStrXY[6][2]  = {{52,11},{52,29},{52,47},{117,11},{117,29},{117,47}}; // エフェクトパラメータ数値表示 右端の文字位置

void mainInit() // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<最初に1回のみ行う処理
{
  // Denormalized numbers（非正規化数）を0として扱うためFPSCRレジスタ変更
  asm("VMRS r0, FPSCR");
  asm("ORR r0, r0, #(1 << 24)");
  asm("VMSR FPSCR, r0");

  // 処理時間計測用設定
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  // ディスプレイ初期化
  ssd1306_Init(&hi2c1);

#if 0
  // ディスプレイ点灯確認
  ssd1306_Fill(White);
  ssd1306_SetCursor(3, 22);
  ssd1306_WriteString(PEDAL_NAME, Font_11x18, Black);
  ssd1306_UpdateScreen(&hi2c1);

  // LED点灯確認
  HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
  HAL_Delay(300);
  HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
  HAL_Delay(300);
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET);
  HAL_Delay(300);
  HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
#endif

  // I2SのDMA開始
  HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t*)TxBuffer, BLOCK_SIZE*4);
  HAL_I2S_Receive_DMA(&hi2s3, (uint16_t*)RxBuffer, BLOCK_SIZE*4);

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

  // 起動時フットスイッチ、左上スイッチ、右下スイッチを押していた場合、データ全消去
  if (!HAL_GPIO_ReadPin(SW0_UPPER_L_GPIO_Port, SW0_UPPER_L_Pin) &&
      !HAL_GPIO_ReadPin(SW3_LOWER_R_GPIO_Port, SW3_LOWER_R_Pin) &&
      !HAL_GPIO_ReadPin(SW4_FOOT_GPIO_Port, SW4_FOOT_Pin))
  {
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("ERASE ALL DATA", Font_7x10, Black);
    ssd1306_UpdateScreen(&hi2c1);
    eraseData();
    HAL_Delay(1000);
  }

  // 保存済パラメータ読込
  loadData();

  // 初期エフェクト読込
  for (int i = 0; i < MAX_FX_NUM; i++)
  {
    if (fxEnabled[fxNum]) break; // エフェクト有効の時は処理終了、無効の時は次のエフェクトへ
    fxNum = (fxNum + 1) % MAX_FX_NUM; // 最大値→最小値で循環 全エフェクト無効なら最初のfxNumに戻る
  }
  fxInit();

}

void mainLoop() // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<メインループ
{

  ssd1306_Fill(Black); // 一旦画面表示を全て消す

  if (mode == NORMAL) // 通常モード *****************************
  {
    uint8_t fxPage = fxParamIndex / 6; // エフェクトパラメータページ番号

    // ステータス表示------------------------------
    if (callbackCount > statusDispCount) // ステータス表示が変わり一定時間経過後、デフォルト表示に戻す
    {
      statusStr = fxNameList[fxNum]; // エフェクト名表示
    }
    ssd1306_xyWriteStrWT(fxNameXY[0], fxNameXY[1], statusStr, Font_7x10);

    // エフェクトパラメータ名称表示------------------------------
    for (int i = 0; i < 6; i++)
    {
      ssd1306_xyWriteStrWT(fxParamNameXY[i][0], fxParamNameXY[i][1], fxParamName[i+3*fxPage], Font_7x10);
    }

    // エフェクトパラメータ数値表示------------------------------
    for (int i = 0; i < 6; i++)
    {
      fxSetParamStr(i+6*fxPage); // パラメータ数値を文字列に変換
      ssd1306_R_xyWriteStrWT(fxParamStrXY[i][0], fxParamStrXY[i][1], fxParamStr[i+3*fxPage], Font_11x18);
    }

    // エフェクトパラメータページ番号表示------------------------------
    string fxPageStr = "P" + std::to_string(fxPage + 1);
    ssd1306_xyWriteStrWT(fxPageXY[0], fxPageXY[1], fxPageStr, Font_7x10);

    // カーソル表示(選択したパラメータの白黒反転) ------------------------------
    for (int i = 0; i < 62; i++)
    {
      for (int j = 0; j < 16; j++)
      {
        ssd1306_InvertPixel(cursorPositionXY[cursorPosition][0]+i, cursorPositionXY[cursorPosition][1]+j);
      }
    }

    // CPU使用率表示------------------------------
    uint8_t cpuUsagePercent = 100.0f * cpuUsageCycleMax[fxNum] / SystemCoreClock / i2sInterruptInterval;
    string percentStr = std::to_string(cpuUsagePercent);
    if (cpuUsagePercent < 10) percentStr = " " + percentStr + "%"; // 右揃えにする
    else percentStr = percentStr + "%";
    ssd1306_xyWriteStrWT(percentXY[0], percentXY[1], percentStr, Font_7x10);

  }

  if (mode == TAP) // タップテンポモード *****************************
  {
    ssd1306_xyWriteStrWT(0, 0, "TAP TEMPO", Font_7x10);
    string tmpStr = std::to_string((uint16_t)tapTime);
    ssd1306_R_xyWriteStrWT(114, 0, tmpStr + " ms", Font_7x10); // タップ間隔時間を表示
    if (tapTime > 60.0f) tmpStr = std::to_string((uint16_t)(60000.0f / tapTime)); // bpmを計算
    ssd1306_R_xyWriteStrWT(103, 20, tmpStr + " bpm", Font_16x26); // bpm表示

    uint16_t blinkCount = 1 + tapTime / (1000 * i2sInterruptInterval); // 点滅用カウント数
    if (callbackCount % blinkCount < 60 / (1000 * i2sInterruptInterval)) // 60msバーを表示、点滅
    {
      for (int i = 0; i < 112; i++)
      {
        ssd1306_DrawPixel(8 + i, 47, White);
        ssd1306_DrawPixel(8 + i, 48, White);
      }
    }
  }

  if (mode == TUNER) // チューナーモード *****************************
  {
#if TUNER_ENABLED
    tunerDisp();
#endif
  }

  // 画面更新------------------------------
  ssd1306_UpdateScreen(&hi2c1);
  //HAL_Delay(10);

  // LED表示------------------------------
  uint8_t r = (fxColorList[fxNum] >> 8) & 0b0000000011111000; // RGB565を変換 PWMで色を制御する場合使えるかも
  uint8_t g = (fxColorList[fxNum] >> 3) & 0b0000000011111100;
  uint8_t b = (fxColorList[fxNum] << 3) & 0b0000000011111000;
  if (r && fxOn) HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
  else HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
  if (g && fxOn) HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
  else HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
  if (b && fxOn) HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET);
  else HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);

}

void mute() // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<エフェクト切替時、データ保存時のミュート
{
  for (uint16_t i = 0; i < BLOCK_SIZE*4; i++)
  {
    TxBuffer[i] = 0;
  }
}

void fxChange() // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<エフェクト変更
{
  mute();
  fxDeinit();
  for (int i = 0; i < MAX_FX_NUM; i++)
  {
    fxNum = (MAX_FX_NUM + fxNum + fxChangeFlag) % MAX_FX_NUM; // 最大値←→最小値で循環
    if (fxEnabled[fxNum]) break; // エフェクト有効の時は処理終了、無効の時は次のエフェクトへ
  }
  fxParamIndex = 0;
  cursorPosition = 0;
  fxInit();
  fxChangeFlag = 0;
}

void swProcess(uint8_t num) // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<スイッチ処理
{
  static uint32_t swCount[10] = {}; // スイッチが押されている間カウントアップ

  switch(num)
  {
    case 0: // 左上スイッチ --------------------------------------------------------
      if (!HAL_GPIO_ReadPin(SW0_UPPER_L_GPIO_Port, SW0_UPPER_L_Pin))
      {
        swCount[num]++;
        if (swCount[num] == longPushCount) // 長押し 1回のみ動作
        {
          if (swCount[num+1] > shortPushCount) // 左下スイッチが押されている場合
          {
            swCount[num+1] = longPushCount; // 左下スイッチも長押し済み扱いにする
            saveData(); // データ保存
          }
          else
          {
            fxChangeFlag = -1;
          }
        }
      }
      else
      {
        if (swCount[num] >= shortPushCount && swCount[num] < longPushCount) // 短押し 離した時の処理
        {
          if (swCount[num+2] > shortPushCount) // 右上スイッチが押されている場合、パラメータ数値を最大値へ
          {
            swCount[num+2] = longPushCount + 1; // 右上スイッチは長押し済み扱いにする
            fxParam[fxParamIndex] = fxParamMax[fxParamIndex];
          }
          else
          { // エフェクトパラメータ選択位置変更 0→最大値で循環
            fxParamIndex = (fxParamIndexMax + 1 + fxParamIndex - 1) % (fxParamIndexMax + 1);
            cursorPosition = fxParamIndex % 6;
          }

        }
        swCount[num] = 0;
      }
      break;
    case 1: // 左下スイッチ --------------------------------------------------------
      if (!HAL_GPIO_ReadPin(SW1_LOWER_L_GPIO_Port, SW1_LOWER_L_Pin))
      {
        swCount[num]++;
        if (swCount[num] == longPushCount) // 長押し 1回のみ動作
        {
          if (swCount[num-1] > shortPushCount) // 左上スイッチが押されている場合
          {
            swCount[num-1] = longPushCount; // 左上スイッチも長押し済み扱いにする
            saveData(); // データ保存
          }
          else
          {
            fxChangeFlag = 1;
          }
        }
      }
      else
      {
        if (swCount[num] >= shortPushCount && swCount[num] < longPushCount) // 短押し 離した時の処理
        {
          if (swCount[num+2] > shortPushCount) // 右下スイッチが押されている場合、パラメータ数値を最小値へ
          {
            swCount[num+2] = longPushCount + 1; // 右下スイッチは長押し済み扱いにする
            fxParam[fxParamIndex] = fxParamMin[fxParamIndex];
          }
          else
          { // エフェクトパラメータ選択位置変更 最大値→0で循環
            fxParamIndex = (fxParamIndex + 1) % (fxParamIndexMax + 1);
            cursorPosition = fxParamIndex % 6;
          }
        }
        swCount[num] = 0;
      }
      break;
    case 2: // 右上スイッチ --------------------------------------------------------
      if (!HAL_GPIO_ReadPin(SW2_UPPER_R_GPIO_Port, SW2_UPPER_R_Pin))
      {
        swCount[num]++;
        if (swCount[num] >= longPushCount/2 && (swCount[num] % (longPushCount/4)) == 0) // 長押し 繰り返し動作
        {
          fxParam[fxParamIndex] = min(fxParam[fxParamIndex] + 10, fxParamMax[fxParamIndex]);
        }
      }
      else
      {
        if (swCount[num] >= shortPushCount && swCount[num] < longPushCount) // 短押し 離した時の処理
        {
          if (swCount[num+1] > shortPushCount) // 右下スイッチが押されている場合、パラメータ数値を中間値へ
          {
            swCount[num+1] = longPushCount + 1; // 右下スイッチは長押し済み扱いにする
            fxParam[fxParamIndex] = (fxParamMin[fxParamIndex] + fxParamMax[fxParamIndex]) / 2;
          }
          else if (swCount[num-2] > shortPushCount) // 左上スイッチが押されている場合、パラメータ数値を最大値へ
          {
            swCount[num-2] = longPushCount + 1; // 左上スイッチは長押し済み扱いにする
            fxParam[fxParamIndex] = fxParamMax[fxParamIndex];
          }
          else fxParam[fxParamIndex] = min(fxParam[fxParamIndex] + 1, fxParamMax[fxParamIndex]);
        }
        swCount[num] = 0;
      }
      break;
    case 3: // 右下スイッチ --------------------------------------------------------
      if (!HAL_GPIO_ReadPin(SW3_LOWER_R_GPIO_Port, SW3_LOWER_R_Pin))
      {
        swCount[num]++;
        if (swCount[num] >= longPushCount/2 && (swCount[num] % (longPushCount/4)) == 0) // 長押し 繰り返し動作
        {
          {
            fxParam[fxParamIndex] = max(fxParam[fxParamIndex] - 10, fxParamMin[fxParamIndex]);
          }
        }
      }
      else
      {
        if (swCount[num] >= shortPushCount && swCount[num] < longPushCount) // 短押し 離した時の処理
        {
          if (swCount[num-1] > shortPushCount) // 右上スイッチが押されている場合、パラメータ数値を中間値へ
          {
            swCount[num-1] = longPushCount + 1; // 右上スイッチは長押し済み扱いにする
            fxParam[fxParamIndex] = (fxParamMin[fxParamIndex] + fxParamMax[fxParamIndex]) / 2;
          }
          else if (swCount[num-2] > shortPushCount) // 左下スイッチが押されている場合、パラメータ数値を最小値へ
          {
            swCount[num-2] = longPushCount + 1; // 左下スイッチは長押し済み扱いにする
            fxParam[fxParamIndex] = fxParamMin[fxParamIndex];
          }
          else fxParam[fxParamIndex] = max(fxParam[fxParamIndex] - 1, fxParamMin[fxParamIndex]);
        }
        swCount[num] = 0;
      }
      break;
    default:
      break;
  }
}

void footSwProcess() // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<フットスイッチ処理
{
  static uint32_t footSwCount = 0; // スイッチが押されている間カウントアップ
  static float tmpTapTime = 0; // タップ間隔時間 一時保存用

  if (!HAL_GPIO_ReadPin(SW4_FOOT_GPIO_Port, SW4_FOOT_Pin))
  {
    footSwCount++;
    if (mode == TAP && footSwCount == 4*shortPushCount) // スイッチを押した時のタップ間隔時間を記録
    {
      tmpTapTime = (float)callbackCount * i2sInterruptInterval * 1000.0f;
      callbackCount = 0;
    }
    if (footSwCount == 4*longPushCount) // 長押し
    {
      if (mode == NORMAL)
      {
#if TAP_ENABLED
        mode = TAP; // タップテンポモードへ
        callbackCount = 0;
#endif
      }
      else
      {
        mode = NORMAL; // タップテンポモード、チューナーモード終了
        tapTime = 0.0f; // タップ時間リセット
      }
    }
    if (footSwCount == 12*longPushCount) // 3倍長押し
    {
      if (mode == TAP)
      {
#if TUNER_ENABLED
        mute();
        mode = TUNER; // チューナーモードへ
#endif
      }
    }
  }
  else
  {
    if (footSwCount >= 4*shortPushCount && footSwCount < 4*longPushCount) // 短押し 離した時の処理
    {
      if (mode == NORMAL) fxOn = !fxOn;
      else if (mode == TAP)
      { // スイッチを押した時記録していたタップ間隔時間をスイッチを離した時に反映させる
        if (100.0f < tmpTapTime && tmpTapTime < MAX_TAP_TIME) tapTime = tmpTapTime;
        else tapTime = 0.0f;
      }
    }
    footSwCount = 0;
  }

}

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<DMA用に上位16ビットと下位16ビットを入れ替える
// 負の値の場合に備えて右シフトの場合0埋めする
int32_t swap16(int32_t x)
{
  return (0x0000FFFF & x >> 16) | x << 16;
}

void mainProcess(uint16_t start_sample) // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<メイン信号処理等
{
  DWT->CYCCNT = 0; // CPU使用率計算用 CPUサイクル数をリセット

  float xL[BLOCK_SIZE] = {}; // Lch float計算用データ
  float xR[BLOCK_SIZE] = {}; // Rch float計算用データ 不使用

  for (uint16_t i = 0; i < BLOCK_SIZE; i++)
  {
     uint16_t m = (start_sample + i) * 2; // データ配列の偶数添字計算 Lch（Rch不使用）

    // 受信データを計算用データ配列へ 値を-1～+1(float)へ変更
    xL[i] = (float)swap16(RxBuffer[m]) / 2147483648.0f;
  }

  if (mode == TUNER)
  {
#if TUNER_ENABLED
    tunerProcess(xL, xR); // チューナー
#endif
  }
  else
  {
    fxProcess(xL, xR); // エフェクト処理 計算用配列を渡す
  }

  for (uint16_t i = 0; i < BLOCK_SIZE; i++)
  {
    if (xL[i] < -1.0f) xL[i] = -1.0f; // オーバーフロー防止
    if (xL[i] > 0.99f) xL[i] = 0.99f;

    uint16_t m = (start_sample + i) * 2; // データ配列の偶数添字計算 Lch（Rch不使用）

    // 計算済データを送信バッファへ 値を32ビット整数へ戻す
    TxBuffer[m] = swap16((int32_t)(2147483648.0f * xL[i]));
  }

  callbackCount++; // I2Sの割り込みごとにカウントアップ タイマとして利用
  footSwProcess(); // フットスイッチ処理
  if (mode == NORMAL)
  {
    swProcess(callbackCount % 4); // 割り込みごとにスイッチ処理するが、スイッチ1つずつを順番に行う
    cpuUsageCycleMax[fxNum] = max(cpuUsageCycleMax[fxNum], DWT->CYCCNT); // CPU使用率計算用
  }
  if (fxChangeFlag) fxChange(); // エフェクト変更 ※ディレイメモリ確保前に信号処理に進まないように割り込み内で行う

}

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<I2Sの受信バッファに半分データがたまったときの割り込み
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  mainProcess(0); // 0 ～ 15 を処理(0 ～ BLOCK_SIZE-1)
}

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<I2Sの受信バッファに全データがたまったときの割り込み
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  mainProcess(BLOCK_SIZE); // 16 ～ 31 を処理(BLOCK_SIZE ～ BLOCK_SIZE*2-1)
}

void loadData() // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<データ読み込み
{
  uint32_t addr = DATA_ADDR;
  for (uint16_t i = 0; i < MAX_FX_NUM; i++) // エフェクトデータ フラッシュ読込
  {
    for (uint16_t j = 0; j < 20; j++)
    {
      fxAllData[i][j] = *((uint16_t*)addr);
      addr += 2;
    }
  }
  fxNum = *((uint16_t*)addr);
  if (fxNum >= MAX_FX_NUM) fxNum = 0;
}

void saveData() // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<データ保存
{
  mute();
  ssd1306_xyWriteStrWT(fxNameXY[0], fxNameXY[1], "WRITING... ", Font_7x10);

  eraseData(); // フラッシュ消去

  HAL_FLASH_Unlock(); // フラッシュ ロック解除

  uint32_t addr = DATA_ADDR;

  for (uint16_t j = 0; j < 20; j++) // 現在のパラメータをデータ配列へ移す
  {
    fxAllData[fxNum][j] = fxParam[j];
  }

  for (uint16_t i = 0; i < MAX_FX_NUM; i++) // フラッシュ書込
  {
    for (uint16_t j = 0; j < 20; j++)
    {
      HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, fxAllData[i][j]);
      addr += 2;
    }
  }
  HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, fxNum); // 保存時のエフェクト番号記録

  HAL_FLASH_Lock(); // フラッシュ ロック

  statusStr = "STORED!    "; // ステータス表示
  callbackCount = 0;

  DWT->CYCCNT = 0; // CPU使用率計算に影響しないように、CPUサイクル数を一旦リセット
}

void eraseData() // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<データ全消去
{
  HAL_FLASH_Unlock(); // フラッシュ ロック解除
  FLASH_EraseInitTypeDef erase;
  uint32_t error = 0;
  erase.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase.Sector = DATA_SECTOR;
  erase.NbSectors = 1;
  erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  HAL_FLASHEx_Erase(&erase, &error); // フラッシュ消去
  HAL_FLASH_Lock(); // フラッシュ ロック
}
