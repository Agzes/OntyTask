#include "ui/statusbar.h"
#include "core/config.h"
#include "engine/engine.h"
#include "lang/lang.h"
#include "ui/gfx.h"
#include <cmath>

static const wchar_t SB_CLASS[] = L"OntyTaskStatus";
static HWND g_sb = nullptr;
static std::wstring g_msg;
static bool g_persistent = false;
static int g_alpha = 0;
static int g_alphaTarget = 0;
static double g_pos = 0;
static double g_posTarget = 0;
static bool g_engaged = false;
static int g_dpi = 96;
static const int SB_H = 30;

static int g_w = 0;
static bool g_dirty = true;
static int g_curW = 0;
static int g_curH = 0;

static int Sbx() { return MulDiv(12, g_dpi, 96); }
static int Sby() { return MulDiv(14, g_dpi, 96); }
static int SbH() { return MulDiv(SB_H, g_dpi, 96); }

static double Home() { return g_cfg.statusbarTop ? 0.0 : 1.0; }
static double Opposite() { return g_cfg.statusbarTop ? 1.0 : 0.0; }

static std::wstring SbText() {
  return std::wstring(L(L_APPNAME)) + L" \u2022 " + g_msg;
}

static void ApplyRegion(int w, int h) {
  RECT rc = {0, 0, w, h};
  int rad = MulDiv(8, g_dpi, 96);
  HRGN rgn =
      CreateRoundRectRgn(0, 0, rc.right + 1, rc.bottom + 1, rad * 2, rad * 2);
  SetWindowRgn(g_sb, rgn, FALSE);
  g_curW = w;
  g_curH = h;
}

static void RefreshMeasure() {
  HDC dc = CreateCompatibleDC(nullptr);
  std::wstring s = SbText();
  HGDIOBJ ob = SelectObject(dc, ThFont(9));
  SIZE sz = {};
  GetTextExtentPoint32W(dc, s.c_str(), (int)s.size(), &sz);
  SelectObject(dc, ob);
  DeleteDC(dc);
  g_w = Sbx() * 2 + sz.cx + 10;
  g_dirty = false;
}

static HMONITOR GetTargetMonitor() {
  if (g_cfg.statusbarFollowCursor) {
    POINT cp = {};
    GetCursorPos(&cp);
    return MonitorFromPoint(cp, MONITOR_DEFAULTTONEAREST);
  }
  return MonitorFromWindow(g_hwnd ? g_hwnd : g_sb, MONITOR_DEFAULTTONEAREST);
}

static void Layout() {
  if (!g_sb)
    return;
  if (g_dirty)
    RefreshMeasure();
  HMONITOR mon = GetTargetMonitor();
  MONITORINFO mi = {sizeof(mi)};
  GetMonitorInfoW(mon, &mi);
  int h = SbH();
  int x = mi.rcWork.left + ((mi.rcWork.right - mi.rcWork.left) - g_w) / 2;
  int topY = mi.rcWork.top + Sby();
  int botY = mi.rcWork.bottom - h - Sby();
  int y = (int)(topY + (botY - topY) * g_pos);
  SetWindowPos(g_sb, HWND_TOPMOST, x, y, g_w, h,
               SWP_NOACTIVATE | SWP_SHOWWINDOW);
  if (g_w != g_curW || h != g_curH)
    ApplyRegion(g_w, h);
  InvalidateRect(g_sb, nullptr, FALSE);
}

static RECT HomeRect() {
  HMONITOR mon = GetTargetMonitor();
  MONITORINFO mi = {sizeof(mi)};
  GetMonitorInfoW(mon, &mi);
  int h = SbH();
  int x = mi.rcWork.left + ((mi.rcWork.right - mi.rcWork.left) - g_w) / 2;
  int y =
      g_cfg.statusbarTop ? mi.rcWork.top + Sby() : mi.rcWork.bottom - h - Sby();
  RECT r = {x, y, x + g_w, y + h};
  return r;
}

static void HoverPoll() {
  if (!g_sb || g_alpha == 0)
    return;
  if (!g_cfg.statusbarShow) {
    ShowWindow(g_sb, SW_HIDE);
    return;
  }
  static HMONITOR lastMon = nullptr;
  if (g_cfg.statusbarFollowCursor) {
    HMONITOR curMon = GetTargetMonitor();
    if (curMon != lastMon) {
      lastMon = curMon;
      Layout();
    }
  }
  if (g_pos != g_posTarget)
    return;
  POINT cp;
  GetCursorPos(&cp);
  if (!g_engaged) {
    RECT home = HomeRect();
    if (PtInRect(&home, cp)) {
      g_engaged = true;
      g_posTarget = Opposite();
      SetTimer(g_sb, 11, 16, nullptr);
    }
  } else {
    RECT zone = HomeRect();
    InflateRect(&zone, 10, 10);
    if (!PtInRect(&zone, cp)) {
      g_engaged = false;
      g_posTarget = Home();
      SetTimer(g_sb, 11, 16, nullptr);
    }
  }
}

static LRESULT CALLBACK SbProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
  switch (msg) {
  case WM_CREATE:
    g_dpi = GfxDpi();
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HGDIOBJ old = SelectObject(mem, bm);
    Gdiplus::Graphics g(mem);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
    int rad = MulDiv(8, g_dpi, 96);
    GfxRoundRect(g, rc, GdiCol(ThBg2()), (REAL)rad, GdiCol(ThBorder()), true);
    if (g_dirty) {
      RefreshMeasure();
      Layout();
    }
    std::wstring s = SbText();
    Gdiplus::SolidBrush tb(GdiCol(ThText()));
    static Gdiplus::StringFormat sf;
    sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    sf.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
    {
      Gdiplus::Font f(mem, ThFont(9));
      Gdiplus::RectF r((REAL)Sbx(), 0, (REAL)(rc.right - Sbx() * 2 + 20),
                       (REAL)rc.bottom);
      g.DrawString(s.c_str(), -1, &f, r, &sf, &tb);
    }
    BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteObject(bm);
    DeleteDC(mem);
    EndPaint(hwnd, &ps);
    return 0;
  }
  case WM_MOUSEMOVE:
    HoverPoll();
    return 0;
  case WM_TIMER:
    if (w == 10) {
      int step = (int)((g_alphaTarget - g_alpha) * 0.35f);
      if (step == 0 && g_alpha != g_alphaTarget)
        step = g_alphaTarget > g_alpha ? 1 : -1;
      g_alpha += step;
      if (g_alpha < 0)
        g_alpha = 0;
      if (g_alpha > 255)
        g_alpha = 255;
      SetLayeredWindowAttributes(hwnd, 0, (BYTE)g_alpha, LWA_ALPHA);
      if (g_alpha == g_alphaTarget) {
        KillTimer(hwnd, 10);
        if (g_alphaTarget == 0)
          ShowWindow(hwnd, SW_HIDE);
      }
    } else if (w == 11) {
      g_pos += (g_posTarget - g_pos) * 0.30;
      if (fabs(g_pos - g_posTarget) < 0.008)
        g_pos = g_posTarget;
      if (g_pos == g_posTarget)
        KillTimer(hwnd, 11);
      Layout();
    } else if (w == 12) {
      KillTimer(hwnd, 12);
      StatusHide();
    } else if (w == 13) {
      HoverPoll();
    }
    return 0;
  }
  return DefWindowProcW(hwnd, msg, w, l);
}

static void Ensure() {
  if (g_sb && IsWindow(g_sb))
    return;
  WNDCLASSW wc = {};
  wc.lpfnWndProc = SbProc;
  wc.hInstance = g_hInst;
  wc.lpszClassName = SB_CLASS;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  RegisterClassW(&wc);
  g_sb = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE |
                             WS_EX_LAYERED,
                         SB_CLASS, L"", WS_POPUP, 0, 0, 10, 10, nullptr,
                         nullptr, g_hInst, nullptr);
  TryRoundCorners(g_sb);
  g_engaged = false;
  SetTimer(g_sb, 13, 100, nullptr);
}

void StatusShow(const wchar_t *msg, UINT ms) {
  if (!g_cfg.statusbarShow) {
    if (g_sb && IsWindow(g_sb))
      ShowWindow(g_sb, SW_HIDE);
    return;
  }
  Ensure();
  g_msg = msg;
  g_dirty = true;
  g_persistent = false;
  KillTimer(g_sb, 12);
  if (ms)
    SetTimer(g_sb, 12, ms, nullptr);
  g_pos = Home();
  g_posTarget = Home();
  g_engaged = false;
  Layout();
  g_alphaTarget = 255;
  SetTimer(g_sb, 10, 16, nullptr);
}

void StatusPersistent(const wchar_t *msg) {
  if (!g_cfg.statusbarShow) {
    if (g_sb && IsWindow(g_sb))
      ShowWindow(g_sb, SW_HIDE);
    return;
  }
  Ensure();
  g_msg = msg;
  g_dirty = true;
  g_persistent = true;
  KillTimer(g_sb, 12);
  g_pos = Home();
  g_posTarget = Home();
  g_engaged = false;
  Layout();
  g_alphaTarget = 255;
  SetTimer(g_sb, 10, 16, nullptr);
}

void StatusUpdate(const wchar_t *msg) {
  if (!g_sb || !g_persistent)
    return;
  g_msg = msg;
  g_dirty = true;
  Layout();
}

void StatusHide() {
  if (!g_sb)
    return;
  g_persistent = false;
  KillTimer(g_sb, 12);
  g_alphaTarget = 0;
  SetTimer(g_sb, 10, 16, nullptr);
}

bool StatusIsPersistent() { return g_persistent; }

bool StatusVisible() { return g_sb && IsWindowVisible(g_sb); }
