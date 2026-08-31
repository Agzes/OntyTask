#include "ui/gfx.h"
#include "core/config.h"

static ULONG_PTR g_token = 0;
static int g_dpi = 96;
static HFONT g_fonts[10];
static int g_fontPt[10];
static bool g_fontBold[10];
static int g_fontCount = 0;
static HFONT g_glyphs[8];
static int g_glyphPx[8];
static int g_glyphCount = 0;

void GfxInit() {
  Gdiplus::GdiplusStartupInput si;
  Gdiplus::GdiplusStartup(&g_token, &si, nullptr);
  HDC dc = GetDC(nullptr);
  g_dpi = GetDeviceCaps(dc, LOGPIXELSY);
  ReleaseDC(nullptr, dc);
}

void GfxShutdown() { Gdiplus::GdiplusShutdown(g_token); }

int GfxDpi() { return g_dpi; }

static COLORREF Blend(COLORREF a, COLORREF b, float t) {
  int r = GetRValue(a) + (int)((GetRValue(b) - GetRValue(a)) * t);
  int gg = GetGValue(a) + (int)((GetGValue(b) - GetGValue(a)) * t);
  int bb = GetBValue(a) + (int)((GetBValue(b) - GetBValue(a)) * t);
  return RGB(r, gg, bb);
}

static const COLORREF *Palette() {
  static const COLORREF pal[3][3] = {{0x1E1E1E, 0x272727, 0xD6D6D6},
                                     {0xF8F9FA, 0xFFFFFF, 0x1F2328},
                                     {0x181818, 0x222222, 0xEDEDED}};
  int t = g_cfg.theme;
  if (t < 0 || t > 2)
    t = 0;
  return pal[t];
}

COLORREF ThBg() { return Palette()[0]; }
COLORREF ThBg2() { return Palette()[1]; }
COLORREF ThText() { return Palette()[2]; }
COLORREF ThDim() { return Blend(ThText(), ThBg(), 0.50f); }
COLORREF ThBorder() { return Blend(ThBg(), ThText(), 0.12f); }
COLORREF ThHover() { return Blend(ThBg(), ThText(), 0.06f); }
COLORREF ThHover2() { return Blend(ThBg(), ThText(), 0.11f); }
COLORREF ThAccent() { return RGB(96, 180, 224); }

Gdiplus::Color GfxAcrylicBg() { return Gdiplus::Color(255, 0, 0, 0); }

Gdiplus::Color GdiCol(COLORREF c, int a) {
  return Gdiplus::Color((BYTE)a, GetRValue(c), GetGValue(c), GetBValue(c));
}

HFONT ThFont(int pt, bool bold) {
  for (int i = 0; i < g_fontCount; i++)
    if (g_fontPt[i] == pt && g_fontBold[i] == bold)
      return g_fonts[i];
  HFONT f = CreateFontW(
      -MulDiv(pt, g_dpi, 72), 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL, FALSE,
      FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
      CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
  if (g_fontCount < 10) {
    g_fonts[g_fontCount] = f;
    g_fontPt[g_fontCount] = pt;
    g_fontBold[g_fontCount] = bold;
    g_fontCount++;
  }
  return f;
}

HFONT GlyphFont(int px) {
  for (int i = 0; i < g_glyphCount; i++)
    if (g_glyphPx[i] == px)
      return g_glyphs[i];
  HFONT f =
      CreateFontW(-px, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                  DEFAULT_PITCH, L"Segoe MDL2 Assets");
  if (g_glyphCount < 8) {
    g_glyphs[g_glyphCount] = f;
    g_glyphPx[g_glyphCount] = px;
    g_glyphCount++;
  }
  return f;
}

void GfxRoundRect(Gdiplus::Graphics &g, const RECT &r, Gdiplus::Color fill,
                  REAL radius, Gdiplus::Color border, bool hasBorder) {
  Gdiplus::GraphicsPath p;
  REAL x = (REAL)r.left, y = (REAL)r.top;
  REAL w = (REAL)(r.right - r.left - 1), h = (REAL)(r.bottom - r.top - 1);
  REAL d = radius * 2;
  p.AddArc(x, y, d, d, 180, 90);
  p.AddArc(x + w - d, y, d, d, 270, 90);
  p.AddArc(x + w - d, y + h - d, d, d, 0, 90);
  p.AddArc(x, y + h - d, d, d, 90, 90);
  p.CloseFigure();
  Gdiplus::SolidBrush b(fill);
  g.FillPath(&b, &p);
  if (hasBorder) {
    Gdiplus::Pen pen(border);
    g.DrawPath(&pen, &p);
  }
}

struct InkFix {
  wchar_t code;
  int px;
  int hint;
  REAL dx, dy;
};
static InkFix g_inkFix[96];
static int g_inkCount = 0;

static void GlyphInkFix(HDC ref, Gdiplus::Graphics &target, wchar_t code,
                        int px, REAL *dx, REAL *dy) {
  int hint = (int)target.GetTextRenderingHint();
  for (int i = 0; i < g_inkCount; i++)
    if (g_inkFix[i].code == code && g_inkFix[i].px == px &&
        g_inkFix[i].hint == hint) {
      *dx = g_inkFix[i].dx;
      *dy = g_inkFix[i].dy;
      return;
    }
  REAL ox = 0, oy = 0;
  const int S = 96;
  BITMAPINFO bi = {};
  bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biWidth = S;
  bi.bmiHeader.biHeight = -S;
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;
  void *bits = nullptr;
  HDC dc = CreateCompatibleDC(ref);
  HBITMAP dib = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
  if (dib && bits) {
    HGDIOBJ ob = SelectObject(dc, dib);
    {
      Gdiplus::Graphics pg(dc);
      pg.SetSmoothingMode(target.GetSmoothingMode());
      pg.SetPixelOffsetMode(target.GetPixelOffsetMode());
      pg.SetTextRenderingHint(target.GetTextRenderingHint());
      pg.SetPageUnit(target.GetPageUnit());
      pg.Clear(Gdiplus::Color(0, 0, 0, 0));
      Gdiplus::Font f(dc, GlyphFont(px));
      Gdiplus::StringFormat psf(Gdiplus::StringFormat::GenericTypographic());
      psf.SetAlignment(Gdiplus::StringAlignmentCenter);
      psf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
      Gdiplus::SolidBrush w(Gdiplus::Color(255, 255, 255, 255));
      std::wstring s(1, code);
      REAL box = (REAL)px * 2.0f;
      pg.DrawString(s.c_str(), 1, &f,
                    Gdiplus::RectF((REAL)S / 2.0f - box / 2.0f,
                                   (REAL)S / 2.0f - box / 2.0f, box, box),
                    &psf, &w);
    }
    SelectObject(dc, ob);
    unsigned *p = (unsigned *)bits;
    int x0 = S, x1 = -1, y0 = S, y1 = -1;
    for (int y = 0; y < S; y++) {
      for (int x = 0; x < S; x++) {
        if (((p[y * S + x] >> 24) & 0xFF) > 8) {
          if (x < x0)
            x0 = x;
          if (x > x1)
            x1 = x;
          if (y < y0)
            y0 = y;
          if (y > y1)
            y1 = y;
        }
      }
    }
    if (x1 >= x0) {
      ox = (REAL)(x0 + x1 + 1.0f) / 2.0f - (REAL)S / 2.0f;
      oy = (REAL)(y0 + y1 + 1.0f) / 2.0f - (REAL)S / 2.0f;
    }
    DeleteObject(dib);
  }
  DeleteDC(dc);
  if (g_inkCount < 96) {
    InkFix fx;
    fx.code = code;
    fx.px = px;
    fx.hint = hint;
    fx.dx = ox;
    fx.dy = oy;
    g_inkFix[g_inkCount++] = fx;
  }
  *dx = ox;
  *dy = oy;
}

void DrawGlyph(HDC hdc, Gdiplus::Graphics &g, REAL cx, REAL cy, int px,
               wchar_t code, COLORREF col) {
  REAL dx = 0, dy = 0;
  GlyphInkFix(hdc, g, code, px, &dx, &dy);
  Gdiplus::StringFormat sf(Gdiplus::StringFormat::GenericTypographic());
  sf.SetAlignment(Gdiplus::StringAlignmentCenter);
  sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
  Gdiplus::Font f(hdc, GlyphFont(px));
  Gdiplus::SolidBrush b(GdiCol(col));
  std::wstring s(1, code);
  REAL box = (REAL)px * 2.0f;
  g.DrawString(
      s.c_str(), 1, &f,
      Gdiplus::RectF(cx - box / 2.0f - dx, cy - box / 2.0f - dy, box, box), &sf,
      &b);
}

void GfxThemedButton(Gdiplus::Graphics &g, HDC hdc, const RECT &r, int state,
                     bool primary) {
  Gdiplus::Color fill;
  Gdiplus::Color border;
  if (primary) {
    fill = state == 1 ? Gdiplus::Color(230, 20, 142, 224)
                      : Gdiplus::Color(200, 0, 122, 204);
    border = Gdiplus::Color(220, 0, 140, 230);
  } else if (g_cfg.theme == THEME_ACRYLIC) {
    fill = state == 2   ? Gdiplus::Color(120, 90, 90, 90)
           : state == 1 ? Gdiplus::Color(90, 70, 70, 70)
                        : Gdiplus::Color(60, 50, 50, 50);
    border = Gdiplus::Color(160, 56, 56, 56);
  } else {
    COLORREF c = state == 2 ? ThHover2() : state == 1 ? ThHover() : ThBg2();
    fill = GdiCol(c);
    border = GdiCol(ThBorder());
  }
  g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
  g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
  REAL x = (REAL)r.left, y = (REAL)r.top;
  REAL w = (REAL)(r.right - r.left), h = (REAL)(r.bottom - r.top);
  Gdiplus::SolidBrush fb(fill);
  g.FillRectangle(&fb, x, y, w, h);
  Gdiplus::SolidBrush bb(border);
  g.FillRectangle(&bb, x, y, w, 1.0f);
  g.FillRectangle(&bb, x, y, 1.0f, h);
  g.FillRectangle(&bb, x, y + h - 1.0f, w, 1.0f);
  g.FillRectangle(&bb, x + w - 1.0f, y, 1.0f, h);
}

RECT GfxCloseRect(int w) {
  RECT r = {w - 30, 0, w, 30};
  return r;
}
RECT GfxMinRect(int w) {
  RECT r = {w - 60, 0, w - 30, 30};
  return r;
}
RECT GfxPinRect(int w) {
  RECT r = {30, 0, 60, 30};
  return r;
}

static bool SysAppsDark() {
  HKEY k;
  if (RegOpenKeyExW(
          HKEY_CURRENT_USER,
          L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
          0, KEY_QUERY_VALUE, &k) != ERROR_SUCCESS)
    return false;
  DWORD v = 1, sz = 4;
  RegQueryValueExW(k, L"AppsUseLightTheme", nullptr, nullptr, (LPBYTE)&v, &sz);
  RegCloseKey(k);
  return v == 0;
}

int GfxMenuTheme() {
  bool dark = (g_cfg.theme != THEME_LIGHT);
  HMODULE ux = GetModuleHandleW(L"uxtheme.dll");
  if (!ux)
    return dark ? 2 : 1;
  typedef int(WINAPI * FnSpam)(int);
  typedef void(WINAPI * FnFlush)();
  FnSpam spam = (FnSpam)GetProcAddress(ux, (LPCSTR)135);
  FnFlush flush = (FnFlush)GetProcAddress(ux, (LPCSTR)136);
  if (!spam || !flush)
    return dark ? 2 : 1;
  spam(dark ? 2 : 0);
  flush();
  return dark ? 2 : 1;
}

void GfxChrome(Gdiplus::Graphics &g, HDC hdc, int width, const wchar_t *title,
               int hover, int buttons) {
  static Gdiplus::StringFormat sfC;
  sfC.SetAlignment(Gdiplus::StringAlignmentCenter);
  sfC.SetLineAlignment(Gdiplus::StringAlignmentCenter);
  bool light = g_cfg.theme == THEME_LIGHT;
  bool acry = g_cfg.theme == THEME_ACRYLIC;
  g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
  g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
  if (buttons & CHB_SEP) {
    Gdiplus::SolidBrush sep(acry ? Gdiplus::Color(45, 255, 255, 255)
                                 : GdiCol(ThBorder()));
    g.FillRectangle(&sep, 0.0f, 30.0f, (REAL)width, 1.0f);
  }
  if (hover == 4) {
    Gdiplus::SolidBrush hb(light ? GdiCol(ThHover())
                                 : Gdiplus::Color(30, 255, 255, 255));
    g.FillRectangle(&hb, 0.0f, 0.0f, 30.0f, 30.0f);
  }
  HICON ic = LoadAppIconId(GfxUiIconId(), 16);
  if (ic) {
    DrawIconEx(hdc, 7, 7, ic, 16, 16, 0, nullptr, DI_NORMAL);
    DestroyIcon(ic);
  }
  g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
  g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
  Gdiplus::Font f(hdc, ThFont(9));
  Gdiplus::SolidBrush tb(GdiCol(ThText()));
  g.DrawString(title, -1, &f,
               Gdiplus::RectF(60.0f, 0.0f, (REAL)(width - 120), 30.0f), &sfC,
               &tb);
  COLORREF glyph = light ? ThText() : RGB(255, 255, 255);
  if (buttons & CHB_PIN) {
    RECT rp = GfxPinRect(width);
    if (hover == 3) {
      Gdiplus::SolidBrush hb(light ? GdiCol(ThHover())
                                   : Gdiplus::Color(30, 255, 255, 255));
      g.FillRectangle(&hb, (REAL)rp.left, (REAL)rp.top,
                      (REAL)(rp.right - rp.left), (REAL)(rp.bottom - rp.top));
    }
    DrawGlyph(hdc, g, (REAL)(rp.left + rp.right) / 2.0f,
              (REAL)(rp.top + rp.bottom) / 2.0f, 12,
              g_cfg.topmost ? (wchar_t)0xE840 : (wchar_t)0xE718,
              g_cfg.topmost ? ThAccent() : ThDim());
  }
  if (buttons & CHB_MIN) {
    RECT rm = GfxMinRect(width);
    if (hover == 1) {
      Gdiplus::SolidBrush hb(light ? GdiCol(ThHover())
                                   : Gdiplus::Color(30, 255, 255, 255));
      g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
      g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
      g.FillRectangle(&hb, (REAL)rm.left, (REAL)rm.top,
                      (REAL)(rm.right - rm.left), (REAL)(rm.bottom - rm.top));
      g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
      g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    }
    REAL mx = (REAL)(rm.left + rm.right) / 2.0f;
    REAL my = (REAL)(rm.top + rm.bottom) / 2.0f;
    Gdiplus::GraphicsState mst = g.Save();
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    Gdiplus::Pen mp(GdiCol(glyph), 1.0f);
    g.DrawLine(&mp, mx - 5.0f, my, mx + 5.0f, my);
    g.Restore(mst);
  }
  RECT rc = GfxCloseRect(width);
  if (hover == 2) {
    Gdiplus::SolidBrush red(Gdiplus::Color(255, 196, 43, 28));
    g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    g.FillRectangle(&red, (REAL)rc.left, (REAL)rc.top,
                    (REAL)(rc.right - rc.left), (REAL)(rc.bottom - rc.top));
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  }
  {
    REAL cx = (REAL)(rc.left + rc.right) / 2.0f;
    REAL cy = (REAL)(rc.top + rc.bottom) / 2.0f;
    REAL s = 4.5f;
    Gdiplus::Pen xp(GdiCol(hover == 2 ? RGB(255, 255, 255) : glyph), 1.0f);
    g.DrawLine(&xp, cx - s, cy - s, cx + s, cy + s);
    g.DrawLine(&xp, cx + s, cy - s, cx - s, cy + s);
  }
}

HICON LoadAppIconId(int id, int size) {
  return (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(id), IMAGE_ICON, size,
                           size, LR_DEFAULTCOLOR);
}

HICON LoadAppIcon(int size) { return LoadAppIconId(1, size); }

int GfxUiIconId() {
  bool light =
      g_cfg.uiIcon == 2 || (g_cfg.uiIcon == 0 && g_cfg.theme == THEME_LIGHT);
  return light ? 4 : 2;
}

static HBITMAP g_bmpCache[128];
static wchar_t g_bmpKey[128];
static COLORREF g_bmpCol[128];
static int g_bmpCount = 0;

static HBITMAP NewAlphaBitmap(int &size) {
  size = MulDiv(16, g_dpi, 96);
  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = size;
  bmi.bmiHeader.biHeight = -size;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  void *bits = nullptr;
  HDC hdc = GetDC(nullptr);
  HBITMAP bmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
  ReleaseDC(nullptr, hdc);
  if (bmp && bits)
    memset(bits, 0, (size_t)size * size * 4);
  return bmp;
}

static HBITMAP CacheBitmap(wchar_t key, COLORREF col, HBITMAP bmp) {
  if (g_bmpCount < 128) {
    g_bmpCache[g_bmpCount] = bmp;
    g_bmpKey[g_bmpCount] = key;
    g_bmpCol[g_bmpCount] = col;
    g_bmpCount++;
  }
  return bmp;
}

HBITMAP GfxGlyphBitmap(wchar_t glyph, COLORREF col) {
  for (int i = 0; i < g_bmpCount; i++)
    if (g_bmpKey[i] == glyph && g_bmpCol[i] == col)
      return g_bmpCache[i];
  int size = 0;
  HBITMAP bmp = NewAlphaBitmap(size);
  if (!bmp)
    return nullptr;
  HDC mem = CreateCompatibleDC(nullptr);
  HGDIOBJ old = SelectObject(mem, bmp);
  Gdiplus::Graphics g(mem);
  g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
  g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
  DrawGlyph(mem, g, (REAL)size / 2.0f, (REAL)size / 2.0f,
            (int)((REAL)size * 0.85f), glyph, col);
  SelectObject(mem, old);
  DeleteDC(mem);
  return CacheBitmap(glyph, col, bmp);
}

HBITMAP GfxDotBitmap(COLORREF col) {
  for (int i = 0; i < g_bmpCount; i++)
    if (g_bmpKey[i] == 0 && g_bmpCol[i] == col)
      return g_bmpCache[i];
  int size = 0;
  HBITMAP bmp = NewAlphaBitmap(size);
  if (!bmp)
    return nullptr;
  HDC mem = CreateCompatibleDC(nullptr);
  HGDIOBJ old = SelectObject(mem, bmp);
  Gdiplus::Graphics g(mem);
  g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  Gdiplus::SolidBrush b(
      Gdiplus::Color(255, GetRValue(col), GetGValue(col), GetBValue(col)));
  REAL d = (REAL)size * 0.55f;
  g.FillEllipse(&b, ((REAL)size - d) / 2, ((REAL)size - d) / 2, d, d);
  SelectObject(mem, old);
  DeleteDC(mem);
  return CacheBitmap(0, col, bmp);
}

void GfxMenuIcon(HMENU m, UINT id, HBITMAP bmp) {
  if (!m || !bmp)
    return;
  MENUITEMINFOW mii = {sizeof(mii)};
  mii.fMask = MIIM_BITMAP;
  mii.hbmpItem = bmp;
  SetMenuItemInfoW(m, id, FALSE, &mii);
}

void GfxMenuGlyph(HMENU m, UINT id, wchar_t glyph, COLORREF col) {
  GfxMenuIcon(m, id, GfxGlyphBitmap(glyph, col));
}

typedef struct {
  int st;
  DWORD flags;
  DWORD color;
  DWORD anim;
} ACCPOL;
typedef struct {
  int attrib;
  ACCPOL *data;
  SIZE_T cb;
} WCAD;
typedef BOOL(WINAPI *FnSWCA)(HWND, WCAD *);

static void SetAccent(HWND h, int st, DWORD color) {
  static FnSWCA f = nullptr;
  if (!f) {
    f = (FnSWCA)GetProcAddress(GetModuleHandleW(L"user32.dll"),
                               "SetWindowCompositionAttribute");
    if (!f)
      return;
  }
  ACCPOL p;
  p.st = st;
  p.flags = 0;
  p.color = color;
  p.anim = 0;
  WCAD d;
  d.attrib = 19;
  d.data = &p;
  d.cb = sizeof(p);
  f(h, &d);
}

typedef struct {
  int l, t, r, b;
} GFXMARGINS;
typedef LONG(WINAPI *FnDwExt)(HWND, GFXMARGINS *);

static void ExtFrame(HWND h, int v) {
  HMODULE d = LoadLibraryW(L"dwmapi.dll");
  if (!d)
    return;
  FnDwExt f = (FnDwExt)GetProcAddress(d, "DwmExtendFrameIntoClientArea");
  if (f) {
    GFXMARGINS m = {v, v, v, v};
    f(h, &m);
  }
  FreeLibrary(d);
}

void ApplyWindowMaterial(HWND h) {
  if (g_cfg.theme == THEME_ACRYLIC) {
    SetAccent(h, 4, 0x8C141414);
    ExtFrame(h, -1);
  } else {
    SetAccent(h, 0, 0);
    ExtFrame(h, 0);
  }
}

void GfxRoundRegion(HWND h) {
  RECT rc;
  GetClientRect(h, &rc);
  int r = MulDiv(8, g_dpi, 96);
  HRGN rg = CreateRoundRectRgn(0, 0, rc.right + 1, rc.bottom + 1, r * 2, r * 2);
  SetWindowRgn(h, rg, TRUE);
}

void TryRoundCorners(HWND h) {
  HMODULE d = LoadLibraryW(L"dwmapi.dll");
  if (!d)
    return;
  typedef LONG(WINAPI * FnDwa)(HWND, DWORD, void *, DWORD);
  FnDwa f = (FnDwa)GetProcAddress(d, "DwmSetWindowAttribute");
  if (f) {
    int v = 2;
    f(h, 33, &v, 4);
  }
  FreeLibrary(d);
}
