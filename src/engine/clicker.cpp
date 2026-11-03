#include "engine/clicker.h"
#include "engine/engine.h"
#include <atomic>
#include <mmsystem.h>
#include <thread>

static std::thread g_thread;
static std::atomic<bool> g_run{false};
static std::atomic<bool> g_running{false};
static std::atomic<DWORD> g_clickStartTick{0};
static HANDLE g_stopEvent = nullptr;
static ClickerParams g_params;

static void SendMouseAction(int button, bool down) {
  INPUT in = {};
  in.type = INPUT_MOUSE;
  switch (button) {
  case CLICK_BUTTON_RIGHT:
    in.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    break;
  case CLICK_BUTTON_MIDDLE:
    in.mi.dwFlags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    break;
  case CLICK_BUTTON_X1:
    in.mi.dwFlags = down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
    in.mi.mouseData = XBUTTON1;
    break;
  case CLICK_BUTTON_X2:
    in.mi.dwFlags = down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
    in.mi.mouseData = XBUTTON2;
    break;
  default:
    in.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    break;
  }
  SendInput(1, &in, sizeof(in));
}

static void SendMouseClick(int button) {
  SendMouseAction(button, true);
  SendMouseAction(button, false);
}

static void SendKeyEvent(WORD vk, bool up) {
  INPUT in = {};
  in.type = INPUT_KEYBOARD;
  in.ki.wVk = vk;
  in.ki.dwFlags = up ? KEYEVENTF_KEYUP : 0;
  SendInput(1, &in, sizeof(in));
}

static void SendKeyTap(const ClickerParams &p) {
  WORD mods[4] = {VK_CONTROL, VK_SHIFT, VK_MENU, VK_LWIN};
  int bits[4] = {HK_CTRL, HK_SHIFT, HK_ALT, HK_WIN};
  int count = 0;
  for (int i = 0; i < 4; i++) {
    if (p.keyMods & bits[i]) {
      SendKeyEvent(mods[i], false);
      count++;
    }
  }
  SendKeyEvent((WORD)p.keyVk, false);
  SendKeyEvent((WORD)p.keyVk, true);
  for (int i = count - 1; i >= 0; i--)
    SendKeyEvent(mods[i], true);
}

static void HoldDown(const ClickerParams &p) {
  if (p.target == CLICK_TARGET_KEY) {
    WORD mods[4] = {VK_CONTROL, VK_SHIFT, VK_MENU, VK_LWIN};
    int bits[4] = {HK_CTRL, HK_SHIFT, HK_ALT, HK_WIN};
    for (int i = 0; i < 4; i++) {
      if (p.keyMods & bits[i])
        SendKeyEvent(mods[i], false);
    }
    SendKeyEvent((WORD)p.keyVk, false);
    return;
  }
  SendMouseAction(p.button, true);
}

static void HoldUp(const ClickerParams &p) {
  if (p.target == CLICK_TARGET_KEY) {
    WORD mods[4] = {VK_CONTROL, VK_SHIFT, VK_MENU, VK_LWIN};
    int bits[4] = {HK_CTRL, HK_SHIFT, HK_ALT, HK_WIN};
    SendKeyEvent((WORD)p.keyVk, true);
    for (int i = 3; i >= 0; i--) {
      if (p.keyMods & bits[i])
        SendKeyEvent(mods[i], true);
    }
    return;
  }
  SendMouseAction(p.button, false);
}

static void PerformClick(const ClickerParams &p) {
  if (p.target == CLICK_TARGET_KEY) {
    if (!p.keyVk)
      return;
    SendKeyTap(p);
    if (p.type == CLICK_DOUBLE) {
      Sleep(25);
      SendKeyTap(p);
    }
    return;
  }
  SendMouseClick(p.button);
  if (p.type == CLICK_DOUBLE) {
    Sleep(25);
    SendMouseClick(p.button);
  }
}

static bool WaitStop(long long ms) {
  while (ms > 0 && g_run.load()) {
    DWORD chunk = ms > 0x7FFFFFFFLL ? 0x7FFFFFFF : (DWORD)ms;
    if (WaitForSingleObject(g_stopEvent, chunk) == WAIT_OBJECT_0)
      return true;
    ms -= chunk;
  }
  return !g_run.load();
}

static bool SelfGuard(const ClickerParams &p) {
  HWND fg = GetForegroundWindow();
  if (!fg)
    return false;
  DWORD pid = 0;
  GetWindowThreadProcessId(fg, &pid);
  if (pid != GetCurrentProcessId())
    return false;
  if (p.target == CLICK_TARGET_KEY)
    return true;
  POINT pt = {};
  GetCursorPos(&pt);
  RECT r = {};
  GetWindowRect(fg, &r);
  return PtInRect(&r, pt) != 0;
}

static void ClickerThread() {
  timeBeginPeriod(1);
  while (g_run.load()) {
    if (SelfGuard(g_params)) {
      if (WaitStop(50))
        break;
      continue;
    }
    bool stopped = false;
    if (g_params.type == CLICK_HOLD) {
      HoldDown(g_params);
      stopped = WaitStop(g_params.hold);
      HoldUp(g_params);
    } else {
      PerformClick(g_params);
    }
    if (stopped)
      break;
    if (WaitStop(g_params.interval))
      break;
  }
  timeEndPeriod(1);
  g_running.store(false);
}

void Clicker_Start(const ClickerParams &params) {
  if (g_run.load())
    return;
  g_params = params;
  if (g_params.interval < 1)
    g_params.interval = 1;
  if (!g_stopEvent)
    g_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  if (!g_stopEvent)
    return;
  ResetEvent(g_stopEvent);
  if (g_thread.joinable())
    g_thread.join();
  g_run.store(true);
  g_running.store(true);
  g_clickStartTick.store(GetTickCount());
  g_thread = std::thread(ClickerThread);
}

void Clicker_Stop() {
  g_run.store(false);
  if (g_stopEvent)
    SetEvent(g_stopEvent);
  if (g_thread.joinable())
    g_thread.join();
  g_running.store(false);
}

bool Clicker_IsRunning() { return g_running.load(); }

DWORD Clicker_StartTick() { return g_clickStartTick.load(); }
