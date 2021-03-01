#include "common.h"
#include "user_main.h"
#include "main.h"
#include "ssd1306.hpp"
#include "fx.hpp"

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
const uint32_t shortPushCount = 1 + SHORT_PUSH_MSEC / (5 * 1000 * i2sInterruptInterval); // 1つのスイッチは5回に1回の読取のため5をかける
const uint32_t longPushCount = 1 + LONG_PUSH_MSEC / (5 * 1000 * i2sInterruptInterval);
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

uint8_t cursorPosition = 0; // パラメータ選択カーソル位置 0 ～ 2
string statusStr = PEDAL_NAME; // ステータス表示文字列

uint8_t screenMode = 0; // 画面モード 0:通常画面 1:未定 2:未定

const uint32_t flashAddr = 0x08020000; // データ保存先（セクタ5）開始アドレス

const uint8_t fxNameXY[2] = {0,0}; // エフェクト名 ステータス 表示位置
const uint8_t fxPageXY[2] = {88,0}; // エフェクトパラメータ ページ 表示位置
const uint8_t percentXY[2] = {107,0}; // 処理時間% 表示位置
const uint8_t cursorPositionXY[3][2] = {{0,10},{0,27},{0,44}}; // カーソル位置
const uint8_t fxParamNameXY[3][2] = {{11,11},{11,28},{11,45}}; // エフェクトパラメータ名表示位置
const uint8_t fxParamStrXY[3][2]  = {{117,11},{117,28},{117,45}}; // エフェクトパラメータ数値表示 一番右の文字の位置

void mainInit() // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<最初に1回のみ行う処理
{
  // Denormalized numbers（非正規化数）を0として扱うためFPSCRレジスタ変更
  asm("VMRS r0, FPSCR");
  asm("ORR r0, r0, #(1 << 24)");
  asm("VMSR FPSCR, r0");

  // 時間計測用設定
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  // ディスプレイ点灯確認
  ssd1306_Init(&hi2c1);
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
  fxChange();
}

void mainLoop() // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<メインループ
{

  if (screenMode == 0) // 通常画面 始まり*****************************
  {

    uint8_t fxPage = fxParamIndex / 3; // エフェクトパラメータページ番号

    ssd1306_Fill(Black); // 一旦画面表示を全て消す

    // ステータス表示------------------------------
    if (callbackCount > statusDispCount) // ステータス表示が変わり一定時間経過後、デフォルト表示に戻す
    {
      statusStr = fxNameList[fxNum]; // エフェクト名表示
    }
    ssd1306_xyWriteStrWT(fxNameXY[0], fxNameXY[1], statusStr, Font_7x10);

    // エフェクトパラメータ数値表示------------------------------
    for (int i = 0; i < 3; i++)
    {
      fxSetParamStr(i+3*fxPage); // パラメータ数値を文字列に変換
      ssd1306_R_xyWriteStrWT(fxParamStrXY[i][0], fxParamStrXY[i][1], fxParamStr[i+3*fxPage], Font_11x18);
    }

    // エフェクトパラメータ名称表示------------------------------
    for (int i = 0; i < 3; i++)
    {
      ssd1306_xyWriteStrWT(fxParamNameXY[i][0], fxParamNameXY[i][1], fxParamName[i+3*fxPage], Font_11x18);
    }

    // エフェクトパラメータページ番号表示------------------------------
    string fxPageStr = "P" + std::to_string(fxPage + 1);
    ssd1306_xyWriteStrWT(fxPageXY[0], fxPageXY[1], fxPageStr, Font_7x10);

    // カーソル表示------------------------------
    ssd1306_xyWriteStrWT(cursorPositionXY[cursorPosition][0], cursorPositionXY[cursorPosition][1], ">", Font_11x18);

    // CPU使用率表示------------------------------
    uint8_t cpuUsagePercent = 100.0f * cpuUsageCycleMax[fxNum] / SystemCoreClock / i2sInterruptInterval;
    string percentStr = std::to_string(cpuUsagePercent);
    if (cpuUsagePercent < 10) percentStr = " " + percentStr + "%"; // 右揃えにする
    else percentStr = percentStr + "%";
    ssd1306_xyWriteStrWT(percentXY[0], percentXY[1], percentStr, Font_7x10);

    // 画面更新------------------------------
    ssd1306_UpdateScreen(&hi2c1);
    //HAL_Delay(10);

  } // 通常画面 終わり*****************************

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
  fxNum = (MAX_FX_NUM + fxNum + fxChangeFlag) % MAX_FX_NUM; // 最大値←→最小値で循環
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
    case 0: // 左上スイッチ ----------------------------
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
        { // エフェクトパラメータ選択位置変更 0→最大値で循環
          fxParamIndex = (fxParamIndexMax + 1 + fxParamIndex - 1) % (fxParamIndexMax + 1);
          cursorPosition = fxParamIndex % 3;
        }
        swCount[num] = 0;
      }
      break;
    case 1: // 左下スイッチ ----------------------------
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
        { // エフェクトパラメータ選択位置変更 最大値→0で循環
          fxParamIndex = (fxParamIndex + 1) % (fxParamIndexMax + 1);
          cursorPosition = fxParamIndex % 3;
        }
        swCount[num] = 0;
      }
      break;
    case 2: // 右上スイッチ ----------------------------
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
          fxParam[fxParamIndex] = min(fxParam[fxParamIndex] + 1, fxParamMax[fxParamIndex]);
        }
        swCount[num] = 0;
      }
      break;
    case 3: // 右下スイッチ ----------------------------
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
          fxParam[fxParamIndex] = max(fxParam[fxParamIndex] - 1, fxParamMin[fxParamIndex]);
        }
        swCount[num] = 0;
      }
      break;
    case 4: // フットスイッチ ----------------------------
      if (!HAL_GPIO_ReadPin(SW4_FOOT_GPIO_Port, SW4_FOOT_Pin))
      {
        swCount[num]++;
        if (swCount[num] == longPushCount) // 長押し 未使用
        {
          statusStr = "SW4LONGPUSH";
          callbackCount = 0;
        }
      }
      else
      {
        if (swCount[num] >= shortPushCount && swCount[num] < longPushCount) // 短押し 離した時の処理
        {
          fxOn = !fxOn;
        }
        swCount[num] = 0;
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

  fxProcess(xL, xR); // エフェクト処理 計算用配列を渡す

  for (uint16_t i = 0; i < BLOCK_SIZE; i++)
  {
    if (xL[i] < -1.0f) xL[i] = -1.0f; // オーバーフロー防止
    if (xL[i] > 0.99f) xL[i] = 0.99f;

    uint16_t m = (start_sample + i) * 2; // データ配列の偶数添字計算 Lch（Rch不使用）

    // 計算済データを送信バッファへ 値を32ビット整数へ戻す
    TxBuffer[m] = swap16((int32_t)(2147483648.0f * xL[i]));
  }

  swProcess(callbackCount % 5); // 割り込みごとにスイッチ処理するが、スイッチ1つずつを順番に行う
  callbackCount++; // I2Sの割り込みごとにカウントアップ タイマとして利用
  cpuUsageCycleMax[fxNum] = max(cpuUsageCycleMax[fxNum], DWT->CYCCNT); // CPU使用率計算用
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
  uint32_t addr = flashAddr;
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

  HAL_FLASH_Unlock(); // フラッシュ ロック解除
  FLASH_EraseInitTypeDef erase;
  uint32_t error = 0;
  erase.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase.Sector = FLASH_SECTOR_5;
  erase.NbSectors = 1;
  erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  HAL_FLASHEx_Erase(&erase, &error); // フラッシュ消去

  uint32_t addr = flashAddr;

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
  erase.Sector = FLASH_SECTOR_5;
  erase.NbSectors = 1;
  erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  HAL_FLASHEx_Erase(&erase, &error); // フラッシュ消去
  HAL_FLASH_Lock(); // フラッシュ ロック
}
