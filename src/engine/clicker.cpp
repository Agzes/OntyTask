#include "engine/clicker.h"
#include "engine/engine.h"
#include <atomic>
#include <mmsystem.h>
#include <thread>

static std::thread g_thread;
static std::atomic<bool> g_run{false};
static std::atomic<bool> g_running{false};
static std::atomic<DWORD> g_clickStartTick{0};
static std::atomic<int> g_stopReason{0};
static std::atomic<int> g_mouseDown{0};
static HHOOK g_mouseHook = nullptr;
static HANDLE g_stopEvent = nullptr;
static ClickerParams g_params;
static const ULONG_PTR kInjected = 0x4F4E5459;

static void SendMouseAction(int button, bool down) {
  INPUT in = {};
  in.type = INPUT_MOUSE;
  in.mi.dwExtraInfo = kInjected;
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

static int RandRange(int lo, int hi) {
  static unsigned seed = 0;
  if (!seed)
    seed = GetTickCount() | 1u;
  seed = seed * 1664525u + 1013904223u;
  return lo + (int)(seed % (unsigned)(hi - lo + 1));
}

static int MouseHoldVk(int mouse) {
  static const int keys[6] = {0,          VK_LBUTTON,  VK_RBUTTON,
                              VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2};
  return mouse > 0 && mouse < 6 ? keys[mouse] : 0;
}

static bool HoldTriggered(const ClickerParams &p) {
  if (p.holdMouse) {
    int bit = 1 << (p.holdMouse - 1);
    if (g_mouseHook)
      return (g_mouseDown.load() & bit) != 0;
    int vk = MouseHoldVk(p.holdMouse);
    return vk && (GetAsyncKeyState(vk) & 0x8000) != 0;
  }
  if (p.holdKeyVk)
    return HotkeyDown(p.holdKeyMods, p.holdKeyVk);
  return true;
}

static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode >= 0) {
    MSLLHOOKSTRUCT *ms = (MSLLHOOKSTRUCT *)lParam;
    if (ms->dwExtraInfo != kInjected) {
      int down = 0, up = 0;
      if (wParam == WM_LBUTTONDOWN)
        down = 1;
      else if (wParam == WM_RBUTTONDOWN)
        down = 2;
      else if (wParam == WM_MBUTTONDOWN)
        down = 4;
      else if (wParam == WM_XBUTTONDOWN)
        down = (ms->mouseData & XBUTTON1) ? 8 : 16;
      else if (wParam == WM_LBUTTONUP)
        up = 1;
      else if (wParam == WM_RBUTTONUP)
        up = 2;
      else if (wParam == WM_MBUTTONUP)
        up = 4;
      else if (wParam == WM_XBUTTONUP)
        up = (ms->mouseData & XBUTTON1) ? 8 : 16;
      if (down)
        g_mouseDown.fetch_or(down);
      if (up)
        g_mouseDown.fetch_and(~up);
    }
  }
  return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
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
  DWORD start = GetTickCount();
  long long done = 0;
  while (g_run.load()) {
    if (!HoldTriggered(g_params)) {
      if (WaitStop(10))
        break;
      continue;
    }
    if (SelfGuard(g_params)) {
      if (WaitStop(50))
        break;
      continue;
    }
    if (g_params.timeLimit > 0 &&
        GetTickCount() - start >= (DWORD)g_params.timeLimit * 1000) {
      g_stopReason.store(2);
      break;
    }
    if (g_params.limit > 0 && done >= g_params.limit) {
      g_stopReason.store(1);
      break;
    }
    POINT orig = {};
    bool jiggle = g_params.jiggle > 0 && g_params.target == CLICK_TARGET_MOUSE;
    if (jiggle) {
      GetCursorPos(&orig);
      SetCursorPos(orig.x + RandRange(-g_params.jiggle, g_params.jiggle),
                   orig.y + RandRange(-g_params.jiggle, g_params.jiggle));
    }
    bool stopped = false;
    if (g_params.type == CLICK_HOLD) {
      HoldDown(g_params);
      stopped = WaitStop(g_params.hold);
      HoldUp(g_params);
    } else {
      PerformClick(g_params);
    }
    if (jiggle)
      SetCursorPos(orig.x, orig.y);
    done += g_params.type == CLICK_DOUBLE ? 2 : 1;
    if (stopped)
      break;
    long long wait = g_params.interval;
    if (g_params.jitter > 0) {
      int pct = 100 - g_params.jitter + RandRange(0, g_params.jitter * 2);
      wait = wait * pct / 100;
      if (wait < 1)
        wait = 1;
    }
    if (g_params.timeLimit > 0) {
      long long left = (long long)g_params.timeLimit * 1000 -
                       (long long)(GetTickCount() - start);
      if (left < wait)
        wait = left;
    }
    if (wait <= 0) {
      g_stopReason.store(2);
      break;
    }
    if (WaitStop(wait))
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
  if (g_params.jitter < 0)
    g_params.jitter = 0;
  if (g_params.jitter > 90)
    g_params.jitter = 90;
  if (g_params.limit < 0)
    g_params.limit = 0;
  if (g_params.timeLimit < 0)
    g_params.timeLimit = 0;
  if (g_params.jiggle < 0)
    g_params.jiggle = 0;
  if (g_params.holdMouse < 0)
    g_params.holdMouse = 0;
  if (g_params.holdMouse > 5)
    g_params.holdMouse = 5;
  if (!g_stopEvent)
    g_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  if (!g_stopEvent)
    return;
  ResetEvent(g_stopEvent);
  if (g_thread.joinable())
    g_thread.join();
  g_run.store(true);
  g_running.store(true);
  g_stopReason.store(0);
  g_mouseDown.store(0);
  int holdVk = MouseHoldVk(g_params.holdMouse);
  if (holdVk && (GetAsyncKeyState(holdVk) & 0x8000))
    g_mouseDown.fetch_or(1 << (g_params.holdMouse - 1));
  if (!g_mouseHook)
    g_mouseHook = SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc,
                                    GetModuleHandleW(nullptr), 0);
  g_clickStartTick.store(GetTickCount());
  g_thread = std::thread(ClickerThread);
}

void Clicker_Stop() {
  g_run.store(false);
  if (g_stopEvent)
    SetEvent(g_stopEvent);
  if (g_thread.joinable())
    g_thread.join();
  if (g_mouseHook) {
    UnhookWindowsHookEx(g_mouseHook);
    g_mouseHook = nullptr;
  }
  g_mouseDown.store(0);
  g_running.store(false);
}

bool Clicker_IsRunning() { return g_running.load(); }

int Clicker_StopReason() { return g_stopReason.load(); }

DWORD Clicker_StartTick() { return g_clickStartTick.load(); }
