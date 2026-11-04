#pragma once
#include <windows.h>

enum { CLICK_TARGET_MOUSE = 0, CLICK_TARGET_KEY = 1 };
enum {
  CLICK_BUTTON_LEFT = 0,
  CLICK_BUTTON_RIGHT = 1,
  CLICK_BUTTON_MIDDLE = 2,
  CLICK_BUTTON_X1 = 3,
  CLICK_BUTTON_X2 = 4
};
enum { CLICK_SINGLE = 0, CLICK_DOUBLE = 1, CLICK_HOLD = 2 };

struct ClickerParams {
  int target = CLICK_TARGET_MOUSE;
  long long interval = 100;
  int button = CLICK_BUTTON_LEFT;
  int type = CLICK_SINGLE;
  int hold = 100;
  int keyMods = 0;
  int keyVk = 0;
  int jitter = 0;
  int limit = 0;
  int timeLimit = 0;
  int jiggle = 0;
  int holdKeyMods = 0;
  int holdKeyVk = 0;
  int holdMouse = 0;
};

void Clicker_Start(const ClickerParams &params);
void Clicker_Stop();
bool Clicker_IsRunning();
int Clicker_StopReason();
DWORD Clicker_StartTick();
