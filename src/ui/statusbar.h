#pragma once
#include <windows.h>

void StatusShow(const wchar_t *msg, UINT ms);
void StatusPersistent(const wchar_t *msg);
void StatusUpdate(const wchar_t *msg);
void StatusHide();
bool StatusIsPersistent();
bool StatusVisible();
