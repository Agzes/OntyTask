#pragma once
#include <string>
#include <vector>
#include <windows.h>

#define MAX_PANIC_KEYS 32

struct Config {
  int x, y, speed, loops, continuous, topmost;
  double speedCustom;
  int loopDelay;
  int recordCountdown;
  int playCountdown;
  int recordMods, recordVk, playMods, playVk;
  int theme, lang, statusbarTop, autoupdate, trayIcon, uiIcon;
  std::wstring themeFile;
  int accent;
  int trayEnable;
  int opacity;
  int uiScale;
  int panicPause, panicScroll, panicEsc, panicMouseMove;
  int panicKeyCount;
  int panicKeyMods[MAX_PANIC_KEYS];
  int panicKeyVk[MAX_PANIC_KEYS];
  int statusbarShow, statusbarFollowCursor;
  int autoMinimize;
  int mode;
  int clickTarget, clickButton, clickType;
  long long clickInterval;
  int clickHold;
  int clickKeyMods, clickKeyVk;
  int clickJitter, clickLimit, clickTimeLimit, clickJiggle;
  int clickHoldKeyMods, clickHoldKeyVk;
  int clickHoldMouse;
};

extern Config g_cfg;
extern HWND g_hwnd;
extern HINSTANCE g_hInst;
extern std::vector<std::wstring> g_recentFiles;

void ConfigLoad();
void ConfigSave();
void RecentAdd(const std::wstring &path);
void RecentClear();
bool IsFileAssociationRegistered();
bool RegisterFileAssociation(bool enable);
