#include "ui/msgbox.h"
#include "core/config.h"
#include "engine/engine.h"
#include "lang/lang.h"
#include "ui/gfx.h"
#include <string>
#include <vector>
#include <windowsx.h>

static COLORREF MRecRed() { return RGB(232, 64, 54); }
static COLORREF MBlue() { return RGB(96, 180, 224); }

struct UiMsg {
  std::wstring text, caption;
  int buttons = UI_OK;
  int icon = UI_ICON_NONE;
  HWND owner = nullptr;
  std::vector<std::pair<int, RECT>> rects;
  RECT close = {};
  int hoverBtn = -1, hoverClose = 0;
  bool tracking = false;
  bool done = false;
  int result = 0;
  int dlgW = 380, dlgH = 150;
  int titleH = 32;
  int iconX = 20, iconY = 48, iconSize = 32;
  int textX = 66, textY = 48, textW = 294, textH = 32;
};

static UiMsg *g_msg = nullptr;

static const wchar_t *BtnLabel(int id) {
  if (id == IDOK || id == IDCANCEL)
    return id == IDOK ? L(L_OK) : L(L_CANCEL);
  return id == IDYES ? L(L_YES) : L(L_NO);
}

static bool IsPrimary(int id) { return id == IDOK || id == IDYES; }

static void DrawCloseX(Gdiplus::Graphics &g, const RECT &rc, bool hover,
                       COLORREF col) {
  if (hover) {
    g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    Gdiplus::SolidBrush redBrush(Gdiplus::Color(255, 196, 43, 28));
    g.FillRectangle(&redBrush, (REAL)rc.left, (REAL)rc.top,
                    (REAL)(rc.right - rc.left), (REAL)(rc.bottom - rc.top));
  }
  g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
  REAL cx = (REAL)(rc.left + rc.right) / 2.0f;
  REAL cy = (REAL)(rc.top + rc.bottom) / 2.0f;
  REAL s = 4.5f;
  Gdiplus::Pen xp(GdiCol(hover ? RGB(255, 255, 255) : col), 1.0f);
  g.DrawLine(&xp, cx - s, cy - s, cx + s, cy + s);
  g.DrawLine(&xp, cx + s, cy - s, cx - s, cy + s);
}

static void DrawDialogIcon(Gdiplus::Graphics &g, int iconType, REAL x, REAL y,
                           REAL size) {
  if (iconType == UI_ICON_NONE)
    return;
  g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

  REAL cx = x + size / 2.0f;
  REAL cy = y + size / 2.0f;

  if (iconType == UI_ICON_ERROR) {
    Gdiplus::SolidBrush bg(Gdiplus::Color(255, 232, 64, 54));
    g.FillEllipse(&bg, x, y, size, size);

    REAL s = size * 0.22f;
    REAL stroke = (std::max)(1.8f, size * 0.075f);
    Gdiplus::Pen pen(Gdiplus::Color(255, 255, 255, 255), stroke);
    pen.SetStartCap(Gdiplus::LineCapRound);
    pen.SetEndCap(Gdiplus::LineCapRound);
    g.DrawLine(&pen, cx - s, cy - s, cx + s, cy + s);
    g.DrawLine(&pen, cx + s, cy - s, cx - s, cy + s);
  } else if (iconType == UI_ICON_WARN) {
    Gdiplus::SolidBrush bg(Gdiplus::Color(255, 245, 158, 11));
    g.FillEllipse(&bg, x, y, size, size);

    REAL stroke = (std::max)(2.0f, size * 0.08f);
    Gdiplus::Pen pen(Gdiplus::Color(255, 255, 255, 255), stroke);
    pen.SetStartCap(Gdiplus::LineCapRound);
    pen.SetEndCap(Gdiplus::LineCapRound);
    g.DrawLine(&pen, cx, cy - size * 0.24f, cx, cy + size * 0.06f);

    REAL dotR = stroke * 0.55f;
    Gdiplus::SolidBrush dotBrush(Gdiplus::Color(255, 255, 255, 255));
    g.FillEllipse(&dotBrush, cx - dotR, cy + size * 0.20f - dotR, dotR * 2,
                  dotR * 2);
  } else if (iconType == UI_ICON_INFO) {
    Gdiplus::SolidBrush bg(Gdiplus::Color(255, 45, 130, 240));
    g.FillEllipse(&bg, x, y, size, size);

    REAL stroke = (std::max)(2.0f, size * 0.08f);
    Gdiplus::SolidBrush dotBrush(Gdiplus::Color(255, 255, 255, 255));
    REAL dotR = stroke * 0.55f;
    g.FillEllipse(&dotBrush, cx - dotR, cy - size * 0.22f - dotR, dotR * 2,
                  dotR * 2);

    Gdiplus::Pen pen(Gdiplus::Color(255, 255, 255, 255), stroke);
    pen.SetStartCap(Gdiplus::LineCapRound);
    pen.SetEndCap(Gdiplus::LineCapRound);
    g.DrawLine(&pen, cx, cy - size * 0.06f, cx, cy + size * 0.22f);
  }
}

static LRESULT CALLBACK MsgProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  UiMsg *d = g_msg;
  switch (m) {
  case WM_CREATE: {
    d = (UiMsg *)((CREATESTRUCTW *)l)->lpCreateParams;
    g_msg = d;
    return 0;
  }
  case WM_ERASEBKGND:
    return 1;
  case WM_NCHITTEST: {
    POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
    ScreenToClient(h, &pt);
    if (pt.y < d->titleH && !PtInRect(&d->close, pt))
      return HTCAPTION;
    return HTCLIENT;
  }
  case WM_MOUSEMOVE: {
    POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
    int nb = -1;
    for (auto &b : d->rects)
      if (PtInRect(&b.second, pt)) {
        nb = b.first;
        break;
      }
    int nc = PtInRect(&d->close, pt) ? 1 : 0;
    if (nb != d->hoverBtn || nc != d->hoverClose) {
      d->hoverBtn = nb;
      d->hoverClose = nc;
      InvalidateRect(h, nullptr, FALSE);
    }
    SetCursor((nb >= 0 || nc) ? LoadCursorW(nullptr, IDC_HAND)
                              : LoadCursorW(nullptr, IDC_ARROW));
    if (!d->tracking) {
      d->tracking = true;
      TRACKMOUSEEVENT t = {sizeof(t), TME_LEAVE, h, 0};
      TrackMouseEvent(&t);
    }
    return 0;
  }
  case WM_MOUSELEAVE:
    d->hoverBtn = -1;
    d->hoverClose = 0;
    d->tracking = false;
    InvalidateRect(h, nullptr, FALSE);
    return 0;
  case WM_LBUTTONUP: {
    POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
    if (PtInRect(&d->close, pt)) {
      d->result = d->buttons == UI_YESNO ? IDNO : IDCANCEL;
      d->done = true;
      DestroyWindow(h);
      return 0;
    }
    for (auto &b : d->rects)
      if (PtInRect(&b.second, pt)) {
        d->result = b.first;
        d->done = true;
        DestroyWindow(h);
        return 0;
      }
    return 0;
  }
  case WM_KEYDOWN:
    if (w == VK_ESCAPE) {
      d->result = d->buttons == UI_YESNO ? IDNO : IDCANCEL;
      d->done = true;
      DestroyWindow(h);
      return 0;
    }
    if (w == VK_RETURN) {
      for (auto &b : d->rects)
        if (IsPrimary(b.first)) {
          d->result = b.first;
          break;
        }
      d->done = true;
      DestroyWindow(h);
      return 0;
    }
    return 0;
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
    g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
    Gdiplus::SolidBrush bg(acry ? GfxAcrylicBg() : GdiCol(ThBg()));
    g.FillRectangle(&bg, 0, 0, rc.right, rc.bottom);
    Gdiplus::SolidBrush bb(GdiCol(ThBorder()));
    g.FillRectangle(&bb, 0, d->titleH, rc.right, 1);
    Gdiplus::Pen borderPen(GdiCol(ThBorder()));
    g.DrawRectangle(&borderPen, 0, 0, rc.right - 1, rc.bottom - 1);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    Gdiplus::Font ft(mem, ThFont(9));
    Gdiplus::SolidBrush tt(GdiCol(ThText()));
    static Gdiplus::StringFormat sfC;
    sfC.SetAlignment(Gdiplus::StringAlignmentCenter);
    sfC.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    HICON ic = LoadAppIconId(GfxUiIconId(), 16);
    if (ic) {
      DrawIconEx(mem, 10, (d->titleH - 16) / 2, ic, 16, 16, 0, nullptr,
                 DI_NORMAL);
      DestroyIcon(ic);
    }
    g.DrawString(
        d->caption.c_str(), -1, &ft,
        Gdiplus::RectF(34.0f, 0.0f, (REAL)(rc.right - 68), (REAL)d->titleH),
        &sfC, &tt);
    DrawCloseX(g, d->close, d->hoverClose != 0, ThText());

    if (d->icon != UI_ICON_NONE) {
      DrawDialogIcon(g, d->icon, (REAL)d->iconX, (REAL)d->iconY,
                     (REAL)d->iconSize);
    }

    Gdiplus::Font f10(mem, ThFont(10));
    Gdiplus::StringFormat sfText;
    sfText.SetAlignment(Gdiplus::StringAlignmentNear);
    sfText.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    g.DrawString(d->text.c_str(), -1, &f10,
                 Gdiplus::RectF((REAL)d->textX, (REAL)d->textY, (REAL)d->textW,
                                (REAL)d->textH),
                 &sfText, &tt);

    for (auto &b : d->rects) {
      int state = d->hoverBtn == b.first ? 1 : 0;
      GfxThemedButton(g, mem, b.second, state, IsPrimary(b.first));
      Gdiplus::SolidBrush lb(
          GdiCol(IsPrimary(b.first) ? RGB(255, 255, 255) : ThText()));
      g.DrawString(BtnLabel(b.first), -1, &ft,
                   Gdiplus::RectF((REAL)b.second.left, (REAL)b.second.top,
                                  (REAL)(b.second.right - b.second.left),
                                  (REAL)(b.second.bottom - b.second.top)),
                   &sfC, &lb);
    }
    if (!d->rects.empty()) {
      g.FillRectangle(&bb, 0, d->rects[0].second.top, rc.right, 1);
      if (d->rects.size() == 2) {
        int mid = d->dlgW / 2;
        g.FillRectangle(&bb, mid, d->rects[0].second.top, 1,
                        rc.bottom - d->rects[0].second.top);
      }
    }

    BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteObject(bm);
    DeleteDC(mem);
    EndPaint(h, &ps);
    return 0;
  }
  case WM_DESTROY:
    if (d && d->owner) {
      EnableWindow(d->owner, TRUE);
      SetForegroundWindow(d->owner);
    }
    return 0;
  }
  return DefWindowProcW(h, m, w, l);
}

static void MsgClass() {
  static bool reg = false;
  if (reg)
    return;
  WNDCLASSW wc = {};
  wc.hInstance = g_hInst;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.lpfnWndProc = MsgProc;
  wc.lpszClassName = L"OntyTaskMsg";
  RegisterClassW(&wc);
  reg = true;
}

int UiMessage(HWND owner, const wchar_t *text, const wchar_t *caption,
              int buttons, int icon) {
  MsgClass();
  if (!owner)
    owner = g_hwnd;
  UiMsg d;
  d.text = text ? text : L"";
  d.caption = caption ? caption : L"";
  d.buttons = buttons;
  d.icon = icon;
  d.owner = owner;
  d.result = buttons == UI_OK ? IDOK : IDCANCEL;
  g_msg = &d;

  int dpi = GfxDpi();
  auto Scale = [dpi](int px) { return MulDiv(px, dpi, 96); };

  int dlgW = Scale(380);
  int titleH = Scale(32);
  int padX = Scale(20);
  int padY = Scale(20);
  int iconSize = Scale(32);

  d.dlgW = dlgW;
  d.titleH = titleH;
  d.close = {dlgW - titleH, 0, dlgW, titleH};

  int contentY = titleH + padY;
  int textX = padX;
  int textW = dlgW - padX * 2;

  if (icon != UI_ICON_NONE) {
    textX = padX + iconSize + Scale(14);
    textW = dlgW - textX - padX;
  }

  HDC m2 = CreateCompatibleDC(nullptr);
  HFONT f10 = ThFont(10);
  HGDIOBJ o2 = SelectObject(m2, f10);
  RECT t = {0, 0, textW, 0};
  DrawTextW(m2, d.text.c_str(), -1, &t,
            DT_CALCRECT | DT_WORDBREAK | DT_EDITCONTROL);
  SelectObject(m2, o2);
  DeleteDC(m2);

  int textH = t.bottom;
  if (textH < Scale(18))
    textH = Scale(18);
  int contentH = (icon != UI_ICON_NONE) ? (std::max)(iconSize, textH) : textH;

  if (icon != UI_ICON_NONE) {
    d.iconX = padX;
    d.iconY = contentY + (contentH - iconSize) / 2;
    d.iconSize = iconSize;
  }
  d.textX = textX;
  d.textY = contentY + (contentH - textH) / 2;
  d.textW = textW;
  d.textH = textH;

  int btnH = Scale(34);
  int gapAboveButtons = Scale(20);

  int btnY = contentY + contentH + gapAboveButtons;
  int dlgH = btnY + btnH;
  d.dlgH = dlgH;

  d.rects.clear();
  if (buttons == UI_OK) {
    d.rects.push_back({IDOK, {0, btnY, dlgW, dlgH}});
  } else {
    int mid = dlgW / 2;
    if (buttons == UI_YESNO) {
      d.rects.push_back({IDNO, {0, btnY, mid, dlgH}});
      d.rects.push_back({IDYES, {mid, btnY, dlgW, dlgH}});
    } else {
      d.rects.push_back({IDCANCEL, {0, btnY, mid, dlgH}});
      d.rects.push_back({IDOK, {mid, btnY, dlgW, dlgH}});
    }
  }

  RECT rw;
  GetWindowRect(owner, &rw);
  int x = rw.left + ((rw.right - rw.left) - dlgW) / 2;
  int y = rw.top + ((rw.bottom - rw.top) - dlgH) / 2;
  EnableWindow(owner, FALSE);
  HWND h = CreateWindowExW(WS_EX_TOOLWINDOW, L"OntyTaskMsg", L"", WS_POPUP, x,
                           y, dlgW, dlgH, owner, nullptr, g_hInst, &d);
  ApplyWindowMaterial(h);
  TryRoundCorners(h);
  GfxRoundRegion(h);
  ShowWindow(h, SW_SHOW);
  SetForegroundWindow(h);
  MSG m;
  while (!d.done && IsWindow(h) && GetMessageW(&m, nullptr, 0, 0) > 0) {
    TranslateMessage(&m);
    DispatchMessageW(&m);
  }
  g_msg = nullptr;
  return d.result;
}

struct UiHk {
  HWND h = nullptr;
  std::wstring title;
  int mods = 0, vk = 0, liveMods = 0;
  bool got = false, done = false, ok = false, tracking = false;
  int hoverClose = 0;
  int titleH = 32;
  RECT close = {};
  HWND owner = nullptr;
};

static UiHk *g_hk = nullptr;
static HHOOK g_hkHook = nullptr;

static bool IsModVkH(int vk) {
  return vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
         vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
         vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU || vk == VK_LWIN ||
         vk == VK_RWIN;
}

static LRESULT CALLBACK HkHookProc(int code, WPARAM w, LPARAM l) {
  if (code == HC_ACTION && g_hk && g_hk->h) {
    KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)l;
    bool up = (p->flags & LLKHF_UP) != 0;
    int vk = (int)p->vkCode;
    if (!up) {
      PostMessageW(g_hk->h, WM_APP + 42, 0, 0);
      if (vk == VK_ESCAPE) {
        PostMessageW(g_hk->h, WM_APP + 40, 0, 0);
        return 1;
      }
      if (!IsModVkH(vk)) {
        int m = 0;
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
          m |= HK_CTRL;
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
          m |= HK_SHIFT;
        if (GetAsyncKeyState(VK_MENU) & 0x8000)
          m |= HK_ALT;
        if ((GetAsyncKeyState(VK_LWIN) & 0x8000) ||
            (GetAsyncKeyState(VK_RWIN) & 0x8000))
          m |= HK_WIN;
        PostMessageW(g_hk->h, WM_APP + 41, (WPARAM)m, (LPARAM)vk);
        return 1;
      }
    } else {
      PostMessageW(g_hk->h, WM_APP + 42, 0, 0);
    }
  }
  return CallNextHookEx(nullptr, code, w, l);
}

static LRESULT CALLBACK HkProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  UiHk *d = g_hk;
  switch (m) {
  case WM_CREATE: {
    d = (UiHk *)((CREATESTRUCTW *)l)->lpCreateParams;
    g_hk = d;
    d->h = h;
    RECT rc;
    GetClientRect(h, &rc);
    d->close = {rc.right - d->titleH, 0, rc.right, d->titleH};
    g_hkHook = SetWindowsHookExW(WH_KEYBOARD_LL, HkHookProc, g_hInst, 0);
    return 0;
  }
  case WM_APP + 40:
    d->done = true;
    DestroyWindow(h);
    return 0;
  case WM_APP + 42: {
    int lm = 0;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
      lm |= HK_CTRL;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
      lm |= HK_SHIFT;
    if (GetAsyncKeyState(VK_MENU) & 0x8000)
      lm |= HK_ALT;
    if ((GetAsyncKeyState(VK_LWIN) & 0x8000) ||
        (GetAsyncKeyState(VK_RWIN) & 0x8000))
      lm |= HK_WIN;
    if (lm != d->liveMods) {
      d->liveMods = lm;
      InvalidateRect(h, nullptr, FALSE);
    }
    return 0;
  }
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN: {
    int vk = (int)w;
    if (vk == VK_ESCAPE) {
      d->done = true;
      DestroyWindow(h);
      return 0;
    }
    if (!IsModVkH(vk) && !d->got) {
      int m = 0;
      if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        m |= HK_CTRL;
      if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
        m |= HK_SHIFT;
      if (GetAsyncKeyState(VK_MENU) & 0x8000)
        m |= HK_ALT;
      if ((GetAsyncKeyState(VK_LWIN) & 0x8000) ||
          (GetAsyncKeyState(VK_RWIN) & 0x8000))
        m |= HK_WIN;
      d->mods = m;
      d->vk = vk;
      d->got = true;
      d->ok = true;
      d->done = true;
      DestroyWindow(h);
    }
    return 0;
  }
  case WM_APP + 41:
    d->mods = (int)w;
    d->vk = (int)l;
    d->got = true;
    d->ok = true;
    d->done = true;
    DestroyWindow(h);
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_NCHITTEST: {
    POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
    ScreenToClient(h, &pt);
    if (pt.y < d->titleH && !PtInRect(&d->close, pt))
      return HTCAPTION;
    return HTCLIENT;
  }
  case WM_MOUSEMOVE: {
    POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
    int nc = PtInRect(&d->close, pt) ? 1 : 0;
    if (nc != d->hoverClose) {
      d->hoverClose = nc;
      InvalidateRect(h, nullptr, FALSE);
    }
    SetCursor(nc ? LoadCursorW(nullptr, IDC_HAND)
                 : LoadCursorW(nullptr, IDC_ARROW));
    if (!d->tracking) {
      d->tracking = true;
      TRACKMOUSEEVENT t = {sizeof(t), TME_LEAVE, h, 0};
      TrackMouseEvent(&t);
    }
    return 0;
  }
  case WM_MOUSELEAVE:
    d->tracking = false;
    if (d->hoverClose) {
      d->hoverClose = 0;
      InvalidateRect(h, nullptr, FALSE);
    }
    return 0;
  case WM_LBUTTONUP: {
    POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
    if (PtInRect(&d->close, pt)) {
      d->done = true;
      DestroyWindow(h);
      return 0;
    }
    return 0;
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
    g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    Gdiplus::SolidBrush bg(acry ? GfxAcrylicBg() : GdiCol(ThBg()));
    g.FillRectangle(&bg, 0, 0, rc.right, rc.bottom);
    Gdiplus::SolidBrush bb(GdiCol(ThBorder()));
    g.FillRectangle(&bb, 0, d->titleH, rc.right, 1);
    Gdiplus::Pen borderPen(GdiCol(ThBorder()));
    g.DrawRectangle(&borderPen, 0, 0, rc.right - 1, rc.bottom - 1);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
    Gdiplus::Font ft(mem, ThFont(9));
    Gdiplus::SolidBrush tt(GdiCol(ThText()));
    Gdiplus::SolidBrush dim(GdiCol(ThDim()));
    static Gdiplus::StringFormat sfC;
    sfC.SetAlignment(Gdiplus::StringAlignmentCenter);
    sfC.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    HICON ic = LoadAppIconId(GfxUiIconId(), 16);
    if (ic) {
      DrawIconEx(mem, 10, (d->titleH - 16) / 2, ic, 16, 16, 0, nullptr,
                 DI_NORMAL);
      DestroyIcon(ic);
    }
    g.DrawString(
        d->title.c_str(), -1, &ft,
        Gdiplus::RectF(34.0f, 0.0f, (REAL)(rc.right - 68), (REAL)d->titleH),
        &sfC, &tt);
    DrawCloseX(g, d->close, d->hoverClose != 0, ThText());

    int dpi = GfxDpi();
    auto Scale = [dpi](int px) { return MulDiv(px, dpi, 96); };
    g.DrawString(L(L_HK_PRESS), -1, &ft,
                 Gdiplus::RectF(0, (REAL)(d->titleH + Scale(12)),
                                (REAL)rc.right, (REAL)Scale(20)),
                 &sfC, &dim);
    Gdiplus::Font fb(mem, ThFont(15, true));
    std::wstring cur = d->vk
                           ? FmtHk(d->mods, d->vk)
                           : (d->liveMods ? FmtHk(d->liveMods, 0) : L"\u2026");
    g.DrawString(cur.c_str(), -1, &fb,
                 Gdiplus::RectF(0, (REAL)(d->titleH + Scale(34)),
                                (REAL)rc.right, (REAL)Scale(36)),
                 &sfC,
                 (d->vk || d->liveMods) ? (Gdiplus::Brush *)&tt
                                        : (Gdiplus::Brush *)&dim);
    BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteObject(bm);
    DeleteDC(mem);
    EndPaint(h, &ps);
    return 0;
  }
  case WM_DESTROY:
    if (g_hkHook) {
      UnhookWindowsHookEx(g_hkHook);
      g_hkHook = nullptr;
    }
    if (d && d->owner) {
      EnableWindow(d->owner, TRUE);
      SetForegroundWindow(d->owner);
    }
    return 0;
  }
  return DefWindowProcW(h, m, w, l);
}

bool UiHotkeyCapture(HWND owner, bool play, int &modsOut, int &vkOut) {
  static bool reg = false;
  if (!reg) {
    WNDCLASSW wc = {};
    wc.hInstance = g_hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpfnWndProc = HkProc;
    wc.lpszClassName = L"OntyTaskHk";
    RegisterClassW(&wc);
    reg = true;
  }
  if (!owner)
    owner = g_hwnd;
  UiHk d;
  d.title = play ? L(L_HK_PLAY) : L(L_HK_REC);
  d.owner = owner;
  g_hk = &d;
  int dpi = GfxDpi();
  auto Scale = [dpi](int px) { return MulDiv(px, dpi, 96); };
  int W = Scale(340), H = Scale(120);
  d.titleH = Scale(32);
  RECT rw;
  GetWindowRect(owner, &rw);
  int x = rw.left + ((rw.right - rw.left) - W) / 2;
  int y = rw.top + ((rw.bottom - rw.top) - H) / 2;
  EnableWindow(owner, FALSE);
  HWND h = CreateWindowExW(WS_EX_TOOLWINDOW, L"OntyTaskHk", L"", WS_POPUP, x, y,
                           W, H, owner, nullptr, g_hInst, &d);
  ApplyWindowMaterial(h);
  TryRoundCorners(h);
  GfxRoundRegion(h);
  ShowWindow(h, SW_SHOW);
  SetForegroundWindow(h);
  MSG m;
  while (!d.done && IsWindow(h) && GetMessageW(&m, nullptr, 0, 0) > 0) {
    TranslateMessage(&m);
    DispatchMessageW(&m);
  }
  g_hk = nullptr;
  if (d.ok) {
    modsOut = d.mods;
    vkOut = d.vk;
  }
  return d.ok;
}
