#include "core/config.h"
#include "core/json.h"
#include "core/update.h"
#include "engine/engine.h"
#include "lang/lang.h"
#include "ui/gfx.h"
#include "ui/msgbox.h"
#include "ui/statusbar.h"
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include <uxtheme.h>
#include <vector>
#include <windows.h>
#include <windowsx.h>

#ifndef CSIDL_PROGRAMS
#define CSIDL_PROGRAMS 0x0002
#endif
#ifndef CSIDL_DESKTOPDIRECTORY
#define CSIDL_DESKTOPDIRECTORY 0x0010
#endif
#ifndef CSIDL_LOCALAPPDATA
#define CSIDL_LOCALAPPDATA 0x001c
#endif

#define WM_APP_TRAY (WM_APP + 1)
#define WM_APP_UPDATE (WM_APP + 2)
#define WM_APP_PLAYDONE (WM_APP + 3)
#define WM_IN_OK (WM_APP + 20)
#define WM_IN_CANCEL (WM_APP + 21)

enum { HK_ID_REC = 1, HK_ID_PLAY = 2 };

enum {
  IDM_REC = 100,
  IDM_PLAY,
  IDM_OPEN,
  IDM_SAVE,
  IDM_SP025 = 107,
  IDM_SP05 = 108,
  IDM_SP075 = 109,
  IDM_SP1 = 110,
  IDM_SP125 = 111,
  IDM_SP15 = 112,
  IDM_SP2 = 113,
  IDM_SP3 = 114,
  IDM_SP4 = 115,
  IDM_SP5 = 116,
  IDM_SP8 = 117,
  IDM_SP10 = 118,
  IDM_SP50 = 119,
  IDM_SPTURBO = 120,
  IDM_SPMAX = 121,
  IDM_SPCUSTOM = 122,
  IDM_LP1 = 125,
  IDM_LP2,
  IDM_LP3,
  IDM_LP5,
  IDM_LP10,
  IDM_LP20,
  IDM_LP50,
  IDM_LP100,
  IDM_LP500,
  IDM_LP1000,
  IDM_LPCONT,
  IDM_LPCUSTOM,
  IDM_TH0 = 140,
  IDM_TH1,
  IDM_TH2,
  IDM_TRI0 = 143,
  IDM_TRI1,
  IDM_TRI2,
  IDM_UI0 = 146,
  IDM_UI1,
  IDM_UI2,
  IDM_LENG = 150,
  IDM_LENRU,
  IDM_LENIMP,
  IDM_LENTR,
  IDM_HKR0 = 160,
  IDM_HKP0,
  IDM_PANIC_PAUSE = 165,
  IDM_PANIC_SCROLL,
  IDM_PANIC_ESC,
  IDM_PANIC_MOUSE,
  IDM_SB_SHOW = 175,
  IDM_SB_TOP,
  IDM_SB_BOTTOM,
  IDM_SB_FOLLOW,
  IDM_ASSOC_ONTY = 185,
  IDM_TOPMOST = 190,
  IDM_AUTOMINIMIZE = 191,
  IDM_COPY_CLIPBOARD = 192,
  IDM_PASTE_CLIPBOARD = 193,
  IDM_CREATE_SHORTCUT = 194,
  IDM_INSTALL = 195,
  IDM_UNINSTALL = 196,
  IDM_UPDATE = 197,
  IDM_ABOUT,
  IDM_EXIT,
  IDM_RECENT_0 = 1700,
  IDM_RECENT_7 = 1707,
  IDM_RECENT_CLEAR = 1708
};

static const int WIN_W = 250, TB_H = 30, TOOL_H = 45, STATUS_H = 24, TILES = 5,
                 TILE_W = WIN_W / TILES;
static const int WIN_H = TB_H + TOOL_H + STATUS_H;

static const wchar_t GLYPH_OPEN = 0xE8E5, GLYPH_SAVE = 0xE74E, GLYPH_REC = 0,
                     GLYPH_STOP = 0xE71A, GLYPH_PLAY = 0xE768;

static const int CMD_BTN[TILES] = {IDM_OPEN, IDM_SAVE, IDM_REC, IDM_PLAY, 0};
static const wchar_t GLYPH_BTN[TILES] = {GLYPH_OPEN, GLYPH_SAVE, GLYPH_REC,
                                         GLYPH_PLAY, 0xE713};

static int g_hoverB = -1, g_pressB = -1, g_hoverT = 0;
static HWND g_abHwnd = nullptr;
static COLORREF g_menuIcon = RGB(210, 210, 210);
static bool g_trayOn = false;
static int g_trayState = 0;
static NOTIFYICONDATAW g_nid = {};
static HICON g_trayIcon = nullptr;
static UINT g_msgTaskbar = 0;
static bool g_closeAfterPlay = false;

static void DoCmd(int id);
static bool InputDialog(const wchar_t *prompt, int init, int minv, int maxv,
                        int &out);
static void ShowAbout();

static RECT TileRect(int i) {
  RECT r = {i * TILE_W, TB_H, (i + 1) * TILE_W, TB_H + TOOL_H};
  return r;
}

static RECT UpdateBtnRect() {
  RECT r = {WIN_W - 23, TB_H + TOOL_H + 1, WIN_W - 1,
            TB_H + TOOL_H + STATUS_H - 1};
  return r;
}
static RECT ReSaveBtnRect() {
  RECT r = {0, TB_H + TOOL_H + 1, 23, TB_H + TOOL_H + STATUS_H - 1};
  return r;
}
static std::wstring g_lastFile;
static bool g_dirty = false;

static COLORREF RecRed() { return RGB(232, 64, 54); }
static COLORREF PlayBlue() { return RGB(90, 184, 224); }

static Gdiplus::StringFormat &SfL() {
  static Gdiplus::StringFormat f;
  return f;
}

static Gdiplus::StringFormat &SfC() {
  static Gdiplus::StringFormat f;
  f.SetAlignment(Gdiplus::StringAlignmentCenter);
  f.SetLineAlignment(Gdiplus::StringAlignmentCenter);
  return f;
}

static Gdiplus::StringFormat &SfR() {
  static Gdiplus::StringFormat f;
  f.SetAlignment(Gdiplus::StringAlignmentFar);
  f.SetLineAlignment(Gdiplus::StringAlignmentCenter);
  return f;
}

static Gdiplus::StringFormat &SfVC() {
  static Gdiplus::StringFormat f;
  f.SetLineAlignment(Gdiplus::StringAlignmentCenter);
  return f;
}

static void FmtMmSs(wchar_t *b, size_t n, DWORD ms);
static bool InputDialog(const wchar_t *prompt, int init, int minv, int maxv,
                        int &out);
static bool InputDialogFloat(const wchar_t *prompt, double init, double minv,
                             double maxv, double &out);

static std::wstring FileNameOnly(const std::wstring &p) {
  const wchar_t *base = wcsrchr(p.c_str(), L'\\');
  return base ? base + 1 : p;
}

static std::wstring CleanPath(std::wstring p) {
  while (!p.empty() &&
         (p.front() == L' ' || p.front() == L'\"' || p.front() == L'\''))
    p.erase(p.begin());
  while (!p.empty() &&
         (p.back() == L' ' || p.back() == L'\"' || p.back() == L'\''))
    p.pop_back();
  return p;
}

static std::wstring DockStatusText() {
  int st = g_state.load();
  wchar_t b[32];
  if (st == ST_REC) {
    FmtMmSs(b, 32, GetTickCount() - g_startTick);
    return std::wstring(L"REC ") + b;
  }
  if (st == ST_PLAY) {
    FmtMmSs(b, 32, GetTickCount() - g_startTick);
    wchar_t b2[64];
    if (g_cfg.continuous)
      swprintf_s(b2, L"PLAY %s \u221E", b);
    else
      swprintf_s(b2, L"PLAY %s (%d/%d)", b, g_playLoop.load() + 1, g_cfg.loops);
    return b2;
  }
  if (!g_lastFile.empty())
    return FileNameOnly(g_lastFile);
  if (!g_events.empty())
    return L(L_NEWMACRO);
  return L(L_ST_NOTHING);
}

static std::wstring ActiveStopKeysText() {
  std::vector<std::wstring> keys;
  if (g_cfg.panicPause)
    keys.push_back(L"Pause");
  if (g_cfg.panicScroll)
    keys.push_back(L"Scroll");
  if (g_cfg.panicEsc)
    keys.push_back(L"Esc");
  if (keys.empty() && g_cfg.playVk) {
    keys.push_back(FmtHk(g_cfg.playMods, g_cfg.playVk));
  }
  if (keys.empty()) {
    if (g_cfg.panicMouseMove)
      keys.push_back(L(L_PANIC_MOUSE));
    else
      keys.push_back(L(L_STOP));
  }
  std::wstring res;
  for (size_t i = 0; i < keys.size(); i++) {
    if (i > 0)
      res += L" / ";
    res += keys[i];
  }
  return res;
}

static std::wstring EngineStatusText() {
  int st = g_state.load();
  wchar_t b[32];
  if (st == ST_REC) {
    FmtMmSs(b, 32, GetTickCount() - g_startTick);
    std::wstring hk = FmtHk(g_cfg.recordMods, g_cfg.recordVk);
    return std::wstring(L"REC ") + b + L" \u2022 " + L(L_STOP_BIND) + L": " +
           hk;
  }
  if (st == ST_PLAY) {
    FmtMmSs(b, 32, GetTickCount() - g_startTick);
    wchar_t b2[128];
    std::wstring stopKey = ActiveStopKeysText();
    if (g_cfg.continuous)
      swprintf_s(b2, L"PLAY %s \u221E \u2022 %s: %s", b, L(L_STOP_BIND),
                 stopKey.c_str());
    else
      swprintf_s(b2, L"PLAY %s (%d/%d) \u2022 %s: %s", b, g_playLoop.load() + 1,
                 g_cfg.loops, L(L_STOP_BIND), stopKey.c_str());
    return b2;
  }
  if (!g_lastFile.empty())
    return FileNameOnly(g_lastFile);
  if (!g_events.empty())
    return L(L_NEWMACRO);
  return L(L_ST_NOTHING);
}

static void Paint() {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(g_hwnd, &ps);
  RECT rc;
  GetClientRect(g_hwnd, &rc);
  HDC mem = CreateCompatibleDC(hdc);
  HBITMAP bm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
  HGDIOBJ old = SelectObject(mem, bm);
  Gdiplus::Graphics g(mem);
  bool acry = g_cfg.theme == THEME_ACRYLIC;
  g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
  g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
  Gdiplus::SolidBrush bg(acry ? GfxAcrylicBg() : GdiCol(ThBg()));
  g.FillRectangle(&bg, 0, 0, rc.right, rc.bottom);
  Gdiplus::Pen border(acry ? Gdiplus::Color(160, 56, 56, 56)
                           : GdiCol(ThBorder()));
  g.DrawRectangle(&border, 0, 0, rc.right - 1, rc.bottom - 1);
  int st = g_state.load();
  for (int i = 0; i < TILES; i++) {
    int hs = g_pressB == i ? 2 : g_hoverB == i ? 1 : 0;
    if (!hs)
      continue;
    Gdiplus::SolidBrush fill(acry
                                 ? Gdiplus::Color(hs == 2 ? 90 : 70, 60, 60, 60)
                                 : GdiCol(hs == 2 ? ThHover2() : ThHover()));
    RECT r = TileRect(i);
    g.FillRectangle(&fill, (REAL)r.left, (REAL)r.top, (REAL)(r.right - r.left),
                    (REAL)(r.bottom - r.top));
  }
  Gdiplus::SolidBrush gborder(acry ? Gdiplus::Color(105, 165, 165, 165)
                                   : GdiCol(ThBorder()));
  for (int i = 1; i < TILES; i++)
    g.FillRectangle(&gborder, (REAL)(i * TILE_W - 1), (REAL)TB_H, 1.0f,
                    (REAL)TOOL_H);
  int sy = TB_H + TOOL_H;
  g.FillRectangle(&gborder, 0.0f, (REAL)TB_H, (REAL)WIN_W, 1.0f);
  g.FillRectangle(&gborder, 0.0f, (REAL)sy, (REAL)WIN_W, 1.0f);
  g.FillRectangle(&gborder, (REAL)(WIN_W - 24), (REAL)sy, 1.0f, (REAL)STATUS_H);
  g.FillRectangle(&gborder, 23.0f, (REAL)sy, 1.0f, (REAL)STATUS_H);
  std::wstring status = DockStatusText();
  Gdiplus::Font f9(mem, ThFont(8));
  Gdiplus::SolidBrush txt(GdiCol(ThText()));
  Gdiplus::SolidBrush dim(GdiCol(ThDim()));
  g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
  g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
  for (int i = 0; i < TILES; i++) {
    RECT r = TileRect(i);
    wchar_t gl = GLYPH_BTN[i];
    COLORREF col = ThText();
    if (i == 2) {
      if (st == ST_REC)
        gl = GLYPH_STOP;
      else {
        Gdiplus::SolidBrush rd(GdiCol(RecRed()));
        g.FillEllipse(&rd, (REAL)(r.left + r.right) / 2.0f - 6.5f,
                      (REAL)(r.top + r.bottom) / 2.0f - 6.5f, 13.0f, 13.0f);
        continue;
      }
    } else if (i == 3 && st == ST_PLAY)
      gl = GLYPH_STOP;
    DrawGlyph(mem, g, (REAL)(r.left + r.right) / 2.0f,
              (REAL)(r.top + r.bottom) / 2.0f, 16, gl, col);
  }
  RECT rsb = ReSaveBtnRect();
  RECT rbtn = {1, rsb.top, 23, rsb.bottom};
  bool hasRec = !g_events.empty();
  bool saved = hasRec && !g_dirty;
  if (g_hoverT == 6 && hasRec) {
    g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    Gdiplus::SolidBrush hb(acry ? Gdiplus::Color(70, 60, 60, 60)
                                : GdiCol(ThHover()));
    g.FillRectangle(&hb, (REAL)rbtn.left, (REAL)rbtn.top,
                    (REAL)(rbtn.right - rbtn.left),
                    (REAL)(rbtn.bottom - rbtn.top));
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
  }
  if (!hasRec) {
    Gdiplus::SolidBrush dd(GdiCol(ThDim()));
    g.FillEllipse(&dd, (REAL)(rbtn.left + rbtn.right) / 2.0f - 3,
                  (REAL)(rbtn.top + rbtn.bottom) / 2.0f - 3, 6.0f, 6.0f);
  } else if (saved)
    DrawGlyph(mem, g, (REAL)(rbtn.left + rbtn.right) / 2.0f,
              (REAL)(rbtn.top + rbtn.bottom) / 2.0f, 11, (wchar_t)0xE73E,
              ThText());
  else
    DrawGlyph(mem, g, (REAL)(rbtn.left + rbtn.right) / 2.0f,
              (REAL)(rbtn.top + rbtn.bottom) / 2.0f, 11, (wchar_t)0xE7BA,
              RGB(224, 176, 64));

  Gdiplus::RectF sBounds;
  g.MeasureString(status.c_str(), -1, &f9, Gdiplus::PointF(0, 0), &SfVC(),
                  &sBounds);
  REAL maxW = 118.0f;
  REAL textX = 30.0f;
  if (sBounds.Width > maxW) {
    DWORD now = GetTickCount();
    REAL overflow = sBounds.Width - maxW + 16.0f;
    DWORD period = 4000;
    REAL t = (REAL)(now % period) / (REAL)period;
    REAL offset = 0;
    if (t > 0.25f && t < 0.75f) {
      offset = ((t - 0.25f) / 0.5f) * overflow;
    } else if (t >= 0.75f) {
      offset = overflow;
    }
    textX -= offset;
  }
  Gdiplus::GraphicsState gState = g.Save();
  g.SetClip(Gdiplus::RectF(28.0f, (REAL)sy, maxW + 2.0f, (REAL)STATUS_H));
  Gdiplus::SolidBrush ttxt(
      GdiCol(st == ST_IDLE ? ThDim() : (st == ST_REC ? RecRed() : PlayBlue())));
  g.DrawString(status.c_str(), -1, &f9,
               Gdiplus::RectF(textX, (REAL)(sy + 1), sBounds.Width + 4.0f,
                              (REAL)STATUS_H),
               &SfVC(), &ttxt);
  g.Restore(gState);

  g.DrawString(APP_VERSION, -1, &f9,
               Gdiplus::RectF(150, (REAL)(sy + 1), 72, (REAL)STATUS_H), &SfR(),
               &dim);
  RECT ub = UpdateBtnRect();
  if (g_hoverT == 5) {
    g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    Gdiplus::SolidBrush hb(acry ? Gdiplus::Color(70, 60, 60, 60)
                                : GdiCol(ThHover()));
    g.FillRectangle(&hb, (REAL)ub.left, (REAL)ub.top,
                    (REAL)(ub.right - ub.left), (REAL)(ub.bottom - ub.top));
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
  }
  DrawGlyph(mem, g, (REAL)(ub.left + ub.right) / 2.0f,
            (REAL)(ub.top + ub.bottom) / 2.0f, 9, (wchar_t)0xE777,
            Update_Found() ? RecRed() : ThText());
  GfxChrome(g, mem, rc.right, L(L_APPNAME), g_hoverT, CHB_MIN | CHB_PIN);
  BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
  SelectObject(mem, old);
  DeleteObject(bm);
  DeleteDC(mem);
  EndPaint(g_hwnd, &ps);
}

enum {
  TIP_TILE0 = 200,
  TIP_PIN = 210,
  TIP_MIN = 211,
  TIP_CLOSE = 212,
  TIP_ICON = 213,
  TIP_UPDATE = 214,
  TIP_RESAVE = 215
};

static HWND g_tip = nullptr;
static std::wstring g_tipBuf;

static const wchar_t CURSOR_TIP_CLASS[] = L"OntyTaskCursorTip";
static HWND g_cursorTip = nullptr;
static std::wstring g_cursorTipBuf;
static DWORD g_cursorTipHideTick = 0;
static int g_cursorTipW = 0, g_cursorTipH = 0;

static std::wstring TipFor(UINT id) {
  if (id >= TIP_TILE0 && id < TIP_TILE0 + TILES) {
    int t = id - TIP_TILE0;
    int st = g_state.load();
    switch (t) {
    case 0:
      return L(L_OPEN);
    case 1:
      return L(L_SAVE);
    case 2:
      return (st == ST_REC ? std::wstring(L(L_STOP)) : std::wstring(L(L_REC))) +
             L"  (" + FmtHk(g_cfg.recordMods, g_cfg.recordVk) + L")";
    case 3:
      return (st == ST_PLAY ? std::wstring(L(L_STOP)) + L"  (" +
                                  ActiveStopKeysText() + L")"
                            : std::wstring(L(L_PLAY)) + L"  (" +
                                  FmtHk(g_cfg.playMods, g_cfg.playVk) + L")");
    default:
      return L(L_SETTINGS);
    }
  }
  switch (id) {
  case TIP_PIN:
    return L(L_TOPMOST);
  case TIP_MIN:
    return L(L_MIN);
  case TIP_CLOSE:
    return L(L_HIDE);
  case TIP_ICON:
    return std::wstring(L(L_APPNAME)) + L" \u2014 " + L(L_ABOUT);
  case TIP_UPDATE:
    if (Update_State() == UR_FOUND)
      return L(L_UPDATE_AVAIL);
    if (Update_State() == UR_CHECKING)
      return std::wstring(L(L_UPDATE)) + L" \u2026";
    if (Update_State() == UR_LATEST)
      return L(L_ST_UPTODATE);
    if (Update_State() == UR_FAIL)
      return L(L_ST_UPDATEFAIL);
    return L(L_UPDATE);
  case TIP_RESAVE:
    if (g_events.empty())
      return L(L_ST_NOTHING);
    return g_dirty ? std::wstring(L(L_RESAVE)) + L"  (Ctrl+S)"
                   : std::wstring(L(L_SAVED));
  default:
    return std::wstring();
  }
}

static const wchar_t TIP_CLASS[] = L"OntyTaskTip";
static UINT g_tipId = 0;
static bool g_tipShown = false;
static DWORD g_tipDue = 0;
static int g_tipW = 0, g_tipH = 0;

static void TipHideNow() {
  if (!g_tip)
    return;
  g_tipShown = false;
  ShowWindow(g_tip, SW_HIDE);
}

static void TipMeasure(const std::wstring &s) {
  HDC dc = GetDC(g_tip);
  HGDIOBJ o = SelectObject(dc, ThFont(8));
  SIZE sz = {};
  GetTextExtentPoint32W(dc, s.c_str(), (int)s.size(), &sz);
  SelectObject(dc, o);
  ReleaseDC(g_tip, dc);
  g_tipW = sz.cx + 14;
  g_tipH = sz.cy + 6;
}

static void TipSet(UINT id) {
  if (id == g_tipId)
    return;
  g_tipId = id;
  g_tipDue = GetTickCount() + 600;
  if (g_tipShown)
    TipHideNow();
}

static void TipRender(int x, int y) {
  int w = g_tipW, h = g_tipH;
  if (w <= 0 || h <= 0)
    return;
  HDC screen = GetDC(nullptr);
  HDC dc = CreateCompatibleDC(screen);
  BITMAPINFO bi = {};
  bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biWidth = w;
  bi.bmiHeader.biHeight = -h;
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;
  void *bits = nullptr;
  HBITMAP dib =
      CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
  HGDIOBJ ob = SelectObject(dc, dib);
  Gdiplus::Graphics g(dc);
  g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
  g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
  g.Clear(Gdiplus::Color(0, 0, 0, 0));
  bool light = g_cfg.theme == THEME_LIGHT;
  Gdiplus::RectF rr(0.5f, 0.5f, (REAL)(w - 1), (REAL)(h - 1));
  REAL d = 8.0f;
  Gdiplus::GraphicsPath path;
  path.AddArc(rr.X, rr.Y, d, d, 180, 90);
  path.AddArc(rr.GetRight() - d, rr.Y, d, d, 270, 90);
  path.AddArc(rr.GetRight() - d, rr.GetBottom() - d, d, d, 0, 90);
  path.AddArc(rr.X, rr.GetBottom() - d, d, d, 90, 90);
  path.CloseFigure();
  Gdiplus::SolidBrush bb(light ? Gdiplus::Color(255, 255, 255, 255)
                               : Gdiplus::Color(255, 40, 40, 40));
  g.FillPath(&bb, &path);
  Gdiplus::Pen pen(light ? Gdiplus::Color(255, 210, 210, 210)
                         : Gdiplus::Color(255, 68, 68, 68),
                   1.0f);
  g.DrawPath(&pen, &path);
  Gdiplus::Font f(dc, ThFont(8));
  Gdiplus::SolidBrush tb(light ? Gdiplus::Color(255, 24, 24, 24)
                               : Gdiplus::Color(255, 230, 230, 230));
  static Gdiplus::StringFormat sf(Gdiplus::StringFormat::GenericTypographic());
  sf.SetAlignment(Gdiplus::StringAlignmentCenter);
  sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
  sf.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
  g.DrawString(g_tipBuf.c_str(), -1, &f, Gdiplus::RectF(0, 0, (REAL)w, (REAL)h),
               &sf, &tb);
  unsigned *px = (unsigned *)bits;
  for (int i = 0; i < w * h; i++) {
    unsigned p = px[i];
    unsigned a = (p >> 24) & 0xFF;
    if (a == 255 || a == 0)
      continue;
    unsigned b0 = (p & 0xFF) * a / 255;
    unsigned b1 = ((p >> 8) & 0xFF) * a / 255;
    unsigned b2 = ((p >> 16) & 0xFF) * a / 255;
    px[i] = (p & 0xFF000000u) | (b2 << 16) | (b1 << 8) | b0;
  }
  SIZE sz = {w, h};
  POINT pos = {x, y};
  POINT srco = {0, 0};
  BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
  UpdateLayeredWindow(g_tip, screen, &pos, &sz, dc, &srco, 0, &bf, ULW_ALPHA);
  SelectObject(dc, ob);
  DeleteObject(dib);
  DeleteDC(dc);
  ReleaseDC(nullptr, screen);
}

static void CursorTipRender(int x, int y) {
  int w = g_cursorTipW, h = g_cursorTipH;
  if (w <= 0 || h <= 0 || !g_cursorTip)
    return;
  HDC screen = GetDC(nullptr);
  HDC dc = CreateCompatibleDC(screen);
  BITMAPINFO bi = {};
  bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biWidth = w;
  bi.bmiHeader.biHeight = -h;
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;
  void *bits = nullptr;
  HBITMAP dib =
      CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
  HGDIOBJ ob = SelectObject(dc, dib);
  Gdiplus::Graphics g(dc);
  g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
  g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
  g.Clear(Gdiplus::Color(0, 0, 0, 0));
  bool light = g_cfg.theme == THEME_LIGHT;
  Gdiplus::RectF rr(0.5f, 0.5f, (REAL)(w - 1), (REAL)(h - 1));
  REAL d = 8.0f;
  Gdiplus::GraphicsPath path;
  path.AddArc(rr.X, rr.Y, d, d, 180, 90);
  path.AddArc(rr.GetRight() - d, rr.Y, d, d, 270, 90);
  path.AddArc(rr.GetRight() - d, rr.GetBottom() - d, d, d, 0, 90);
  path.AddArc(rr.X, rr.GetBottom() - d, d, d, 90, 90);
  path.CloseFigure();
  Gdiplus::SolidBrush bb(light ? Gdiplus::Color(255, 255, 255, 255)
                               : Gdiplus::Color(255, 40, 40, 40));
  g.FillPath(&bb, &path);
  Gdiplus::Pen pen(light ? Gdiplus::Color(255, 210, 210, 210)
                         : Gdiplus::Color(255, 68, 68, 68),
                   1.0f);
  g.DrawPath(&pen, &path);
  Gdiplus::Font f(dc, ThFont(8));
  Gdiplus::SolidBrush tb(light ? Gdiplus::Color(255, 24, 24, 24)
                               : Gdiplus::Color(255, 230, 230, 230));
  static Gdiplus::StringFormat sf(Gdiplus::StringFormat::GenericTypographic());
  sf.SetAlignment(Gdiplus::StringAlignmentCenter);
  sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
  sf.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
  g.DrawString(g_cursorTipBuf.c_str(), -1, &f,
               Gdiplus::RectF(0, 0, (REAL)w, (REAL)h), &sf, &tb);
  unsigned *px = (unsigned *)bits;
  for (int i = 0; i < w * h; i++) {
    unsigned p = px[i];
    unsigned a = (p >> 24) & 0xFF;
    if (a == 255 || a == 0)
      continue;
    unsigned b0 = (p & 0xFF) * a / 255;
    unsigned b1 = ((p >> 8) & 0xFF) * a / 255;
    unsigned b2 = ((p >> 16) & 0xFF) * a / 255;
    px[i] = (p & 0xFF000000u) | (b2 << 16) | (b1 << 8) | b0;
  }
  SIZE sz = {w, h};
  POINT pos = {x, y};
  POINT srco = {0, 0};
  BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
  UpdateLayeredWindow(g_cursorTip, screen, &pos, &sz, dc, &srco, 0, &bf,
                      ULW_ALPHA);
  SelectObject(dc, ob);
  DeleteObject(dib);
  DeleteDC(dc);
  ReleaseDC(nullptr, screen);
}

static void CursorTipShow(const std::wstring &msg) {
  if (!g_cursorTip)
    return;
  g_cursorTipBuf = msg;
  HDC dc = GetDC(g_cursorTip);
  HGDIOBJ o = SelectObject(dc, ThFont(8));
  SIZE sz = {};
  GetTextExtentPoint32W(dc, msg.c_str(), (int)msg.size(), &sz);
  SelectObject(dc, o);
  ReleaseDC(g_cursorTip, dc);
  g_cursorTipW = sz.cx + 14;
  g_cursorTipH = sz.cy + 6;
  POINT pt = {};
  GetCursorPos(&pt);
  int x = pt.x + 12, y = pt.y + 22;
  HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi = {sizeof(mi)};
  GetMonitorInfoW(mon, &mi);
  if (x + g_cursorTipW > mi.rcWork.right)
    x = pt.x - g_cursorTipW - 8;
  if (y + g_cursorTipH > mi.rcWork.bottom)
    y = pt.y - g_cursorTipH - 8;
  if (x < mi.rcWork.left)
    x = mi.rcWork.left + 4;
  if (y < mi.rcWork.top)
    y = mi.rcWork.top + 4;
  CursorTipRender(x, y);
  SetWindowPos(g_cursorTip, HWND_TOPMOST, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
  g_cursorTipHideTick = GetTickCount() + 1800;
}

static void CursorTipHide() {
  if (g_cursorTip)
    ShowWindow(g_cursorTip, SW_HIDE);
  g_cursorTipHideTick = 0;
}

static void TipPoll() {
  if (g_cursorTipHideTick && GetTickCount() > g_cursorTipHideTick) {
    CursorTipHide();
  }
  if (!g_tip)
    return;
  if (!g_tipId) {
    if (g_tipShown)
      TipHideNow();
    return;
  }
  if (GetTickCount() < g_tipDue)
    return;
  std::wstring s = TipFor(g_tipId);
  if (s.empty()) {
    if (g_tipShown)
      TipHideNow();
    return;
  }
  if (s != g_tipBuf) {
    g_tipBuf = s;
    TipMeasure(s);
  }
  POINT cp;
  GetCursorPos(&cp);
  TipRender(cp.x + 12, cp.y + 22);
  SetWindowPos(g_tip, HWND_TOPMOST, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  if (!g_tipShown) {
    ShowWindow(g_tip, SW_SHOWNOACTIVATE);
    g_tipShown = true;
  }
}

static LRESULT CALLBACK TipProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  return DefWindowProcW(h, m, w, l);
}

static void TipInit(HWND) {
  WNDCLASSW wc = {};
  wc.lpfnWndProc = TipProc;
  wc.hInstance = g_hInst;
  wc.lpszClassName = TIP_CLASS;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  RegisterClassW(&wc);
  g_tip = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE |
                              WS_EX_TRANSPARENT | WS_EX_LAYERED,
                          TIP_CLASS, L"", WS_POPUP, 0, 0, 10, 10, nullptr,
                          nullptr, g_hInst, nullptr);

  WNDCLASSW wc2 = {};
  wc2.lpfnWndProc = TipProc;
  wc2.hInstance = g_hInst;
  wc2.lpszClassName = CURSOR_TIP_CLASS;
  wc2.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  RegisterClassW(&wc2);
  g_cursorTip =
      CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE |
                          WS_EX_TRANSPARENT | WS_EX_LAYERED,
                      CURSOR_TIP_CLASS, L"", WS_POPUP, 0, 0, 10, 10, nullptr,
                      nullptr, g_hInst, nullptr);
}

static void AddMI(HMENU m, UINT id, const wchar_t *txt, wchar_t glyph) {
  AppendMenuW(m, MF_STRING, id, txt);
  GfxMenuGlyph(m, id, glyph, g_menuIcon);
}

static void AddCheck(HMENU m, UINT id, const wchar_t *txt, bool checked) {
  AppendMenuW(m, MF_STRING | (checked ? MF_CHECKED : 0), id, txt);
  if (checked) {
    GfxMenuGlyph(m, id, (wchar_t)0xE73E, g_menuIcon);
  }
}

static void AddCheckWithIcon(HMENU m, UINT id, const wchar_t *txt, bool checked,
                             wchar_t uncheckedGlyph) {
  AppendMenuW(m, MF_STRING | (checked ? MF_CHECKED : 0), id, txt);
  GfxMenuGlyph(m, id, checked ? (wchar_t)0xE73E : uncheckedGlyph, g_menuIcon);
}

static COLORREF MenuIconColor() {
  return GfxMenuTheme() == 2 ? RGB(210, 210, 210) : RGB(60, 60, 60);
}

static void IconLast(HMENU m, wchar_t glyph) {
  MENUITEMINFOW mii = {sizeof(mii)};
  mii.fMask = MIIM_BITMAP;
  mii.hbmpItem = GfxGlyphBitmap(glyph, g_menuIcon);
  SetMenuItemInfoW(m, GetMenuItemCount(m) - 1, TRUE, &mii);
}

static bool CopyMacroToClipboard() {
  std::string s;
  if (!Engine_ToJson(s))
    return false;
  std::wstring ws = json::ToWide(s);
  if (!OpenClipboard(g_hwnd))
    return false;
  EmptyClipboard();
  size_t bytes = (ws.size() + 1) * sizeof(wchar_t);
  HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, bytes);
  if (!hg) {
    CloseClipboard();
    return false;
  }
  memcpy(GlobalLock(hg), ws.c_str(), bytes);
  GlobalUnlock(hg);
  SetClipboardData(CF_UNICODETEXT, hg);
  CloseClipboard();
  return true;
}

static bool PasteMacroFromClipboard() {
  if (!OpenClipboard(g_hwnd))
    return false;
  HANDLE hg = GetClipboardData(CF_UNICODETEXT);
  if (!hg) {
    CloseClipboard();
    return false;
  }
  const wchar_t *wstr = (const wchar_t *)GlobalLock(hg);
  if (!wstr) {
    CloseClipboard();
    return false;
  }
  std::string jsonStr = json::ToUtf8(wstr);
  GlobalUnlock(hg);
  CloseClipboard();
  if (!Engine_FromJson(jsonStr))
    return false;
  g_lastFile.clear();
  g_dirty = true;
  ConfigSave();
  return true;
}

static bool CreateDesktopShortcut(const std::wstring &macroPath) {
  if (macroPath.empty())
    return false;
  CoInitialize(nullptr);
  IShellLinkW *psl = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                IID_IShellLinkW, (void **)&psl);
  if (SUCCEEDED(hr)) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    psl->SetPath(exePath);
    std::wstring args = L"\"" + macroPath + L"\"";
    psl->SetArguments(args.c_str());
    wchar_t dir[MAX_PATH];
    lstrcpynW(dir, exePath, MAX_PATH);
    wchar_t *lastBs = wcsrchr(dir, L'\\');
    if (lastBs)
      *lastBs = 0;
    psl->SetWorkingDirectory(dir);
    psl->SetIconLocation(exePath, 0);

    IPersistFile *ppf = nullptr;
    hr = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
    if (SUCCEEDED(hr)) {
      wchar_t desktop[MAX_PATH];
      SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, desktop);
      std::wstring baseName = FileNameOnly(macroPath);
      size_t dot = baseName.find_last_of(L'.');
      if (dot != std::wstring::npos)
        baseName = baseName.substr(0, dot);
      std::wstring lnkPath =
          std::wstring(desktop) + L"\\" + baseName + L" (OntyTask).lnk";
      hr = ppf->Save(lnkPath.c_str(), TRUE);
      ppf->Release();
    }
    psl->Release();
  }
  CoUninitialize();
  return SUCCEEDED(hr);
}

static const wchar_t UNINSTALL_KEY[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\OntyTask";

static bool GetInstalledPaths(std::wstring &dirOut, std::wstring &exeOut) {
  wchar_t localApp[MAX_PATH];
  if (FAILED(
          SHGetFolderPathW(nullptr, CSIDL_LOCALAPPDATA, nullptr, 0, localApp)))
    return false;
  dirOut = std::wstring(localApp) + L"\\OntyTask";
  exeOut = dirOut + L"\\OntyTask.exe";
  return true;
}

static bool IsAppInstalled() {
  std::wstring dir, exe;
  if (!GetInstalledPaths(dir, exe))
    return false;
  DWORD attr = GetFileAttributesW(exe.c_str());
  return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

static bool InstallToLocalApp() {
  std::wstring targetDir, targetExe;
  if (!GetInstalledPaths(targetDir, targetExe))
    return false;

  CreateDirectoryW(targetDir.c_str(), nullptr);

  wchar_t curExe[MAX_PATH];
  GetModuleFileNameW(nullptr, curExe, MAX_PATH);

  if (_wcsicmp(curExe, targetExe.c_str()) != 0) {
    if (!CopyFileW(curExe, targetExe.c_str(), FALSE))
      return false;
  }

  RegisterFileAssociation(true);

  CoInitialize(nullptr);

  wchar_t programs[MAX_PATH];
  if (SUCCEEDED(
          SHGetFolderPathW(nullptr, CSIDL_PROGRAMS, nullptr, 0, programs))) {
    IShellLinkW *psl1 = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr,
                                   CLSCTX_INPROC_SERVER, IID_IShellLinkW,
                                   (void **)&psl1))) {
      psl1->SetPath(targetExe.c_str());
      psl1->SetWorkingDirectory(targetDir.c_str());
      psl1->SetIconLocation(targetExe.c_str(), 0);
      IPersistFile *ppf1 = nullptr;
      if (SUCCEEDED(psl1->QueryInterface(IID_IPersistFile, (void **)&ppf1))) {
        std::wstring startLnk = std::wstring(programs) + L"\\OntyTask.lnk";
        ppf1->Save(startLnk.c_str(), TRUE);
        ppf1->Release();
      }
      psl1->Release();
    }
  }

  wchar_t desktop[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0,
                                 desktop))) {
    IShellLinkW *psl2 = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr,
                                   CLSCTX_INPROC_SERVER, IID_IShellLinkW,
                                   (void **)&psl2))) {
      psl2->SetPath(targetExe.c_str());
      psl2->SetWorkingDirectory(targetDir.c_str());
      psl2->SetIconLocation(targetExe.c_str(), 0);
      IPersistFile *ppf2 = nullptr;
      if (SUCCEEDED(psl2->QueryInterface(IID_IPersistFile, (void **)&ppf2))) {
        std::wstring deskLnk = std::wstring(desktop) + L"\\OntyTask.lnk";
        ppf2->Save(deskLnk.c_str(), TRUE);
        ppf2->Release();
      }
      psl2->Release();
    }
  }

  CoUninitialize();

  HKEY kUninst = nullptr;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, UNINSTALL_KEY, 0, nullptr, 0,
                      KEY_WRITE, nullptr, &kUninst, nullptr) == ERROR_SUCCESS) {
    const wchar_t name[] = L"OntyTask";
    const wchar_t publisher[] = L"Agzes";
    const wchar_t url[] = L"https://github.com/Agzes/OntyTask";
    std::wstring uninstCmd = L"\"" + targetExe + L"\" --uninstall";
    std::wstring quietUninstCmd = L"\"" + targetExe + L"\" --uninstall";
    std::wstring icon = L"\"" + targetExe + L"\",0";

    RegSetValueExW(kUninst, L"DisplayName", 0, REG_SZ, (const BYTE *)name,
                   (DWORD)((wcslen(name) + 1) * sizeof(wchar_t)));
    RegSetValueExW(kUninst, L"DisplayVersion", 0, REG_SZ,
                   (const BYTE *)APP_VERSION_RAW,
                   (DWORD)((wcslen(APP_VERSION_RAW) + 1) * sizeof(wchar_t)));
    RegSetValueExW(kUninst, L"Publisher", 0, REG_SZ, (const BYTE *)publisher,
                   (DWORD)((wcslen(publisher) + 1) * sizeof(wchar_t)));
    RegSetValueExW(kUninst, L"DisplayIcon", 0, REG_SZ, (const BYTE *)icon.c_str(),
                   (DWORD)((icon.size() + 1) * sizeof(wchar_t)));
    RegSetValueExW(kUninst, L"UninstallString", 0, REG_SZ,
                   (const BYTE *)uninstCmd.c_str(),
                   (DWORD)((uninstCmd.size() + 1) * sizeof(wchar_t)));
    RegSetValueExW(kUninst, L"QuietUninstallString", 0, REG_SZ,
                   (const BYTE *)quietUninstCmd.c_str(),
                   (DWORD)((quietUninstCmd.size() + 1) * sizeof(wchar_t)));
    RegSetValueExW(kUninst, L"InstallLocation", 0, REG_SZ,
                   (const BYTE *)targetDir.c_str(),
                   (DWORD)((targetDir.size() + 1) * sizeof(wchar_t)));
    RegSetValueExW(kUninst, L"URLInfoAbout", 0, REG_SZ, (const BYTE *)url,
                   (DWORD)((wcslen(url) + 1) * sizeof(wchar_t)));
    RegSetValueExW(kUninst, L"HelpLink", 0, REG_SZ, (const BYTE *)url,
                   (DWORD)((wcslen(url) + 1) * sizeof(wchar_t)));
    DWORD noModify = 1;
    RegSetValueExW(kUninst, L"NoModify", 0, REG_DWORD, (const BYTE *)&noModify,
                   sizeof(DWORD));
    DWORD noRepair = 1;
    RegSetValueExW(kUninst, L"NoRepair", 0, REG_DWORD, (const BYTE *)&noRepair,
                   sizeof(DWORD));
    RegCloseKey(kUninst);
  }

  return true;
}

static bool UninstallFromLocalApp() {
  RegisterFileAssociation(false);

  wchar_t programs[MAX_PATH];
  if (SUCCEEDED(
          SHGetFolderPathW(nullptr, CSIDL_PROGRAMS, nullptr, 0, programs))) {
    std::wstring startLnk = std::wstring(programs) + L"\\OntyTask.lnk";
    DeleteFileW(startLnk.c_str());
  }

  wchar_t desktop[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0,
                                 desktop))) {
    std::wstring deskLnk = std::wstring(desktop) + L"\\OntyTask.lnk";
    DeleteFileW(deskLnk.c_str());
  }

  RegDeleteTreeW(HKEY_CURRENT_USER, UNINSTALL_KEY);

  std::wstring targetDir, targetExe;
  if (GetInstalledPaths(targetDir, targetExe)) {
    std::wstring targetIni = targetDir + L"\\OntyTask.ini";
    DeleteFileW(targetIni.c_str());

    wchar_t curExe[MAX_PATH];
    GetModuleFileNameW(nullptr, curExe, MAX_PATH);

    if (_wcsicmp(curExe, targetExe.c_str()) == 0) {
      wchar_t tempPath[MAX_PATH];
      if (GetTempPathW(MAX_PATH, tempPath)) {
        std::wstring batPath = std::wstring(tempPath) + L"ontytask_uninst.bat";
        FILE *f = nullptr;
        if (_wfopen_s(&f, batPath.c_str(), L"w") == 0 && f) {
          fwprintf(f,
                   L"@echo off\n"
                   L":repeat\n"
                   L"timeout /t 1 /nobreak >nul\n"
                   L"del /f /q \"%s\" >nul 2>&1\n"
                   L"if exist \"%s\" goto repeat\n"
                   L"rd \"%s\" >nul 2>&1\n"
                   L"del /f /q \"%%~f0\" >nul 2>&1\n",
                   targetExe.c_str(), targetExe.c_str(), targetDir.c_str());
          fclose(f);
          STARTUPINFOW si = {sizeof(si)};
          si.dwFlags = STARTF_USESHOWWINDOW;
          si.wShowWindow = SW_HIDE;
          PROCESS_INFORMATION pi = {};
          std::wstring cmd = L"cmd.exe /c \"" + batPath + L"\"";
          if (CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE,
                             CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
          }
        }
      }
    } else {
      DeleteFileW(targetExe.c_str());
      RemoveDirectoryW(targetDir.c_str());
    }
  }
  return true;
}

static void AutoSyncInstalledExe() {
  std::wstring targetDir, targetExe;
  if (!GetInstalledPaths(targetDir, targetExe))
    return;
  if (!IsAppInstalled())
    return;
  wchar_t curExe[MAX_PATH];
  GetModuleFileNameW(nullptr, curExe, MAX_PATH);
  if (_wcsicmp(curExe, targetExe.c_str()) != 0) {
    InstallToLocalApp();
  }
}

static void BuildMenu(HMENU root) {
  int st = g_state.load();

  std::wstring recHk = FmtHk(g_cfg.recordMods, g_cfg.recordVk);
  std::wstring playHk = (st == ST_PLAY) ? ActiveStopKeysText()
                                        : FmtHk(g_cfg.playMods, g_cfg.playVk);
  std::wstring recText = std::wstring(st == ST_REC ? L(L_STOP) : L(L_REC)) +
                         (recHk.empty() ? L"" : (L"\t" + recHk));
  std::wstring playText = std::wstring(st == ST_PLAY ? L(L_STOP) : L(L_PLAY)) +
                          (playHk.empty() ? L"" : (L"\t" + playHk));

  AppendMenuW(root, MF_STRING, IDM_REC, recText.c_str());
  GfxMenuIcon(root, IDM_REC,
              st == ST_REC ? GfxGlyphBitmap(GLYPH_STOP, g_menuIcon)
                           : GfxDotBitmap(RecRed()));
  AppendMenuW(root, MF_STRING, IDM_PLAY, playText.c_str());
  GfxMenuGlyph(root, IDM_PLAY, st == ST_PLAY ? GLYPH_STOP : GLYPH_PLAY,
               g_menuIcon);
  AppendMenuW(root, MF_SEPARATOR, 0, nullptr);

  std::wstring openText = std::wstring(L(L_OPEN)) + L"\tCtrl+O";
  std::wstring saveText = std::wstring(L(L_SAVE)) + L"\tCtrl+S";
  AddMI(root, IDM_OPEN, openText.c_str(), 0xE8E5);
  AddMI(root, IDM_SAVE, saveText.c_str(), 0xE74E);

  HMENU recM = CreatePopupMenu();
  if (g_recentFiles.empty()) {
    AppendMenuW(recM, MF_STRING | MF_GRAYED, 0, L(L_NO_RECENT));
  } else {
    for (size_t i = 0; i < g_recentFiles.size(); i++) {
      std::wstring fname = FileNameOnly(g_recentFiles[i]);
      AppendMenuW(recM, MF_STRING, IDM_RECENT_0 + (UINT)i, fname.c_str());
    }
    AppendMenuW(recM, MF_SEPARATOR, 0, nullptr);
    AddMI(recM, IDM_RECENT_CLEAR, L(L_CLEAR_RECENT), 0xE74D);
  }
  AppendMenuW(root, MF_POPUP, (UINT_PTR)recM, L(L_RECENT_FILES));
  IconLast(root, 0xE81C);

  HMENU clipM = CreatePopupMenu();
  std::wstring copyText = std::wstring(L(L_COPY_CLIPBOARD)) + L"\tCtrl+C";
  std::wstring pasteText = std::wstring(L(L_PASTE_CLIPBOARD)) + L"\tCtrl+V";
  AddMI(clipM, IDM_COPY_CLIPBOARD, copyText.c_str(), 0xE8C8);
  AddMI(clipM, IDM_PASTE_CLIPBOARD, pasteText.c_str(), 0xE77F);
  AppendMenuW(clipM, MF_SEPARATOR, 0, nullptr);
  if (!g_lastFile.empty())
    AddMI(clipM, IDM_CREATE_SHORTCUT, L(L_CREATE_SHORTCUT), 0xE71B);
  else
    AppendMenuW(clipM, MF_STRING | MF_GRAYED, IDM_CREATE_SHORTCUT,
                L(L_CREATE_SHORTCUT));
  AppendMenuW(root, MF_POPUP, (UINT_PTR)clipM, L(L_CLIPBOARD));
  IconLast(root, 0xE77F);
  AppendMenuW(root, MF_SEPARATOR, 0, nullptr);

  HMENU sp = CreatePopupMenu();
  AddCheck(sp, IDM_SP05, L(L_SPEED05), g_cfg.speed == 0);
  AddCheck(sp, IDM_SP1, L(L_SPEED1), g_cfg.speed == 1);
  AddCheck(sp, IDM_SP2, L(L_SPEED2), g_cfg.speed == 2);
  AddCheck(sp, IDM_SP5, L(L_SPEED5), g_cfg.speed == 5);
  AddCheck(sp, IDM_SP10, L(L_SPEED10), g_cfg.speed == 10);
  AddCheck(sp, IDM_SPTURBO, L(L_SPEEDTURBO), g_cfg.speed == 100);
  AddCheck(sp, IDM_SPMAX, L(L_SPEED_MAX), g_cfg.speed == -5);
  wchar_t spCustStr[64];
  swprintf_s(spCustStr, L"%s (%.4gx)...", L(L_SPEEDCUSTOM), g_cfg.speedCustom);
  AddCheck(sp, IDM_SPCUSTOM, spCustStr, g_cfg.speed == 999);
  AppendMenuW(root, MF_POPUP, (UINT_PTR)sp, L(L_SPEED));
  IconLast(root, 0xE916);

  HMENU lp = CreatePopupMenu();
  bool customLoop = !g_cfg.continuous && g_cfg.loops != 1 && g_cfg.loops != 2 &&
                    g_cfg.loops != 3 && g_cfg.loops != 5 && g_cfg.loops != 10 &&
                    g_cfg.loops != 100;
  AddCheck(lp, IDM_LP1, L(L_LOOPS1), !g_cfg.continuous && g_cfg.loops == 1);
  AddCheck(lp, IDM_LP2, L(L_LOOPS2), !g_cfg.continuous && g_cfg.loops == 2);
  AddCheck(lp, IDM_LP3, L(L_LOOPS3), !g_cfg.continuous && g_cfg.loops == 3);
  AddCheck(lp, IDM_LP5, L(L_LOOPS5), !g_cfg.continuous && g_cfg.loops == 5);
  AddCheck(lp, IDM_LP10, L(L_LOOPS10), !g_cfg.continuous && g_cfg.loops == 10);
  AddCheck(lp, IDM_LP100, L(L_LOOPS100),
           !g_cfg.continuous && g_cfg.loops == 100);
  AddCheck(lp, IDM_LPCONT, L(L_CONT), g_cfg.continuous != 0);
  wchar_t lpCustStr[64];
  swprintf_s(lpCustStr, L"%s (%d)...", L(L_LOOPSCUSTOM), g_cfg.loops);
  AddCheck(lp, IDM_LPCUSTOM, lpCustStr, customLoop);
  AppendMenuW(root, MF_POPUP, (UINT_PTR)lp, L(L_LOOPS));
  IconLast(root, 0xE74D);
  AppendMenuW(root, MF_SEPARATOR, 0, nullptr);

  AddCheckWithIcon(root, IDM_TOPMOST, L(L_TOPMOST), g_cfg.topmost != 0, 0xE718);

  HMENU hk = CreatePopupMenu();
  std::wstring recItem = std::wstring(L(L_HK_REC)) + L":   " +
                         FmtHk(g_cfg.recordMods, g_cfg.recordVk);
  std::wstring playItem = std::wstring(L(L_HK_PLAY)) + L":   " +
                          FmtHk(g_cfg.playMods, g_cfg.playVk);
  AddMI(hk, IDM_HKR0, recItem.c_str(), 0xE70F);
  AddMI(hk, IDM_HKP0, playItem.c_str(), 0xE70F);
  AppendMenuW(hk, MF_SEPARATOR, 0, nullptr);
  HMENU pnk = CreatePopupMenu();
  AddCheck(pnk, IDM_PANIC_PAUSE, L(L_PANIC_PAUSE), g_cfg.panicPause != 0);
  AddCheck(pnk, IDM_PANIC_SCROLL, L(L_PANIC_SCROLL), g_cfg.panicScroll != 0);
  AddCheck(pnk, IDM_PANIC_ESC, L(L_PANIC_ESC), g_cfg.panicEsc != 0);
  AddCheck(pnk, IDM_PANIC_MOUSE, L(L_PANIC_MOUSE), g_cfg.panicMouseMove != 0);
  AppendMenuW(hk, MF_POPUP, (UINT_PTR)pnk, L(L_PANIC_KEYS));
  IconLast(hk, 0xE71A);
  AppendMenuW(root, MF_POPUP, (UINT_PTR)hk, L(L_HOTKEYS));
  IconLast(root, 0xE765);

  HMENU th = CreatePopupMenu();
  AppendMenuW(th, MF_STRING | MF_GRAYED, 0, L(L_INTERFACE_THEME));
  AddCheck(th, IDM_TH0, L(L_TH_DARK), g_cfg.theme == 0);
  AddCheck(th, IDM_TH1, L(L_TH_LIGHT), g_cfg.theme == 1);
  AddCheck(th, IDM_TH2, L(L_TH_ACRYLIC), g_cfg.theme == 2);
  AppendMenuW(th, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(th, MF_STRING | MF_GRAYED, 0, L(L_TRAY_ICON));
  AddCheck(th, IDM_TRI0, L(L_TRAY_AUTO), g_cfg.trayIcon == 0);
  AddCheck(th, IDM_TRI1, L(L_TH_DARK), g_cfg.trayIcon == 1);
  AddCheck(th, IDM_TRI2, L(L_TH_LIGHT), g_cfg.trayIcon == 2);
  AppendMenuW(th, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(th, MF_STRING | MF_GRAYED, 0, L(L_UI_ICON));
  AddCheck(th, IDM_UI0, L(L_TRAY_AUTO), g_cfg.uiIcon == 0);
  AddCheck(th, IDM_UI1, L(L_TH_DARK), g_cfg.uiIcon == 1);
  AddCheck(th, IDM_UI2, L(L_TH_LIGHT), g_cfg.uiIcon == 2);
  AppendMenuW(root, MF_POPUP, (UINT_PTR)th, L(L_THEME));
  IconLast(root, 0xE727);

  HMENU sb = CreatePopupMenu();
  AddCheck(sb, IDM_SB_SHOW, L(L_SB_ENABLE), g_cfg.statusbarShow != 0);
  AppendMenuW(sb, MF_SEPARATOR, 0, nullptr);
  AddCheck(sb, IDM_SB_TOP, L(L_SB_TOP), g_cfg.statusbarTop != 0);
  AddCheck(sb, IDM_SB_BOTTOM, L(L_SB_BOTTOM), g_cfg.statusbarTop == 0);
  AppendMenuW(sb, MF_SEPARATOR, 0, nullptr);
  AddCheck(sb, IDM_SB_FOLLOW, L(L_SB_FOLLOW_MONITOR),
           g_cfg.statusbarFollowCursor != 0);
  AppendMenuW(root, MF_POPUP, (UINT_PTR)sb, L(L_STATUSBAR));
  IconLast(root, 0xEC42);

  AddCheckWithIcon(root, IDM_AUTOMINIMIZE, L(L_AUTOMINIMIZE),
                   g_cfg.autoMinimize != 0, 0xE921);
  bool isAssoc = IsFileAssociationRegistered();
  AddCheckWithIcon(root, IDM_ASSOC_ONTY, L(L_ASSOC_ONTY), isAssoc, 0xE71B);
  if (IsAppInstalled())
    AddMI(root, IDM_UNINSTALL, L(L_UNINSTALL), 0xE74D);
  else
    AddMI(root, IDM_INSTALL, L(L_INSTALL), 0xE896);

  HMENU lg = CreatePopupMenu();
  AddCheck(lg, IDM_LENG, L(L_LANG_EN), g_cfg.lang == 1);
  AddCheck(lg, IDM_LENRU, L(L_LANG_RU), g_cfg.lang == 2);
  AppendMenuW(lg, MF_SEPARATOR, 0, nullptr);
  AddMI(lg, IDM_LENIMP, L(L_LANG_IMPORT), 0xE896);
  AddMI(lg, IDM_LENTR, L(L_LANG_TRANSLATE), 0xE8A1);
  AppendMenuW(root, MF_POPUP, (UINT_PTR)lg, L(L_LANG));
  IconLast(root, 0xE774);
  AppendMenuW(root, MF_SEPARATOR, 0, nullptr);

  if (Update_Found()) {
    AddMI(root, IDM_UPDATE, L(L_UPDATE_AVAIL), 0xE777);
  }
  AddMI(root, IDM_ABOUT, L(L_ABOUT), 0xE946);
  AddMI(root, IDM_EXIT, L(L_EXIT), 0xE711);
}

static void TrackMenu() {
  GfxMenuTheme();
  g_menuIcon = MenuIconColor();
  HMENU m = CreatePopupMenu();
  BuildMenu(m);
  POINT p;
  GetCursorPos(&p);
  SetForegroundWindow(g_hwnd);
  TrackPopupMenu(m, TPM_RIGHTBUTTON, p.x, p.y, 0, g_hwnd, nullptr);
  DestroyMenu(m);
}

static HICON TrayIconMake() {
  bool light = g_cfg.theme == THEME_LIGHT;
  if (g_cfg.trayIcon == 1)
    light = false;
  else if (g_cfg.trayIcon == 2)
    light = true;
  int id = g_trayState == ST_REC    ? (light ? 5 : 3)
           : g_trayState == ST_PLAY ? (light ? 7 : 6)
                                    : (light ? 4 : 2);
  return LoadAppIconId(id, GetSystemMetrics(SM_CXSMICON));
}

static void TrayAdd() {
  g_nid = {};
  g_nid.cbSize = sizeof(g_nid);
  g_nid.hWnd = g_hwnd;
  g_nid.uID = 1;
  g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
  g_nid.uCallbackMessage = WM_APP_TRAY;
  lstrcpynW(g_nid.szTip, L"OntyTask", 64);
  if (g_trayIcon)
    DestroyIcon(g_trayIcon);
  g_trayIcon = TrayIconMake();
  g_nid.hIcon = g_trayIcon;
  g_trayOn = Shell_NotifyIconW(NIM_ADD, &g_nid) != FALSE;
}

static void TrayRemove() {
  if (g_trayOn)
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
  g_trayOn = false;
}

static void TrayIconRefresh() {
  if (g_trayIcon)
    DestroyIcon(g_trayIcon);
  g_trayIcon = TrayIconMake();
  g_nid.hIcon = g_trayIcon;
  if (g_trayOn)
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

static void TraySync() {
  int st = g_state.load();
  if (st != g_trayState) {
    g_trayState = st;
    TrayIconRefresh();
  }
}

static void TrayBalloon(const wchar_t *title, const wchar_t *text) {
  if (!g_trayOn)
    return;
  UINT f = g_nid.uFlags;
  g_nid.uFlags = NIF_INFO;
  lstrcpynW(g_nid.szInfoTitle, title, 64);
  lstrcpynW(g_nid.szInfo, text, 256);
  g_nid.dwInfoFlags = NIIF_INFO;
  Shell_NotifyIconW(NIM_MODIFY, &g_nid);
  g_nid.uFlags = f;
}

static void FmtMmSs(wchar_t *b, size_t n, DWORD ms) {
  swprintf_s(b, n, L"%02d:%02d", (int)(ms / 60000), (int)(ms / 1000 % 60));
}

static void StatusTick() {
  std::wstring s = EngineStatusText();
  int st = g_state.load();
  if (st == ST_REC || st == ST_PLAY) {
    if (StatusIsPersistent())
      StatusUpdate(s.c_str());
    else if (!StatusVisible())
      StatusPersistent(s.c_str());
    InvalidateRect(g_hwnd, nullptr, FALSE);
  } else {
    if (StatusIsPersistent() && StatusVisible())
      StatusHide();
    InvalidateRect(g_hwnd, nullptr, FALSE);
  }
  int trayState = st;
  if (trayState != g_trayState) {
    g_trayState = trayState;
    TrayIconRefresh();
  }
}

static UINT HkSysMod(int m) {
  return ((m & HK_CTRL) ? MOD_CONTROL : 0) | ((m & HK_SHIFT) ? MOD_SHIFT : 0) |
         ((m & HK_ALT) ? MOD_ALT : 0) | ((m & HK_WIN) ? MOD_WIN : 0) |
         MOD_NOREPEAT;
}

static void HotkeyRegisterAll(HWND hwnd) {
  UnregisterHotKey(hwnd, HK_ID_REC);
  UnregisterHotKey(hwnd, HK_ID_PLAY);
  if (g_cfg.recordVk) {
    UINT mod = HkSysMod(g_cfg.recordMods);
    if (!RegisterHotKey(hwnd, HK_ID_REC, mod, g_cfg.recordVk)) {
      RegisterHotKey(hwnd, HK_ID_REC, mod & ~MOD_NOREPEAT, g_cfg.recordVk);
    }
  }
  if (g_cfg.playVk) {
    UINT mod = HkSysMod(g_cfg.playMods);
    if (!RegisterHotKey(hwnd, HK_ID_PLAY, mod, g_cfg.playVk)) {
      RegisterHotKey(hwnd, HK_ID_PLAY, mod & ~MOD_NOREPEAT, g_cfg.playVk);
    }
  }
}

static void DoCmd(int id) {
  switch (id) {
  case IDM_REC:
    if (g_state.load() == ST_REC) {
      Engine_RecordStop();
      g_lastFile.clear();
      g_dirty = !g_events.empty();
      if (g_cfg.autoMinimize) {
        ShowWindow(g_hwnd, SW_SHOW);
        SetForegroundWindow(g_hwnd);
      }
    } else if (g_state.load() == ST_PLAY)
      Engine_PlayStop();
    else {
      if (Engine_Record()) {
        g_lastFile.clear();
        g_dirty = true;
        if (g_cfg.autoMinimize)
          ShowWindow(g_hwnd, SW_HIDE);
      }
    }
    break;
  case IDM_PLAY:
    if (g_state.load() == ST_PLAY)
      Engine_PlayStop();
    else if (g_state.load() == ST_REC)
      Engine_RecordStop();
    else {
      if (Engine_Play()) {
        if (g_cfg.autoMinimize)
          ShowWindow(g_hwnd, SW_HIDE);
      }
    }
    break;
  case IDM_OPEN: {
    if (g_state.load() != ST_IDLE)
      break;
    wchar_t path[MAX_PATH] = L"";
    OPENFILENAMEW o = {};
    o.lStructSize = sizeof(o);
    o.hwndOwner = g_hwnd;
    o.lpstrFilter =
        L"OntyTask (*.onty;*.json)\0*.onty;*.json\0All files\0*.*\0";
    o.lpstrFile = path;
    o.nMaxFile = MAX_PATH;
    o.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&o)) {
      if (Engine_Load(path)) {
        g_lastFile = path;
        g_dirty = false;
        RecentAdd(path);
        StatusShow(L(L_ST_LOADED), 2500);
      } else {
        StatusShow(L(L_ST_INVALID), 2500);
        UiMessage(g_hwnd, L(L_ST_INVALID), L(L_APPNAME), UI_OK, UI_ICON_ERROR);
      }
    }
    break;
  }
  case IDM_SAVE: {
    if (g_state.load() != ST_IDLE)
      break;
    if (g_events.empty()) {
      StatusShow(L(L_ST_NOTHING), 2500);
      break;
    }
    wchar_t path[MAX_PATH];
    lstrcpynW(path, L"macro.onty", MAX_PATH);
    OPENFILENAMEW o = {};
    o.lStructSize = sizeof(o);
    o.hwndOwner = g_hwnd;
    o.lpstrFilter =
        L"OntyTask (*.onty)\0*.onty\0JSON (*.json)\0*.json\0All files\0*.*\0";
    o.lpstrFile = path;
    o.nMaxFile = MAX_PATH;
    o.lpstrDefExt = L"onty";
    o.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    if (GetSaveFileNameW(&o)) {
      if (Engine_Save(path)) {
        g_lastFile = path;
        g_dirty = false;
        RecentAdd(path);
        StatusShow(L(L_ST_SAVED), 2500);
      } else
        StatusShow(L(L_SAVEFAIL), 3000);
    }
    break;
  }
  case IDM_COPY_CLIPBOARD:
    if (CopyMacroToClipboard())
      StatusShow(L(L_CLIPBOARD_COPIED), 2500);
    else
      StatusShow(L(L_ST_NOTHING), 2500);
    break;
  case IDM_PASTE_CLIPBOARD:
    if (PasteMacroFromClipboard())
      StatusShow(L(L_CLIPBOARD_LOADED), 2500);
    else
      StatusShow(L(L_CLIPBOARD_EMPTY), 2500);
    break;
  case IDM_CREATE_SHORTCUT:
    if (!g_lastFile.empty()) {
      if (CreateDesktopShortcut(g_lastFile))
        StatusShow(L(L_SHORTCUT_SUCCESS), 2500);
      else
        StatusShow(L(L_SHORTCUT_FAIL), 2500);
    } else {
      StatusShow(L(L_ST_NOTHING), 2500);
    }
    break;
  case IDM_RECENT_CLEAR:
    RecentClear();
    break;
  case IDM_SP025:
    g_cfg.speed = -1;
    ConfigSave();
    break;
  case IDM_SP05:
    g_cfg.speed = 0;
    ConfigSave();
    break;
  case IDM_SP075:
    g_cfg.speed = -2;
    ConfigSave();
    break;
  case IDM_SP1:
    g_cfg.speed = 1;
    ConfigSave();
    break;
  case IDM_SP125:
    g_cfg.speed = -3;
    ConfigSave();
    break;
  case IDM_SP15:
    g_cfg.speed = -4;
    ConfigSave();
    break;
  case IDM_SP2:
    g_cfg.speed = 2;
    ConfigSave();
    break;
  case IDM_SP3:
    g_cfg.speed = 3;
    ConfigSave();
    break;
  case IDM_SP4:
    g_cfg.speed = 4;
    ConfigSave();
    break;
  case IDM_SP5:
    g_cfg.speed = 5;
    ConfigSave();
    break;
  case IDM_SP8:
    g_cfg.speed = 8;
    ConfigSave();
    break;
  case IDM_SP10:
    g_cfg.speed = 10;
    ConfigSave();
    break;
  case IDM_SP50:
    g_cfg.speed = 50;
    ConfigSave();
    break;
  case IDM_SPTURBO:
    g_cfg.speed = 100;
    ConfigSave();
    break;
  case IDM_SPMAX:
    g_cfg.speed = -5;
    ConfigSave();
    break;
  case IDM_SPCUSTOM: {
    double v = g_cfg.speedCustom;
    if (InputDialogFloat(L(L_Q_SPEED), g_cfg.speedCustom, 0.01, 1000.0, v)) {
      g_cfg.speedCustom = v;
      g_cfg.speed = 999;
      ConfigSave();
    }
    break;
  }
  case IDM_LP1:
    g_cfg.loops = 1;
    g_cfg.continuous = 0;
    ConfigSave();
    break;
  case IDM_LP2:
    g_cfg.loops = 2;
    g_cfg.continuous = 0;
    ConfigSave();
    break;
  case IDM_LP3:
    g_cfg.loops = 3;
    g_cfg.continuous = 0;
    ConfigSave();
    break;
  case IDM_LP5:
    g_cfg.loops = 5;
    g_cfg.continuous = 0;
    ConfigSave();
    break;
  case IDM_LP10:
    g_cfg.loops = 10;
    g_cfg.continuous = 0;
    ConfigSave();
    break;
  case IDM_LP20:
    g_cfg.loops = 20;
    g_cfg.continuous = 0;
    ConfigSave();
    break;
  case IDM_LP50:
    g_cfg.loops = 50;
    g_cfg.continuous = 0;
    ConfigSave();
    break;
  case IDM_LP100:
    g_cfg.loops = 100;
    g_cfg.continuous = 0;
    ConfigSave();
    break;
  case IDM_LP500:
    g_cfg.loops = 500;
    g_cfg.continuous = 0;
    ConfigSave();
    break;
  case IDM_LP1000:
    g_cfg.loops = 1000;
    g_cfg.continuous = 0;
    ConfigSave();
    break;
  case IDM_LPCONT:
    g_cfg.continuous = !g_cfg.continuous;
    ConfigSave();
    break;
  case IDM_LPCUSTOM: {
    int v = g_cfg.loops;
    if (InputDialog(L(L_Q_LOOPS), g_cfg.loops, 1, 99999, v)) {
      g_cfg.loops = v;
      g_cfg.continuous = 0;
      ConfigSave();
    }
    break;
  }
  case IDM_PANIC_PAUSE:
    g_cfg.panicPause = !g_cfg.panicPause;
    ConfigSave();
    break;
  case IDM_PANIC_SCROLL:
    g_cfg.panicScroll = !g_cfg.panicScroll;
    ConfigSave();
    break;
  case IDM_PANIC_ESC:
    g_cfg.panicEsc = !g_cfg.panicEsc;
    ConfigSave();
    break;
  case IDM_PANIC_MOUSE:
    g_cfg.panicMouseMove = !g_cfg.panicMouseMove;
    ConfigSave();
    break;
  case IDM_SB_SHOW:
    g_cfg.statusbarShow = !g_cfg.statusbarShow;
    if (!g_cfg.statusbarShow)
      StatusHide();
    ConfigSave();
    break;
  case IDM_SB_TOP:
    g_cfg.statusbarTop = 1;
    ConfigSave();
    break;
  case IDM_SB_BOTTOM:
    g_cfg.statusbarTop = 0;
    ConfigSave();
    break;
  case IDM_SB_FOLLOW:
    g_cfg.statusbarFollowCursor = !g_cfg.statusbarFollowCursor;
    ConfigSave();
    break;
  case IDM_ASSOC_ONTY: {
    bool reg = IsFileAssociationRegistered();
    RegisterFileAssociation(!reg);
    StatusShow(!reg ? L(L_ASSOC_SUCCESS) : L(L_ASSOC_REMOVED), 2500);
    break;
  }
  case IDM_TH0:
  case IDM_TH1:
  case IDM_TH2:
    g_cfg.theme = id - IDM_TH0;
    ApplyWindowMaterial(g_hwnd);
    if (IsWindow(g_abHwnd)) {
      ApplyWindowMaterial(g_abHwnd);
      InvalidateRect(g_abHwnd, nullptr, TRUE);
    }
    SendMessageW(g_hwnd, WM_SETICON, ICON_SMALL,
                 (LPARAM)LoadAppIconId(GfxUiIconId(), 16));
    SendMessageW(g_hwnd, WM_SETICON, ICON_BIG,
                 (LPARAM)LoadAppIconId(GfxUiIconId(), 32));
    TrayIconRefresh();
    ConfigSave();
    break;
  case IDM_TRI0:
  case IDM_TRI1:
  case IDM_TRI2:
    g_cfg.trayIcon = id - IDM_TRI0;
    ConfigSave();
    TrayIconRefresh();
    break;
  case IDM_UI0:
  case IDM_UI1:
  case IDM_UI2:
    g_cfg.uiIcon = id - IDM_UI0;
    ConfigSave();
    SendMessageW(g_hwnd, WM_SETICON, ICON_SMALL,
                 (LPARAM)LoadAppIconId(GfxUiIconId(), 16));
    SendMessageW(g_hwnd, WM_SETICON, ICON_BIG,
                 (LPARAM)LoadAppIconId(GfxUiIconId(), 32));
    if (IsWindow(g_abHwnd))
      InvalidateRect(g_abHwnd, nullptr, FALSE);
    break;
  case IDM_LENG:
    g_cfg.lang = 1;
    LangInit();
    ConfigSave();
    break;
  case IDM_LENRU:
    g_cfg.lang = 2;
    LangInit();
    ConfigSave();
    break;
  case IDM_LENIMP: {
    if (g_state.load() != ST_IDLE)
      break;
    wchar_t path[MAX_PATH] = L"";
    OPENFILENAMEW o = {};
    o.lStructSize = sizeof(o);
    o.hwndOwner = g_hwnd;
    o.lpstrFilter = L"JSON (*.json)\0*.json\0All files\0*.*\0";
    o.lpstrFile = path;
    o.nMaxFile = MAX_PATH;
    o.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&o)) {
      std::wstring name;
      if (LangImport(path, name)) {
        g_cfg.lang = 0;
        ConfigSave();
        StatusShow(L(L_ST_TRANSLATED), 2500);
        InvalidateRect(g_hwnd, nullptr, TRUE);
        if (IsWindow(g_abHwnd))
          InvalidateRect(g_abHwnd, nullptr, TRUE);
      } else
        StatusShow(L(L_ST_INVALID), 3000);
    }
    break;
  }
  case IDM_LENTR:
    ShellExecuteW(g_hwnd, L"open", URL_ISSUE, nullptr, nullptr, SW_SHOWNORMAL);
    break;
  case IDM_HKR0:
  case IDM_HKP0: {
    bool play = id == IDM_HKP0;
    int mo = 0, vk = 0;
    if (UiHotkeyCapture(g_hwnd, play, mo, vk)) {
      int &cm = play ? g_cfg.playMods : g_cfg.recordMods;
      int &cv = play ? g_cfg.playVk : g_cfg.recordVk;
      int om = play ? g_cfg.recordMods : g_cfg.playMods;
      int ov = play ? g_cfg.recordVk : g_cfg.playVk;
      if (mo == om && vk == ov) {
        UiMessage(g_hwnd, L(L_ST_CONFLICT), L(L_APPNAME), UI_OK, UI_ICON_ERROR);
        break;
      }
      cm = mo;
      cv = vk;
      ConfigSave();
      HotkeyRegisterAll(g_hwnd);
    }
    break;
  }
  case IDM_AUTOMINIMIZE:
    g_cfg.autoMinimize = !g_cfg.autoMinimize;
    ConfigSave();
    break;
  case IDM_TOPMOST:
    g_cfg.topmost = !g_cfg.topmost;
    SetWindowPos(g_hwnd, g_cfg.topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0,
                 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    ConfigSave();
    break;
  case IDM_INSTALL:
    if (InstallToLocalApp())
      StatusShow(L(L_INSTALL_SUCCESS), 3000);
    else
      StatusShow(L(L_INSTALL_FAIL), 3000);
    break;
  case IDM_UNINSTALL: {
    int res = UiMessage(g_hwnd, L(L_UNINSTALL_CONFIRM), L(L_APPNAME),
                        UI_YESNO, UI_ICON_WARN);
    if (res == IDYES) {
      if (UninstallFromLocalApp()) {
        StatusShow(L(L_UNINSTALL_SUCCESS), 3000);
      } else {
        StatusShow(L(L_UNINSTALL_FAIL), 3000);
      }
    }
    break;
  }
  case IDM_UPDATE:
    if (Update_Found())
      ShellExecuteW(g_hwnd, L"open", URL_RELEASES, nullptr, nullptr,
                    SW_SHOWNORMAL);
    else
      Update_ManualCheck();
    break;
  case IDM_ABOUT:
    ShowAbout();
    break;
  case IDM_EXIT:
    ConfigSave();
    DestroyWindow(g_hwnd);
    break;
  default:
    if (id >= IDM_RECENT_0 && id < IDM_RECENT_0 + 8) {
      size_t idx = id - IDM_RECENT_0;
      if (idx < g_recentFiles.size()) {
        std::wstring p = g_recentFiles[idx];
        if (Engine_Load(p.c_str())) {
          g_lastFile = p;
          g_dirty = false;
          RecentAdd(p);
          StatusShow(L(L_ST_LOADED), 2500);
        } else {
          StatusShow(L(L_ST_INVALID), 2500);
          UiMessage(g_hwnd, L(L_ST_INVALID), L(L_APPNAME), UI_OK,
                    UI_ICON_ERROR);
        }
      }
    }
    break;
  }
  TraySync();
  InvalidateRect(g_hwnd, nullptr, FALSE);
}

static LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
  switch (msg) {
  case WM_CREATE:
    DragAcceptFiles(hwnd, TRUE);
    ApplyWindowMaterial(hwnd);
    TryRoundCorners(hwnd);
    SetTimer(hwnd, 1, 200, nullptr);
    SetTimer(hwnd, 3, 120, nullptr);
    SetTimer(hwnd, 14, 10, nullptr);
    HotkeyRegisterAll(hwnd);
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL,
                 (LPARAM)LoadAppIconId(GfxUiIconId(), 16));
    SendMessageW(hwnd, WM_SETICON, ICON_BIG,
                 (LPARAM)LoadAppIconId(GfxUiIconId(), 32));
    TipInit(hwnd);
    if (g_cfg.topmost)
      SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    return 0;
  case WM_PAINT:
    Paint();
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_NCHITTEST: {
    POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
    ScreenToClient(hwnd, &pt);
    RECT rmin = GfxMinRect(WIN_W), rclose = GfxCloseRect(WIN_W),
         rpin = GfxPinRect(WIN_W);
    RECT ric = {0, 0, 32, TB_H};
    if (PtInRect(&rmin, pt) || PtInRect(&rclose, pt) || PtInRect(&rpin, pt) ||
        PtInRect(&ric, pt))
      return HTCLIENT;
    if (pt.y < TB_H)
      return HTCAPTION;
    return HTCLIENT;
  }
  case WM_MOUSEMOVE: {
    POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
    int nb = -1, nt = 0;
    for (int i = 0; i < TILES; i++) {
      RECT r = TileRect(i);
      if (PtInRect(&r, pt))
        nb = i;
    }
    RECT rmin = GfxMinRect(WIN_W), rclose = GfxCloseRect(WIN_W),
         rpin = GfxPinRect(WIN_W);
    RECT ric = {0, 0, 30, TB_H}, rub = UpdateBtnRect(), rsb = ReSaveBtnRect();
    if (PtInRect(&rmin, pt))
      nt = 1;
    if (PtInRect(&rclose, pt))
      nt = 2;
    if (PtInRect(&rpin, pt))
      nt = 3;
    if (PtInRect(&ric, pt))
      nt = 4;
    if (PtInRect(&rub, pt))
      nt = 5;
    if (PtInRect(&rsb, pt) && g_state.load() == ST_IDLE && !g_events.empty())
      nt = 6;
    UINT tid = 0;
    switch (nt) {
    case 1:
      tid = TIP_MIN;
      break;
    case 2:
      tid = TIP_CLOSE;
      break;
    case 3:
      tid = TIP_PIN;
      break;
    case 4:
      tid = TIP_ICON;
      break;
    case 5:
      tid = TIP_UPDATE;
      break;
    case 6:
      tid = TIP_RESAVE;
      break;
    default:
      if (nb >= 0)
        tid = TIP_TILE0 + nb;
      break;
    }
    TipSet(tid);
    if (nb != g_hoverB || nt != g_hoverT) {
      g_hoverB = nb;
      g_hoverT = nt;
      InvalidateRect(hwnd, nullptr, FALSE);
    }
    SetCursor(nt >= 4 ? LoadCursorW(nullptr, IDC_HAND)
                      : LoadCursorW(nullptr, IDC_ARROW));
    TRACKMOUSEEVENT t = {sizeof(t), TME_LEAVE, hwnd, 0};
    TrackMouseEvent(&t);
    return 0;
  }
  case WM_MOUSELEAVE:
    g_hoverB = -1;
    g_hoverT = 0;
    TipSet(0);
    InvalidateRect(hwnd, nullptr, FALSE);
    return 0;
  case WM_LBUTTONDOWN:
    g_pressB = g_hoverB;
    if (g_hoverT == 1)
      ShowWindow(hwnd, SW_MINIMIZE);
    else if (g_hoverT == 2)
      ShowWindow(hwnd, SW_HIDE);
    else if (g_hoverT == 3) {
      g_cfg.topmost = !g_cfg.topmost;
      SetWindowPos(hwnd, g_cfg.topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0,
                   0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
      ConfigSave();
    } else if (g_hoverT == 4)
      ShowAbout();
    else if (g_hoverT == 5) {
      if (Update_Found())
        ShellExecuteW(hwnd, L"open", URL_RELEASES, nullptr, nullptr,
                      SW_SHOWNORMAL);
      else
        Update_ManualCheck();
    } else if (g_hoverT == 6) {
      if (g_state.load() == ST_IDLE && !g_events.empty()) {
        if (!g_dirty) {
          StatusShow(L(L_SAVED), 2000);
        } else if (!g_lastFile.empty()) {
          if (Engine_Save(g_lastFile.c_str())) {
            g_dirty = false;
            StatusShow(L(L_ST_SAVED), 2500);
          } else
            StatusShow(L(L_SAVEFAIL), 3000);
        } else
          DoCmd(IDM_SAVE);
      }
    }
    InvalidateRect(hwnd, nullptr, FALSE);
    return 0;
  case WM_LBUTTONUP:
    if (g_pressB >= 0 && g_hoverB == g_pressB) {
      int cmd = CMD_BTN[g_pressB];
      g_pressB = -1;
      if (cmd)
        DoCmd(cmd);
      else
        TrackMenu();
      return 0;
    }
    g_pressB = -1;
    return 0;
  case WM_RBUTTONUP:
    TrackMenu();
    return 0;
  case WM_COMMAND:
    DoCmd(LOWORD(w));
    return 0;
  case WM_TIMER:
    if (w == 1)
      StatusTick();
    else if (w == 3)
      TipPoll();
    else if (w == 14)
      Engine_FlushMoves();
    return 0;
  case WM_HOTKEY:
    if (w == HK_ID_REC)
      DoCmd(IDM_REC);
    else if (w == HK_ID_PLAY)
      DoCmd(IDM_PLAY);
    return 0;
  case WM_INPUT:
    Engine_RawInput(l);
    return DefWindowProcW(hwnd, msg, w, l);
  case WM_APP_TRAY:
    if (l == WM_LBUTTONUP) {
      if (IsWindowVisible(hwnd))
        ShowWindow(hwnd, SW_HIDE);
      else {
        ShowWindow(hwnd, SW_SHOW);
        SetForegroundWindow(hwnd);
      }
    } else if (l == WM_RBUTTONUP)
      TrackMenu();
    return 0;
  case WM_APP_UPDATE:
    if (l == 1)
      TrayBalloon(L(L_UPDATE_AVAIL), L"github.com/Agzes/OntyTask/releases");
    else if (w) {
      if (l == 0)
        StatusShow(L(L_ST_UPTODATE), 3000);
      else
        StatusShow(L(L_ST_UPDATEFAIL), 3000);
    }
    if (IsWindow(g_abHwnd))
      InvalidateRect(g_abHwnd, nullptr, FALSE);
    InvalidateRect(hwnd, nullptr, FALSE);
    return 0;
  case WM_APP_PLAYDONE:
    TraySync();
    StatusHide();
    CursorTipHide();
    if (g_cfg.autoMinimize) {
      ShowWindow(hwnd, SW_SHOW);
      SetForegroundWindow(hwnd);
    }
    if (g_playStopped.load())
      StatusShow(L(L_ST_PLAYSTOP), 2500);
    InvalidateRect(hwnd, nullptr, FALSE);
    if (g_closeAfterPlay) {
      DestroyWindow(hwnd);
      return 0;
    }
    return 0;
  case WM_APP + 5:
    if (g_state.load() == ST_PLAY) {
      std::wstring tip = std::wstring(L(L_ST_PLAYING)) + L" \u2022 " +
                         L(L_STOP_BIND) + L": " + ActiveStopKeysText();
      CursorTipShow(tip);
    }
    return 0;
  case WM_KEYDOWN:
    if (GetKeyState(VK_CONTROL) & 0x8000) {
      if (w == 'C' || w == 'c') {
        DoCmd(IDM_COPY_CLIPBOARD);
        return 0;
      }
      if (w == 'V' || w == 'v') {
        DoCmd(IDM_PASTE_CLIPBOARD);
        return 0;
      }
    }
    return DefWindowProcW(hwnd, msg, w, l);
  case WM_COPYDATA: {
    COPYDATASTRUCT *cds = (COPYDATASTRUCT *)l;
    if (cds && cds->lpData) {
      if (cds->dwData == 1) {
        std::wstring p = CleanPath((const wchar_t *)cds->lpData);
        if (Engine_Load(p.c_str())) {
          g_lastFile = p;
          g_dirty = false;
          RecentAdd(p);
          ConfigSave();
          StatusShow(L(L_ST_LOADED), 2500);
          InvalidateRect(hwnd, nullptr, FALSE);
        } else {
          StatusShow(L(L_ST_INVALID), 2500);
          UiMessage(hwnd, L(L_ST_INVALID), L(L_APPNAME), UI_OK, UI_ICON_ERROR);
        }
      } else if (cds->dwData == 2) {
        int argc = 0;
        LPWSTR *argv = CommandLineToArgvW((const wchar_t *)cds->lpData, &argc);
        if (argv) {
          std::wstring f;
          bool play = false, rec = false;
          for (int i = 1; i < argc; i++) {
            std::wstring arg = argv[i];
            if (arg == L"--play" || arg == L"-p")
              play = true;
            else if (arg == L"--rec" || arg == L"-r")
              rec = true;
            else if ((arg == L"--loop" || arg == L"-l") && i + 1 < argc) {
              int n = _wtoi(argv[++i]);
              if (n == 0)
                g_cfg.continuous = 1;
              else {
                g_cfg.loops = n;
                g_cfg.continuous = 0;
              }
            } else if ((arg == L"--speed" || arg == L"-s") && i + 1 < argc) {
              double sp = _wtof(argv[++i]);
              if (sp == 0.25)
                g_cfg.speed = -1;
              else if (sp == 0.5)
                g_cfg.speed = 0;
              else if (sp == 0.75)
                g_cfg.speed = -2;
              else if (sp == 1.0)
                g_cfg.speed = 1;
              else if (sp == 1.25)
                g_cfg.speed = -3;
              else if (sp == 1.5)
                g_cfg.speed = -4;
              else if (sp == 0.0)
                g_cfg.speed = -5;
              else if (sp == 100.0)
                g_cfg.speed = 100;
              else if (sp >= 2 && sp <= 50 && (double)(int)sp == sp)
                g_cfg.speed = (int)sp;
              else {
                g_cfg.speed = 999;
                g_cfg.speedCustom =
                    sp < 0.01 ? 0.01 : (sp > 1000.0 ? 1000.0 : sp);
              }
            } else if (arg == L"--close-after" || arg == L"-c")
              g_closeAfterPlay = true;
            else if (arg == L"--file" || arg == L"-f") {
              if (i + 1 < argc)
                f = CleanPath(argv[++i]);
            } else if (!arg.empty() && arg[0] != L'-') {
              if (f.empty())
                f = CleanPath(arg);
            }
          }
          LocalFree(argv);
          if (!f.empty()) {
            if (Engine_Load(f.c_str())) {
              g_lastFile = f;
              g_dirty = false;
              RecentAdd(f);
              ConfigSave();
              StatusShow(L(L_ST_LOADED), 2500);
            } else {
              StatusShow(L(L_ST_INVALID), 2500);
              UiMessage(hwnd, L(L_ST_INVALID), L(L_APPNAME), UI_OK,
                        UI_ICON_ERROR);
            }
          }
          if (play)
            DoCmd(IDM_PLAY);
          else if (rec)
            DoCmd(IDM_REC);
          InvalidateRect(hwnd, nullptr, FALSE);
        }
      }
    }
    return TRUE;
  }
  case WM_DROPFILES: {
    HDROP hDrop = (HDROP)w;
    wchar_t path[MAX_PATH];
    if (DragQueryFileW(hDrop, 0, path, MAX_PATH)) {
      std::wstring p = CleanPath(path);
      if (Engine_Load(p.c_str())) {
        g_lastFile = p;
        g_dirty = false;
        RecentAdd(p);
        ConfigSave();
        StatusShow(L(L_ST_LOADED), 2500);
        InvalidateRect(hwnd, nullptr, FALSE);
      } else {
        StatusShow(L(L_ST_INVALID), 2500);
        UiMessage(hwnd, L(L_ST_INVALID), L(L_APPNAME), UI_OK, UI_ICON_ERROR);
      }
    }
    DragFinish(hDrop);
    return 0;
  }
  case WM_CLOSE:
    ShowWindow(hwnd, SW_HIDE);
    return 0;
  case WM_QUERYENDSESSION:
    ConfigSave();
    return TRUE;
  case WM_ENDSESSION:
    if (w) {
      Engine_Shutdown();
      ConfigSave();
      TrayRemove();
      DestroyWindow(hwnd);
    }
    return 0;
  case WM_DESTROY:
    KillTimer(hwnd, 1);
    KillTimer(hwnd, 2);
    Engine_Shutdown();
    if (!IsIconic(hwnd)) {
      RECT r;
      GetWindowRect(hwnd, &r);
      g_cfg.x = r.left;
      g_cfg.y = r.top;
    }
    ConfigSave();
    TrayRemove();
    if (g_trayIcon) {
      DestroyIcon(g_trayIcon);
      g_trayIcon = nullptr;
    }
    PostQuitMessage(0);
    return 0;
  default:
    if (msg == g_msgTaskbar && g_msgTaskbar) {
      TrayAdd();
      return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
  }
}

struct InDlg {
  HWND h = nullptr, edit = nullptr;
  const wchar_t *prompt = nullptr;
  int *outInt = nullptr;
  double *outDouble = nullptr;
  double minv = 0, maxv = 0;
  bool isFloat = false;
  bool ok = false, done = false, tracking = false, focus = false;
  int hover = 0;
};

static InDlg *g_in = nullptr;

static const int IN_W = 280, IN_H = 114;
static RECT InCaRect() {
  RECT r = {0, 84, IN_W / 2, IN_H};
  return r;
}
static RECT InOkRect() {
  RECT r = {IN_W / 2, 84, IN_W, IN_H};
  return r;
}
static RECT InBannerRect() {
  RECT r = {0, 30, IN_W, 56};
  return r;
}
static RECT InInputRow() {
  RECT r = {0, 56, IN_W, 84};
  return r;
}

static COLORREF InFieldColor(bool focus) {
  if (g_cfg.theme == THEME_LIGHT)
    return focus ? RGB(225, 225, 225) : RGB(238, 238, 238);
  if (g_cfg.theme == THEME_ACRYLIC)
    return focus ? RGB(34, 34, 34) : RGB(21, 21, 21);
  return focus ? RGB(46, 46, 46) : RGB(37, 37, 37);
}

static LRESULT CALLBACK InputEditProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  WNDPROC old = (WNDPROC)GetWindowLongPtrW(h, GWLP_USERDATA);
  if (m == WM_KEYDOWN && (w == VK_RETURN || w == VK_ESCAPE)) {
    PostMessageW(GetParent(h), w == VK_RETURN ? WM_IN_OK : WM_IN_CANCEL, 0, 0);
    return 0;
  }
  return CallWindowProcW(old, h, m, w, l);
}

static LRESULT CALLBACK InputProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  InDlg *d = g_in;
  switch (m) {
  case WM_CREATE: {
    CREATESTRUCTW *cs = (CREATESTRUCTW *)l;
    d = (InDlg *)cs->lpCreateParams;
    d->h = h;
    DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT |
                  (d->isFloat ? 0 : ES_NUMBER);
    d->edit = CreateWindowExW(0, L"EDIT", L"", style, 14, 60, IN_W - 28, 20, h,
                              (HMENU)1, g_hInst, nullptr);
    SendMessageW(d->edit, WM_SETFONT, (WPARAM)ThFont(10), TRUE);
    WNDPROC old = (WNDPROC)SetWindowLongPtrW(d->edit, GWLP_WNDPROC,
                                             (LONG_PTR)InputEditProc);
    SetWindowLongPtrW(d->edit, GWLP_USERDATA, (LONG_PTR)old);
    SetFocus(d->edit);
    ApplyWindowMaterial(h);
    TryRoundCorners(h);
    return 0;
  }
  case WM_CTLCOLOREDIT: {
    HDC dc = (HDC)w;
    SetTextColor(dc, ThText());
    COLORREF c = InFieldColor(d->focus);
    SetBkColor(dc, c);
    static COLORREF bc = -1;
    static HBRUSH br = nullptr;
    if (!br || c != bc) {
      if (br)
        DeleteObject(br);
      br = CreateSolidBrush(c);
      bc = c;
    }
    return (LRESULT)br;
  }
  case WM_IN_OK: {
    wchar_t b[64];
    GetWindowTextW(d->edit, b, 64);
    for (int i = 0; b[i]; i++)
      if (b[i] == L',')
        b[i] = L'.';
    if (d->isFloat) {
      double v = wcstod(b, nullptr);
      if (v < d->minv)
        v = d->minv;
      if (v > d->maxv)
        v = d->maxv;
      if (d->outDouble)
        *d->outDouble = v;
    } else {
      int v = wcstol(b, nullptr, 10);
      if ((double)v < d->minv)
        v = (int)d->minv;
      if ((double)v > d->maxv)
        v = (int)d->maxv;
      if (d->outInt)
        *d->outInt = v;
    }
    d->ok = true;
    d->done = true;
    DestroyWindow(h);
    return 0;
  }
  case WM_IN_CANCEL:
    d->done = true;
    DestroyWindow(h);
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_NCHITTEST: {
    POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
    ScreenToClient(h, &pt);
    RECT cr = GfxCloseRect(IN_W);
    if (pt.y < TB_H && !PtInRect(&cr, pt))
      return HTCAPTION;
    return HTCLIENT;
  }
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(h, &ps);
    RECT rc;
    GetClientRect(h, &rc);
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HGDIOBJ old = SelectObject(mem, bm);
    Gdiplus::Graphics g(mem);
    bool acry = g_cfg.theme == THEME_ACRYLIC;
    bool light = g_cfg.theme == THEME_LIGHT;
    Gdiplus::SolidBrush bg(acry ? GfxAcrylicBg() : GdiCol(ThBg()));
    g.FillRectangle(&bg, 0, 0, rc.right, rc.bottom);
    Gdiplus::Pen pen(acry ? Gdiplus::Color(180, 56, 56, 56)
                          : GdiCol(ThBorder()));
    g.DrawRectangle(&pen, 0, 0, rc.right - 1, rc.bottom - 1);
    GfxChrome(g, mem, rc.right, L(L_SETTINGS), d->hover == 3 ? 2 : 0, CHB_SEP);
    Gdiplus::SolidBrush txt(GdiCol(ThText()));
    Gdiplus::SolidBrush gsep(GdiCol(ThBorder()));

    RECT rb = InBannerRect();
    Gdiplus::SolidBrush banf(acry ? Gdiplus::Color(40, 50, 50, 50)
                                  : (light ? Gdiplus::Color(245, 245, 245)
                                           : Gdiplus::Color(60, 45, 45, 45)));
    g.FillRectangle(&banf, (REAL)rb.left, (REAL)rb.top,
                    (REAL)(rb.right - rb.left), (REAL)(rb.bottom - rb.top));
    g.FillRectangle(&gsep, (REAL)rb.left, (REAL)rb.top,
                    (REAL)(rb.right - rb.left), 1.0f);
    g.FillRectangle(&gsep, (REAL)rb.left, (REAL)rb.top, 1.0f,
                    (REAL)(rb.bottom - rb.top));
    g.FillRectangle(&gsep, (REAL)(rb.right - 1), (REAL)rb.top, 1.0f,
                    (REAL)(rb.bottom - rb.top));
    {
      g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
      g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
      g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
      DrawGlyph(mem, g, 16.0f, 43.0f, 11, (wchar_t)0xE946, ThAccent());
      Gdiplus::Font f(mem, ThFont(9));
      static Gdiplus::StringFormat sfLV;
      sfLV.SetAlignment(Gdiplus::StringAlignmentNear);
      sfLV.SetLineAlignment(Gdiplus::StringAlignmentCenter);
      Gdiplus::SolidBrush dimBrush(GdiCol(ThDim()));
      g.DrawString(d->prompt, -1, &f,
                   Gdiplus::RectF(30.0f, (REAL)rb.top, (REAL)(rb.right - 38),
                                  (REAL)(rb.bottom - rb.top)),
                   &sfLV, &dimBrush);
    }

    RECT ri = InInputRow();
    Gdiplus::SolidBrush inf(GdiCol(InFieldColor(d->focus)));
    g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
    g.FillRectangle(&inf, (REAL)ri.left, (REAL)ri.top,
                    (REAL)(ri.right - ri.left), (REAL)(ri.bottom - ri.top));
    g.FillRectangle(&gsep, (REAL)ri.left, (REAL)ri.top,
                    (REAL)(ri.right - ri.left), 1.0f);
    g.FillRectangle(&gsep, (REAL)ri.left, (REAL)ri.top, 1.0f,
                    (REAL)(ri.bottom - ri.top));
    g.FillRectangle(&gsep, (REAL)(ri.right - 1), (REAL)ri.top, 1.0f,
                    (REAL)(ri.bottom - ri.top));

    RECT ro = InOkRect(), rca = InCaRect();
    Gdiplus::SolidBrush okf(
        acry ? (d->hover == 1 ? Gdiplus::Color(140, 80, 160, 224)
                              : Gdiplus::Color(100, 70, 140, 200))
             : (light ? (d->hover == 1 ? Gdiplus::Color(255, 90, 170, 220)
                                       : Gdiplus::Color(255, 80, 155, 210))
                      : (d->hover == 1 ? Gdiplus::Color(255, 70, 145, 205)
                                       : Gdiplus::Color(255, 55, 125, 185))));
    Gdiplus::SolidBrush caf(
        acry ? (d->hover == 2 ? Gdiplus::Color(70, 70, 70, 70)
                              : Gdiplus::Color(40, 50, 50, 50))
             : (light ? (d->hover == 2 ? Gdiplus::Color(230, 220, 220, 220)
                                       : Gdiplus::Color(255, 245, 245, 245))
                      : (d->hover == 2 ? Gdiplus::Color(255, 60, 60, 60)
                                       : Gdiplus::Color(255, 42, 42, 42))));
    g.FillRectangle(&okf, (REAL)ro.left, (REAL)ro.top,
                    (REAL)(ro.right - ro.left), (REAL)(ro.bottom - ro.top));
    g.FillRectangle(&caf, (REAL)rca.left, (REAL)rca.top,
                    (REAL)(rca.right - rca.left), (REAL)(rca.bottom - rca.top));
    g.FillRectangle(&gsep, (REAL)rca.left, (REAL)rca.top,
                    (REAL)(ro.right - rca.left), 1.0f);
    g.FillRectangle(&gsep, (REAL)(ro.left), (REAL)rca.top, 1.0f,
                    (REAL)(rca.bottom - rca.top));
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
    Gdiplus::Font fb(mem, ThFont(10, false));
    Gdiplus::SolidBrush okText(GdiCol(RGB(240, 245, 250)));
    g.DrawString(L(L_OK), -1, &fb,
                 Gdiplus::RectF((REAL)ro.left, (REAL)ro.top,
                                (REAL)(ro.right - ro.left),
                                (REAL)(ro.bottom - ro.top)),
                 &SfC(), &okText);
    g.DrawString(L(L_CANCEL), -1, &fb,
                 Gdiplus::RectF((REAL)rca.left, (REAL)rca.top,
                                (REAL)(rca.right - rca.left),
                                (REAL)(rca.bottom - rca.top)),
                 &SfC(), &txt);
    BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteObject(bm);
    DeleteDC(mem);
    EndPaint(h, &ps);
    return 0;
  }
  case WM_COMMAND:
    if (HIWORD(w) == EN_SETFOCUS) {
      d->focus = true;
      InvalidateRect(h, nullptr, FALSE);
    } else if (HIWORD(w) == EN_KILLFOCUS) {
      d->focus = false;
      InvalidateRect(h, nullptr, FALSE);
    }
    return 0;
  case WM_MOUSEMOVE: {
    POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
    RECT ro = InOkRect(), rca = InCaRect(), rcl = GfxCloseRect(IN_W);
    int nh = PtInRect(&ro, pt)    ? 1
             : PtInRect(&rca, pt) ? 2
             : PtInRect(&rcl, pt) ? 3
                                  : 0;
    if (nh != d->hover) {
      d->hover = nh;
      InvalidateRect(h, nullptr, FALSE);
    }
    if (!d->tracking) {
      d->tracking = true;
      TRACKMOUSEEVENT t = {sizeof(t), TME_LEAVE, h, 0};
      TrackMouseEvent(&t);
    }
    return 0;
  }
  case WM_MOUSELEAVE:
    d->hover = 0;
    InvalidateRect(h, nullptr, FALSE);
    return 0;
  case WM_LBUTTONUP:
    if (d->hover == 1)
      PostMessageW(h, WM_IN_OK, 0, 0);
    else if (d->hover == 2 || d->hover == 3)
      PostMessageW(h, WM_IN_CANCEL, 0, 0);
    return 0;
  case WM_DESTROY:
    EnableWindow(g_hwnd, TRUE);
    SetForegroundWindow(g_hwnd);
    return 0;
  }
  return DefWindowProcW(h, m, w, l);
}

static bool InputDialog(const wchar_t *prompt, int init, int minv, int maxv,
                        int &out) {
  InDlg d;
  d.prompt = prompt;
  d.outInt = &out;
  d.minv = (double)minv;
  d.maxv = (double)maxv;
  d.isFloat = false;
  g_in = &d;
  RECT rw;
  GetWindowRect(g_hwnd, &rw);
  int x = rw.left + ((rw.right - rw.left) - IN_W) / 2;
  int y = rw.top + ((rw.bottom - rw.top) - IN_H) / 2;
  EnableWindow(g_hwnd, FALSE);
  HWND h = CreateWindowExW(WS_EX_TOOLWINDOW, L"OntyTaskInput", L"", WS_POPUP, x,
                           y, IN_W, IN_H, g_hwnd, nullptr, g_hInst, &d);
  GfxRoundRegion(h);
  wchar_t b[32];
  swprintf_s(b, L"%d", init);
  SetWindowTextW(d.edit, b);
  SendMessageW(d.edit, EM_SETSEL, 0, -1);
  ShowWindow(h, SW_SHOW);
  SetForegroundWindow(h);
  MSG m;
  while (!d.done && IsWindow(h) && GetMessageW(&m, nullptr, 0, 0) > 0) {
    TranslateMessage(&m);
    DispatchMessageW(&m);
  }
  g_in = nullptr;
  return d.ok;
}

static bool InputDialogFloat(const wchar_t *prompt, double init, double minv,
                             double maxv, double &out) {
  InDlg d;
  d.prompt = prompt;
  d.outDouble = &out;
  d.minv = minv;
  d.maxv = maxv;
  d.isFloat = true;
  g_in = &d;
  RECT rw;
  GetWindowRect(g_hwnd, &rw);
  int x = rw.left + ((rw.right - rw.left) - IN_W) / 2;
  int y = rw.top + ((rw.bottom - rw.top) - IN_H) / 2;
  EnableWindow(g_hwnd, FALSE);
  HWND h = CreateWindowExW(WS_EX_TOOLWINDOW, L"OntyTaskInput", L"", WS_POPUP, x,
                           y, IN_W, IN_H, g_hwnd, nullptr, g_hInst, &d);
  GfxRoundRegion(h);
  wchar_t b[32];
  swprintf_s(b, L"%.4g", init);
  SetWindowTextW(d.edit, b);
  SendMessageW(d.edit, EM_SETSEL, 0, -1);
  ShowWindow(h, SW_SHOW);
  SetForegroundWindow(h);
  MSG m;
  while (!d.done && IsWindow(h) && GetMessageW(&m, nullptr, 0, 0) > 0) {
    TranslateMessage(&m);
    DispatchMessageW(&m);
  }
  g_in = nullptr;
  return d.ok;
}

struct AbDlg {
  HWND h = nullptr;
  bool done = false, tracking = false;
  int hover = 0;
};

static AbDlg *g_ab = nullptr;

static const int AB_W = 280, AB_H = 228;
static RECT AbCloseRect() {
  RECT r = {AB_W - 30, 0, AB_W, 30};
  return r;
}
static RECT AbUpdRect() {
  RECT r = {0, AB_H - 58, AB_W, AB_H - 32};
  return r;
}
static RECT AbGhRect() {
  RECT r = {0, AB_H - 32, AB_W / 2, AB_H};
  return r;
}
static RECT AbLiRect() {
  RECT r = {AB_W / 2, AB_H - 32, AB_W, AB_H};
  return r;
}

static std::vector<std::wstring> WrapText(HDC dc, const std::wstring &s,
                                          int maxW) {
  std::vector<std::wstring> out;
  std::wstring line;
  size_t i = 0;
  while (i < s.size()) {
    size_t j = s.find(L' ', i);
    if (j == std::wstring::npos)
      j = s.size();
    std::wstring word = s.substr(i, j - i);
    std::wstring test = line.empty() ? word : line + L" " + word;
    SIZE sz;
    GetTextExtentPoint32W(dc, test.c_str(), (int)test.size(), &sz);
    if (sz.cx > maxW && !line.empty()) {
      out.push_back(line);
      line = word;
    } else
      line = test;
    i = j + 1;
  }
  if (!line.empty())
    out.push_back(line);
  return out;
}

static LRESULT CALLBACK AboutProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  AbDlg *d = g_ab;
  switch (m) {
  case WM_CREATE: {
    CREATESTRUCTW *cs = (CREATESTRUCTW *)l;
    d = (AbDlg *)cs->lpCreateParams;
    g_ab = d;
    if (d)
      d->h = h;
    g_abHwnd = h;
    ApplyWindowMaterial(h);
    TryRoundCorners(h);
    return 0;
  }
  case WM_ERASEBKGND:
    return 1;
  case WM_NCHITTEST: {
    POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
    ScreenToClient(h, &pt);
    RECT cr = AbCloseRect();
    if (pt.y < 72 && !PtInRect(&cr, pt))
      return HTCAPTION;
    return HTCLIENT;
  }
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(h, &ps);
    RECT rc;
    GetClientRect(h, &rc);
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HGDIOBJ old = SelectObject(mem, bm);
    Gdiplus::Graphics g(mem);
    bool acry = g_cfg.theme == THEME_ACRYLIC;
    bool light = g_cfg.theme == THEME_LIGHT;
    g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
    Gdiplus::SolidBrush bg(acry ? GfxAcrylicBg() : GdiCol(ThBg()));
    g.FillRectangle(&bg, 0, 0, rc.right, rc.bottom);
    Gdiplus::SolidBrush header(acry ? Gdiplus::Color(100, 10, 10, 10)
                                    : GdiCol(ThBg2()));
    g.FillRectangle(&header, 0, 0, AB_W, 72);
    Gdiplus::SolidBrush accent(GdiCol(ThAccent()));
    g.FillRectangle(&accent, 0, 72, AB_W, 1);
    Gdiplus::SolidBrush bb(GdiCol(ThBorder()));
    Gdiplus::Pen borderPen(GdiCol(ThBorder()));
    g.DrawRectangle(&borderPen, 0, 0, rc.right - 1, rc.bottom - 1);
    RECT rcl = AbCloseRect();
    int hover = d ? d->hover : 0;
    if (hover == 3) {
      Gdiplus::SolidBrush red(Gdiplus::Color(255, 196, 43, 28));
      g.FillRectangle(&red, (REAL)rcl.left, (REAL)rcl.top,
                      (REAL)(rcl.right - rcl.left),
                      (REAL)(rcl.bottom - rcl.top));
    }
    {
      int cx = (rcl.left + rcl.right) / 2;
      REAL cy = 14.5f, s = 5.0f;
      g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
      Gdiplus::Pen xp(GdiCol(hover == 3 ? RGB(255, 255, 255) : ThText()), 1.0f);
      g.DrawLine(&xp, (REAL)cx - s, cy - s, (REAL)cx + s, cy + s);
      g.DrawLine(&xp, (REAL)cx + s, cy - s, (REAL)cx - s, cy + s);
      g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    }

    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
    Gdiplus::Font f18b(mem, ThFont(18, true));
    Gdiplus::Font f10(mem, ThFont(10));
    Gdiplus::Font f9(mem, ThFont(9));
    Gdiplus::Font f9b(mem, ThFont(9, true));
    Gdiplus::SolidBrush txt(GdiCol(ThText()));
    Gdiplus::SolidBrush dim(GdiCol(ThDim()));
    Gdiplus::SolidBrush ac(GdiCol(ThAccent()));
    HICON ic = LoadAppIconId(GfxUiIconId(), 36);
    if (ic) {
      DrawIconEx(mem, 18, 18, ic, 36, 36, 0, nullptr, DI_NORMAL);
      DestroyIcon(ic);
    }
    g.DrawString(L(L_APPNAME), -1, &f18b, Gdiplus::PointF(64, 13), &txt);
    g.DrawString(APP_VERSION, -1, &f9, Gdiplus::PointF(65, 38), &dim);

    std::wstring desc = L(L_ABOUT_DESC);
    SelectObject(mem, ThFont(10));
    std::vector<std::wstring> lines = WrapText(mem, desc, AB_W - 36);
    int dy = 84;
    for (const auto &ln : lines) {
      g.DrawString(ln.c_str(), -1, &f10, Gdiplus::PointF(18, (REAL)dy), &txt);
      dy += 15;
    }

    std::wstring meta = std::wstring(L(L_DEVELOPED)) + L"\n" + L(L_LIC_LINE);
    g.DrawString(meta.c_str(), -1, &f9,
                 Gdiplus::RectF(18.0f, (REAL)dy + 6, (REAL)(AB_W - 36), 30.0f),
                 &SfL(), &dim);

    RECT ru = AbUpdRect();
    if (hover == 4) {
      g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
      Gdiplus::SolidBrush hb(Gdiplus::Color(35, 90, 90, 90));
      g.FillRectangle(&hb, (REAL)ru.left, (REAL)ru.top,
                      (REAL)(ru.right - ru.left), (REAL)(ru.bottom - ru.top));
      g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    }
    g.FillRectangle(&bb, (REAL)ru.left, (REAL)ru.top,
                    (REAL)(ru.right - ru.left), 1.0f);
    g.FillRectangle(&bb, (REAL)ru.left, (REAL)(ru.bottom - 1),
                    (REAL)(ru.right - ru.left), 1.0f);
    const wchar_t *upTxt = L(L_UPDATE);
    int ust = Update_State();
    if (ust == UR_LATEST)
      upTxt = L(L_ST_UPTODATE);
    else if (ust == UR_FOUND)
      upTxt = L(L_UPDATE_AVAIL);
    else if (ust == UR_FAIL)
      upTxt = L(L_ST_UPDATEFAIL);
    Gdiplus::SolidBrush ufb(GdiCol(ust == UR_FOUND ? RecRed()
                                   : (ust == UR_LATEST || ust == UR_CHECKING)
                                       ? ThAccent()
                                       : ThDim()));
    g.DrawString(upTxt, -1, &f9,
                 Gdiplus::RectF((REAL)ru.left, (REAL)ru.top,
                                (REAL)(ru.right - ru.left),
                                (REAL)(ru.bottom - ru.top)),
                 &SfC(), &ufb);
    RECT rg = AbGhRect(), rl2 = AbLiRect();
    Gdiplus::Color ghc = light
                             ? GdiCol(hover == 1 ? ThHover2() : ThBg2())
                             : Gdiplus::Color(hover == 1 ? 70 : 50, 60, 60, 60);
    Gdiplus::Color lic = light
                             ? GdiCol(hover == 2 ? ThHover2() : ThBg2())
                             : Gdiplus::Color(hover == 2 ? 70 : 50, 60, 60, 60);
    Gdiplus::SolidBrush ghf(ghc);
    Gdiplus::SolidBrush lif(lic);
    g.FillRectangle(&ghf, (REAL)rg.left, (REAL)rg.top,
                    (REAL)(rg.right - rg.left), (REAL)(rg.bottom - rg.top));
    g.FillRectangle(&lif, (REAL)rl2.left, (REAL)rl2.top,
                    (REAL)(rl2.right - rl2.left), (REAL)(rl2.bottom - rl2.top));
    g.FillRectangle(&bb, (REAL)rg.left, (REAL)rg.top, 1.0f,
                    (REAL)(rg.bottom - rg.top));
    g.FillRectangle(&bb, (REAL)(rg.right - 1), (REAL)rg.top, 1.0f,
                    (REAL)(rg.bottom - rg.top));
    g.FillRectangle(&bb, (REAL)(rl2.right - 1), (REAL)rl2.top, 1.0f,
                    (REAL)(rl2.bottom - rl2.top));
    g.FillRectangle(&bb, (REAL)rg.left, (REAL)rg.bottom - 1,
                    (REAL)(rl2.right - rg.left), 1.0f);
    static Gdiplus::StringFormat sfA;
    sfA.SetAlignment(Gdiplus::StringAlignmentCenter);
    sfA.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    COLORREF btnTxt = light ? ThText() : RGB(255, 255, 255);
    Gdiplus::SolidBrush btb(GdiCol(btnTxt));
    g.DrawString(L(L_GITHUB), -1, &f9b,
                 Gdiplus::RectF((REAL)rg.left, (REAL)rg.top,
                                (REAL)(rg.right - rg.left),
                                (REAL)(rg.bottom - rg.top)),
                 &sfA, &btb);
    g.DrawString(L(L_LICENSE), -1, &f9b,
                 Gdiplus::RectF((REAL)rl2.left, (REAL)rl2.top,
                                (REAL)(rl2.right - rl2.left),
                                (REAL)(rl2.bottom - rl2.top)),
                 &sfA, &btb);
    BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteObject(bm);
    DeleteDC(mem);
    EndPaint(h, &ps);
    return 0;
  }
  case WM_MOUSEMOVE: {
    POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
    RECT rg = AbGhRect(), rl = AbLiRect(), rcl = AbCloseRect(),
         ru = AbUpdRect();
    int nh = PtInRect(&rg, pt)    ? 1
             : PtInRect(&rl, pt)  ? 2
             : PtInRect(&rcl, pt) ? 3
             : PtInRect(&ru, pt)  ? 4
                                  : 0;
    if (d && nh != d->hover) {
      d->hover = nh;
      InvalidateRect(h, nullptr, FALSE);
    }
    SetCursor(nh ? LoadCursorW(nullptr, IDC_HAND)
                 : LoadCursorW(nullptr, IDC_ARROW));
    if (d && !d->tracking) {
      d->tracking = true;
      TRACKMOUSEEVENT t = {sizeof(t), TME_LEAVE, h, 0};
      TrackMouseEvent(&t);
    }
    return 0;
  }
  case WM_MOUSELEAVE:
    if (d)
      d->hover = 0;
    InvalidateRect(h, nullptr, FALSE);
    return 0;
  case WM_LBUTTONUP:
    if (!d)
      return 0;
    if (d->hover == 1)
      ShellExecuteW(h, L"open", URL_GITHUB, nullptr, nullptr, SW_SHOWNORMAL);
    else if (d->hover == 2)
      ShellExecuteW(h, L"open", URL_LICENSE, nullptr, nullptr, SW_SHOWNORMAL);
    else if (d->hover == 3) {
      d->done = true;
      DestroyWindow(h);
    } else if (d->hover == 4) {
      if (Update_State() == UR_FOUND)
        ShellExecuteW(h, L"open", URL_RELEASES, nullptr, nullptr,
                      SW_SHOWNORMAL);
      else
        Update_ManualCheck();
    }
    return 0;
  case WM_DESTROY:
    g_abHwnd = nullptr;
    g_ab = nullptr;
    EnableWindow(g_hwnd, TRUE);
    SetForegroundWindow(g_hwnd);
    return 0;
  }
  return DefWindowProcW(h, m, w, l);
}

static void ShowAbout() {
  if (IsWindow(g_abHwnd)) {
    ShowWindow(g_abHwnd, SW_SHOW);
    SetForegroundWindow(g_abHwnd);
    return;
  }
  static AbDlg s_ab;
  s_ab = AbDlg();
  g_ab = &s_ab;
  RECT pr;
  GetWindowRect(g_hwnd, &pr);
  int x = pr.left + (pr.right - pr.left - AB_W) / 2;
  int y = pr.top + (pr.bottom - pr.top - AB_H) / 2;
  g_abHwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"OntyTaskAbout",
                             L"About OntyTask", WS_POPUP | WS_VISIBLE, x, y,
                             AB_W, AB_H, g_hwnd, nullptr, g_hInst, &s_ab);
}

static void RegisterClasses() {
  WNDCLASSW wc = {};
  wc.hInstance = g_hInst;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.lpfnWndProc = MainProc;
  wc.lpszClassName = L"OntyTaskMain";
  RegisterClassW(&wc);
  wc.lpfnWndProc = InputProc;
  wc.lpszClassName = L"OntyTaskInput";
  RegisterClassW(&wc);
  wc.lpfnWndProc = AboutProc;
  wc.lpszClassName = L"OntyTaskAbout";
  RegisterClassW(&wc);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int) {
  g_hInst = hInst;
  {
    typedef BOOL(WINAPI * SetProcessDpiAwarenessContext_fn)(HANDLE);
    auto fn = (SetProcessDpiAwarenessContext_fn)GetProcAddress(
        GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext");
    if (fn)
      fn((HANDLE)-5);
    else
      SetProcessDPIAware();
  }

  struct CliArgs {
    std::wstring file;
    bool play = false;
    bool rec = false;
    bool min = false;
    bool closeAfter = false;
    int loop = -1;
    double speed = -100.0;
  } cli;

  int argc = 0;
  LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argv) {
    for (int i = 1; i < argc; i++) {
      std::wstring arg = argv[i];
      if (arg == L"--install" || arg == L"-i") {
        InstallToLocalApp();
        LocalFree(argv);
        return 0;
      } else if (arg == L"--uninstall" || arg == L"-u") {
        UninstallFromLocalApp();
        LocalFree(argv);
        return 0;
      } else if (arg == L"--play" || arg == L"-p")
        cli.play = true;
      else if (arg == L"--rec" || arg == L"-r")
        cli.rec = true;
      else if (arg == L"--min" || arg == L"-m" || arg == L"--hide")
        cli.min = true;
      else if (arg == L"--close-after" || arg == L"-c")
        cli.closeAfter = true;
      else if ((arg == L"--loop" || arg == L"-l") && i + 1 < argc) {
        cli.loop = _wtoi(argv[++i]);
      } else if ((arg == L"--speed" || arg == L"-s") && i + 1 < argc) {
        wchar_t *endp = nullptr;
        cli.speed = wcstod(argv[++i], &endp);
      } else if (arg == L"--file" || arg == L"-f") {
        if (i + 1 < argc)
          cli.file = CleanPath(argv[++i]);
      } else if (!arg.empty() && arg[0] != L'-') {
        if (cli.file.empty())
          cli.file = CleanPath(arg);
      }
    }
    LocalFree(argv);
  }

  HANDLE mutex = CreateMutexW(nullptr, FALSE, L"OntyTask.SingleInstance");
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    HWND prev = FindWindowW(L"OntyTaskMain", nullptr);
    if (prev) {
      std::wstring cmdLine = GetCommandLineW();
      COPYDATASTRUCT cds = {};
      cds.dwData = 2;
      cds.cbData = (DWORD)((cmdLine.size() + 1) * sizeof(wchar_t));
      cds.lpData = (PVOID)cmdLine.c_str();
      SendMessageW(prev, WM_COPYDATA, 0, (LPARAM)&cds);
      if (!cli.min) {
        ShowWindow(prev, SW_SHOW);
        SetForegroundWindow(prev);
      }
    }
    CloseHandle(mutex);
    return 0;
  }

  AutoSyncInstalledExe();

  ConfigLoad();
  LangInit();
  GfxInit();
  g_msgTaskbar = RegisterWindowMessageW(L"TaskbarCreated");
  RegisterClasses();
  int x = g_cfg.x, y = g_cfg.y;
  int vx = GetSystemMetrics(SM_XVIRTUALSCREEN),
      vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
  int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN),
      vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  if (x < vx - WIN_W + 20 || x > vx + vw - 20 || y < vy || y > vy + vh - 40) {
    x = vx + (vw - WIN_W) / 2;
    y = vy + (vh - WIN_H) / 3;
  }
  g_hwnd = CreateWindowExW(0, L"OntyTaskMain", L"OntyTask",
                           WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX, x, y, WIN_W,
                           WIN_H, nullptr, nullptr, hInst, nullptr);

  if (!cli.file.empty()) {
    if (Engine_Load(cli.file.c_str())) {
      g_lastFile = cli.file;
      g_dirty = false;
      RecentAdd(cli.file);
      StatusShow(L(L_ST_LOADED), 2500);
      InvalidateRect(g_hwnd, nullptr, FALSE);
    } else {
      StatusShow(L(L_ST_INVALID), 2500);
      UiMessage(g_hwnd, L(L_ST_INVALID), L(L_APPNAME), UI_OK, UI_ICON_ERROR);
    }
  }

  if (cli.closeAfter)
    g_closeAfterPlay = true;
  if (cli.loop >= 0) {
    if (cli.loop == 0)
      g_cfg.continuous = 1;
    else {
      g_cfg.loops = cli.loop;
      g_cfg.continuous = 0;
    }
  }
  if (cli.speed > -90.0) {
    if (cli.speed == 0.25)
      g_cfg.speed = -1;
    else if (cli.speed == 0.5)
      g_cfg.speed = 0;
    else if (cli.speed == 0.75)
      g_cfg.speed = -2;
    else if (cli.speed == 1.0)
      g_cfg.speed = 1;
    else if (cli.speed == 1.25)
      g_cfg.speed = -3;
    else if (cli.speed == 1.5)
      g_cfg.speed = -4;
    else if (cli.speed == 0.0)
      g_cfg.speed = -5;
    else if (cli.speed == 100.0)
      g_cfg.speed = 100;
    else if (cli.speed >= 2 && cli.speed <= 50 &&
             (double)(int)cli.speed == cli.speed)
      g_cfg.speed = (int)cli.speed;
    else {
      g_cfg.speed = 999;
      g_cfg.speedCustom =
          cli.speed < 0.01 ? 0.01 : (cli.speed > 1000.0 ? 1000.0 : cli.speed);
    }
  }

  if (!cli.min)
    ShowWindow(g_hwnd, SW_SHOW);
  else
    ShowWindow(g_hwnd, SW_HIDE);

  Engine_InitRawInput();
  TrayAdd();
  Update_AutoCheck();

  if (cli.play)
    DoCmd(IDM_PLAY);
  else if (cli.rec)
    DoCmd(IDM_REC);

  MSG m;
  while (GetMessageW(&m, nullptr, 0, 0) > 0) {
    TranslateMessage(&m);
    DispatchMessageW(&m);
  }
  GfxShutdown();
  CloseHandle(mutex);
  return 0;
}
