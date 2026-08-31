#pragma once
#include <windows.h>

enum { UI_OK, UI_OKCANCEL, UI_YESNO, UI_YESNOCANCEL };
enum { UI_ICON_NONE, UI_ICON_INFO, UI_ICON_WARN, UI_ICON_ERROR };

int UiMessage(HWND owner, const wchar_t *text, const wchar_t *caption,
              int buttons, int icon);
bool UiHotkeyCapture(HWND owner, bool play, int &modsOut, int &vkOut);
