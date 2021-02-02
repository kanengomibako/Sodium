#ifndef USER_MAIN_H
#define USER_MAIN_H

/* 各定数設定 --------------------------*/

// ペダル名称表示
#define PEDAL_NAME "  Sodium v0.5test "

// ブロックサイズ まとめて処理を行う数
#define BLOCK_SIZE 16

// 円周率
#define PI 3.14159265359f

// サンプリング周波数
#define SAMPLING_FREQ 44108.07f

// スイッチ短押し時間 ミリ秒
#define SHORT_PUSH_MSEC 20

// スイッチ長押し時間 ミリ秒
#define LONG_PUSH_MSEC 1000

void mainInit();

void mainLoop();

float lpf(float x,float fc);

float hpf(float x,float fc);

#endif // USER_MAIN_H
