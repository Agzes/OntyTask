#include "ui/gfx.h"
#include "core/config.h"
#include "core/json.h"
#include <vector>

static ULONG_PTR g_token = 0;
static int g_dpi = 96;
static HFONT g_fonts[10];
static int g_fontPt[10];
static bool g_fontBold[10];
static int g_fontCount = 0;
static HFONT g_glyphs[8];
static int g_glyphPx[8];
static int g_glyphCount = 0;

struct ThemeDef {
  std::wstring file;
  std::wstring name;
  COLORREF bg = RGB(30, 30, 30);
  COLORREF bg2 = RGB(39, 39, 39);
  COLORREF text = RGB(214, 214, 214);
  COLORREF accent = RGB(96, 180, 224);
  COLORREF dim = RGB(139, 139, 139);
  COLORREF border = RGB(54, 54, 54);
  COLORREF hover = RGB(42, 42, 42);
  COLORREF hover2 = RGB(48, 48, 48);
  COLORREF record = RGB(232, 64, 54);
  COLORREF play = RGB(90, 184, 224);
  COLORREF close = RGB(196, 43, 28);
  COLORREF warn = RGB(224, 176, 64);
  bool hasDim = false;
  bool hasBorder = false;
  bool hasHover = false;
  bool hasHover2 = false;
  bool hasRecord = false;
  bool hasPlay = false;
  bool hasClose = false;
  bool hasWarn = false;
  bool dark = true;
};

static std::vector<ThemeDef> g_customThemes;
static const ThemeDef *g_customTheme = nullptr;

static bool ParseHexColor(const std::wstring &s, COLORREF &out) {
  if (s.size() != 7 || s[0] != L'#')
    return false;
  wchar_t *end = nullptr;
  unsigned long v = wcstoul(s.c_str() + 1, &end, 16);
  if (!end || *end)
    return false;
  out = RGB((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
  return true;
}

static bool ParseThemeFile(const std::wstring &path, ThemeDef &t) {
  std::string bytes;
  if (!json::ReadTextFile(path.c_str(), bytes))
    return false;
  std::wstring w = json::ToWide(bytes);
  json::Js j{w.c_str(), w.c_str() + w.size()};
  if (!json::Eat(j, L'{'))
    return false;
  bool hasBg = false, hasText = false;
  while (true) {
    json::Ws(j);
    if (json::Eat(j, L'}'))
      break;
    std::wstring key;
    if (!json::Str(j, key))
      return false;
    if (!json::Eat(j, L':'))
      return false;
    json::Ws(j);
    if (j.p < j.e && *j.p == L'"') {
      std::wstring v;
      if (!json::Str(j, v))
        return false;
      if (key == L"name")
        t.name = v;
      else if (key == L"bg")
        hasBg = ParseHexColor(v, t.bg);
      else if (key == L"bg2")
        ParseHexColor(v, t.bg2);
      else if (key == L"text")
        hasText = ParseHexColor(v, t.text);
      else if (key == L"accent")
        ParseHexColor(v, t.accent);
      else if (key == L"dim")
        t.hasDim = ParseHexColor(v, t.dim);
      else if (key == L"border")
        t.hasBorder = ParseHexColor(v, t.border);
      else if (key == L"hover")
        t.hasHover = ParseHexColor(v, t.hover);
      else if (key == L"hover2")
        t.hasHover2 = ParseHexColor(v, t.hover2);
      else if (key == L"record")
        t.hasRecord = ParseHexColor(v, t.record);
      else if (key == L"play")
        t.hasPlay = ParseHexColor(v, t.play);
      else if (key == L"close")
        t.hasClose = ParseHexColor(v, t.close);
      else if (key == L"warn")
        t.hasWarn = ParseHexColor(v, t.warn);
    } else if (j.p < j.e && (*j.p == L't' || *j.p == L'f')) {
      bool b = false;
      if (json::Bool(j, b) && key == L"dark")
        t.dark = b;
    } else {
      json::SkipVal(j);
    }
    json::Ws(j);
    json::Eat(j, L',');
  }
  if (!hasBg || !hasText)
    return false;
  if (t.name.empty())
    t.name = t.file;
  return true;
}

static void LoadThemeFiles() {
  g_customThemes.clear();
  g_customTheme = nullptr;
  wchar_t exe[MAX_PATH];
  GetModuleFileNameW(nullptr, exe, MAX_PATH);
  wchar_t *slash = wcsrchr(exe, L'\\');
  if (!slash)
    return;
  *(slash + 1) = 0;
  std::wstring dir = std::wstring(exe) + L"themes";
  std::wstring pat = dir + L"\\*.json";
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW(pat.c_str(), &fd);
  if (h == INVALID_HANDLE_VALUE)
    return;
  do {
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      continue;
    ThemeDef t;
    t.file = fd.cFileName;
    if (ParseThemeFile(dir + L"\\" + fd.cFileName, t))
      g_customThemes.push_back(t);
  } while (FindNextFileW(h, &fd));
  FindClose(h);
  if (!g_cfg.themeFile.empty()) {
    for (const ThemeDef &t : g_customThemes) {
      if (t.file == g_cfg.themeFile) {
        g_customTheme = &t;
        break;
      }
    }
  }
}

int GfxCustomThemeCount() { return (int)g_customThemes.size(); }

const wchar_t *GfxCustomThemeName(int index) {
  if (index < 0 || index >= (int)g_customThemes.size())
    return L"";
  return g_customThemes[index].name.c_str();
}

const wchar_t *GfxCustomThemeFile(int index) {
  if (index < 0 || index >= (int)g_customThemes.size())
    return L"";
  return g_customThemes[index].file.c_str();
}

void GfxSetThemeFile(const wchar_t *file) {
  g_customTheme = nullptr;
  if (!file || !*file)
    return;
  for (const ThemeDef &t : g_customThemes) {
    if (t.file == file) {
      g_customTheme = &t;
      break;
    }
  }
}

void GfxReloadThemes() { LoadThemeFiles(); }

void GfxInit() {
  Gdiplus::GdiplusStartupInput si;
  Gdiplus::GdiplusStartup(&g_token, &si, nullptr);
  HDC dc = GetDC(nullptr);
  g_dpi = GetDeviceCaps(dc, LOGPIXELSY);
  ReleaseDC(nullptr, dc);
  LoadThemeFiles();
}

void GfxShutdown() {}

int GfxDpi() { return g_dpi; }

static COLORREF Blend(COLORREF a, COLORREF b, float t) {
  int r = GetRValue(a) + (int)((GetRValue(b) - GetRValue(a)) * t);
  int gg = GetGValue(a) + (int)((GetGValue(b) - GetGValue(a)) * t);
  int bb = GetBValue(a) + (int)((GetBValue(b) - GetBValue(a)) * t);
  return RGB(r, gg, bb);
}

static const COLORREF *Palette() {
  static const COLORREF pal[3][3] = {{0x1E1E1E, 0x272727, 0xD6D6D6},
                                     {0xF8F9FA, 0xFFFFFF, 0x15181C},
                                     {0x181818, 0x222222, 0xEDEDED}};
  if (g_customTheme) {
    static COLORREF cpal[3];
    cpal[0] = g_customTheme->bg;
    cpal[1] = g_customTheme->bg2;
    cpal[2] = g_customTheme->text;
    return cpal;
  }
  int t = GfxTheme();
  if (t < 0 || t > 2)
    t = 0;
  return pal[t];
}

COLORREF ThBg() { return Palette()[0]; }
COLORREF ThBg2() { return Palette()[1]; }
COLORREF ThText() { return Palette()[2]; }
COLORREF ThDim() {
  if (g_customTheme && g_customTheme->hasDim)
    return g_customTheme->dim;
  return Blend(ThText(), ThBg(), GfxTheme() == THEME_LIGHT ? 0.38f : 0.50f);
}
COLORREF ThBorder() {
  if (g_customTheme && g_customTheme->hasBorder)
    return g_customTheme->border;
  return Blend(ThBg(), ThText(), GfxTheme() == THEME_LIGHT ? 0.18f : 0.12f);
}
COLORREF ThHover() {
  if (g_customTheme && g_customTheme->hasHover)
    return g_customTheme->hover;
  return Blend(ThBg(), ThText(), GfxTheme() == THEME_LIGHT ? 0.08f : 0.06f);
}
COLORREF ThHover2() {
  if (g_customTheme && g_customTheme->hasHover2)
    return g_customTheme->hover2;
  return Blend(ThBg(), ThText(), GfxTheme() == THEME_LIGHT ? 0.14f : 0.11f);
}
COLORREF ThAccent() {
  if (g_cfg.accent >= 0)
    return (COLORREF)g_cfg.accent;
  if (g_customTheme)
    return g_customTheme->accent;
  return GfxTheme() == THEME_LIGHT ? RGB(43, 127, 181) : RGB(96, 180, 224);
}
COLORREF ThRecord() {
  if (g_customTheme)
    return g_customTheme->record;
  return RGB(232, 64, 54);
}
COLORREF ThPlay() {
  if (g_customTheme)
    return g_customTheme->play;
  return RGB(90, 184, 224);
}
COLORREF ThClose() {
  if (g_customTheme)
    return g_customTheme->close;
  return RGB(196, 43, 28);
}
COLORREF ThWarn() {
  if (g_customTheme)
    return g_customTheme->warn;
  return RGB(224, 176, 64);
}

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
               wchar_t code, COLORREF col, int alpha) {
  REAL dx = 0, dy = 0;
  GlyphInkFix(hdc, g, code, px, &dx, &dy);
  Gdiplus::StringFormat sf(Gdiplus::StringFormat::GenericTypographic());
  sf.SetAlignment(Gdiplus::StringAlignmentCenter);
  sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
  Gdiplus::Font f(hdc, GlyphFont(px));
  Gdiplus::SolidBrush b(GdiCol(col, alpha));
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
    COLORREF ac = ThAccent();
    fill = GdiCol(Blend(ac, RGB(0, 0, 0), state == 1 ? 0.15f : 0.30f));
    border = GdiCol(ac);
  } else if (GfxTheme() == THEME_ACRYLIC) {
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

int GfxTheme() {
  if (g_customTheme)
    return g_customTheme->dark ? THEME_DARK : THEME_LIGHT;
  if (g_cfg.theme < 0 || g_cfg.theme > 2)
    return THEME_DARK;
  return g_cfg.theme;
}

int GfxMenuTheme() {
  bool dark = (GfxTheme() != THEME_LIGHT);
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
  bool light = GfxTheme() == THEME_LIGHT;
  bool acry = GfxTheme() == THEME_ACRYLIC;
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
  Gdiplus::Matrix mt;
  g.GetTransform(&mt);
  REAL me[6] = {1, 0, 0, 1, 0, 0};
  mt.GetElements(me);
  if (me[0] > 1.001f) {
    Gdiplus::GraphicsState st = g.Save();
    g.ResetTransform();
    int sz = MulDiv(16, g_cfg.uiScale, 100);
    int px = MulDiv(7, g_cfg.uiScale, 100);
    HICON ic = LoadAppIconId(GfxUiIconId(), 32);
    if (ic) {
      Gdiplus::Bitmap bmp(ic);
      g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
      g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
      g.DrawImage(&bmp, (REAL)px, (REAL)px, (REAL)sz, (REAL)sz);
      DestroyIcon(ic);
    }
    g.Restore(st);
  } else {
    HICON ic = LoadAppIconId(GfxUiIconId(), 16);
    if (ic) {
      Gdiplus::Bitmap bmp(ic);
      if (bmp.GetLastStatus() == Gdiplus::Ok)
        g.DrawImage(&bmp, 7.0f, 7.0f, 16.0f, 16.0f);
      DestroyIcon(ic);
    }
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
    Gdiplus::SolidBrush red(GdiCol(ThClose()));
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
      g_cfg.uiIcon == 2 || (g_cfg.uiIcon == 0 && GfxTheme() == THEME_LIGHT);
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
  if (GfxTheme() == THEME_ACRYLIC) {
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
