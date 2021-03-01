#ifndef USER_MAIN_H
#define USER_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

void mainInit();

void mainLoop();

#ifdef __cplusplus
}
#endif

void fxChange();

void loadData();
void saveData();
void eraseData();

#endif // USER_MAIN_H
