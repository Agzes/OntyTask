#pragma once
#include <gdiplus.h>
#include <string>
#include <windows.h>

using Gdiplus::REAL;

enum { THEME_DARK = 0, THEME_LIGHT = 1, THEME_ACRYLIC = 2 };

void GfxInit();
void GfxShutdown();
int GfxDpi();
COLORREF ThBg();
COLORREF ThBg2();
COLORREF ThText();
COLORREF ThDim();
COLORREF ThBorder();
COLORREF ThHover();
COLORREF ThHover2();
COLORREF ThAccent();
Gdiplus::Color GdiCol(COLORREF c, int a = 255);
Gdiplus::Color GfxAcrylicBg();
HFONT ThFont(int pt, bool bold = false);
HFONT GlyphFont(int px);
void GfxRoundRect(Gdiplus::Graphics &g, const RECT &r, Gdiplus::Color fill,
                  REAL radius, Gdiplus::Color border, bool hasBorder);
void DrawGlyph(HDC hdc, Gdiplus::Graphics &g, Gdiplus::REAL cx,
               Gdiplus::REAL cy, int px, wchar_t code, COLORREF col);
void GfxThemedButton(Gdiplus::Graphics &g, HDC hdc, const RECT &r, int state,
                     bool primary = false);
enum { CHB_MIN = 1, CHB_PIN = 4, CHB_SEP = 8 };
RECT GfxCloseRect(int w);
RECT GfxMinRect(int w);
RECT GfxPinRect(int w);
void GfxChrome(Gdiplus::Graphics &g, HDC hdc, int width, const wchar_t *title,
               int hover, int buttons);
int GfxMenuTheme();
HBITMAP GfxGlyphBitmap(wchar_t glyph, COLORREF col);
HBITMAP GfxDotBitmap(COLORREF col);
void GfxMenuIcon(HMENU m, UINT id, HBITMAP bmp);
void GfxMenuGlyph(HMENU m, UINT id, wchar_t glyph, COLORREF col);
HICON LoadAppIconId(int id, int size);
int GfxUiIconId();
HICON LoadAppIcon(int size);
void ApplyWindowMaterial(HWND h);
void TryRoundCorners(HWND h);
void GfxRoundRegion(HWND h);
