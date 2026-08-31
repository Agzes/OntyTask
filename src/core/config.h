#pragma once
#include <string>
#include <vector>
#include <windows.h>

struct Config {
  int x, y, speed, loops, continuous, topmost;
  double speedCustom;
  int recordMods, recordVk, playMods, playVk;
  int theme, lang, statusbarTop, autoupdate, trayIcon, uiIcon;
  int panicPause, panicScroll, panicEsc, panicMouseMove;
  int statusbarShow, statusbarFollowCursor;
  int autoMinimize;
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
