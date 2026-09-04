#pragma once

enum { UR_NONE = 0, UR_CHECKING, UR_LATEST, UR_FOUND, UR_FAIL };

void Update_AutoCheck();
void Update_ManualCheck();
bool Update_Found();
int Update_State();

extern const wchar_t *URL_RELEASES;
extern const wchar_t *URL_GITHUB;
extern const wchar_t *URL_ISSUE;
extern const wchar_t *URL_LICENSE;
extern const wchar_t *APP_VERSION;
extern const wchar_t *APP_VERSION_RAW;
