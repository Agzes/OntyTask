#include "core/config.h"
#include <cwchar>
#include <shlobj.h>
#include <string>

Config g_cfg = {};
HWND g_hwnd = nullptr;
HINSTANCE g_hInst = nullptr;

static std::wstring IniPath() {
  wchar_t p[MAX_PATH];
  GetModuleFileNameW(nullptr, p, MAX_PATH);
  wchar_t *s = wcsrchr(p, L'\\');
  if (s)
    *(s + 1) = 0;
  lstrcatW(p, L"OntyTask.ini");
  return p;
}

std::vector<std::wstring> g_recentFiles;

void RecentAdd(const std::wstring &path) {
  if (path.empty())
    return;
  for (auto it = g_recentFiles.begin(); it != g_recentFiles.end(); ++it) {
    if (_wcsicmp(it->c_str(), path.c_str()) == 0) {
      g_recentFiles.erase(it);
      break;
    }
  }
  g_recentFiles.insert(g_recentFiles.begin(), path);
  if (g_recentFiles.size() > 8)
    g_recentFiles.resize(8);
  ConfigSave();
}

void RecentClear() {
  g_recentFiles.clear();
  ConfigSave();
}

static int GetSystemDefaultTheme() {
  DWORD val = 0;
  DWORD sz = sizeof(val);
  if (RegGetValueW(
          HKEY_CURRENT_USER,
          L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
          L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &val,
          &sz) == ERROR_SUCCESS) {
    return (val != 0) ? 1 : 0;
  }
  return 0;
}

void ConfigLoad() {
  std::wstring ini = IniPath();
  g_cfg.x = GetPrivateProfileIntW(L"OntyTask", L"window_x", -1, ini.c_str());
  g_cfg.y = GetPrivateProfileIntW(L"OntyTask", L"window_y", -1, ini.c_str());
  g_cfg.speed = GetPrivateProfileIntW(L"OntyTask", L"speed", 1, ini.c_str());
  wchar_t spCustBuf[32] = L"";
  GetPrivateProfileStringW(L"OntyTask", L"speed_custom_val", L"", spCustBuf, 32,
                           ini.c_str());
  if (spCustBuf[0]) {
    for (int i = 0; spCustBuf[i]; i++)
      if (spCustBuf[i] == L',')
        spCustBuf[i] = L'.';
    g_cfg.speedCustom = wcstod(spCustBuf, nullptr);
  } else {
    int oldVal =
        GetPrivateProfileIntW(L"OntyTask", L"speed_custom", 8, ini.c_str());
    g_cfg.speedCustom = (double)oldVal;
  }
  if (g_cfg.speedCustom < 0.01)
    g_cfg.speedCustom = 0.01;
  if (g_cfg.speedCustom > 1000.0)
    g_cfg.speedCustom = 1000.0;
  g_cfg.loops = GetPrivateProfileIntW(L"OntyTask", L"loops", 1, ini.c_str());
  g_cfg.continuous =
      GetPrivateProfileIntW(L"OntyTask", L"continuous", 0, ini.c_str());
  g_cfg.topmost =
      GetPrivateProfileIntW(L"OntyTask", L"topmost", 0, ini.c_str());
  int defTheme = GetSystemDefaultTheme();
  g_cfg.theme =
      GetPrivateProfileIntW(L"OntyTask", L"theme", defTheme, ini.c_str());
  g_cfg.lang = GetPrivateProfileIntW(L"OntyTask", L"lang", 1, ini.c_str());
  g_cfg.statusbarTop =
      GetPrivateProfileIntW(L"OntyTask", L"statusbar_top", 1, ini.c_str());
  g_cfg.statusbarShow =
      GetPrivateProfileIntW(L"OntyTask", L"statusbar_show", 1, ini.c_str());
  g_cfg.statusbarFollowCursor = GetPrivateProfileIntW(
      L"OntyTask", L"statusbar_follow_monitor", 1, ini.c_str());
  g_cfg.panicPause =
      GetPrivateProfileIntW(L"OntyTask", L"panic_pause", 1, ini.c_str());
  g_cfg.panicScroll =
      GetPrivateProfileIntW(L"OntyTask", L"panic_scroll", 1, ini.c_str());
  g_cfg.panicEsc =
      GetPrivateProfileIntW(L"OntyTask", L"panic_esc", 0, ini.c_str());
  g_cfg.panicMouseMove =
      GetPrivateProfileIntW(L"OntyTask", L"panic_mouse", 0, ini.c_str());
  g_cfg.autoupdate =
      GetPrivateProfileIntW(L"OntyTask", L"autoupdate", 1, ini.c_str());
  g_cfg.trayIcon =
      GetPrivateProfileIntW(L"OntyTask", L"tray_icon", 0, ini.c_str());
  if (g_cfg.trayIcon < 0 || g_cfg.trayIcon > 2)
    g_cfg.trayIcon = 0;
  g_cfg.uiIcon = GetPrivateProfileIntW(L"OntyTask", L"ui_icon", 0, ini.c_str());
  if (g_cfg.uiIcon < 0 || g_cfg.uiIcon > 2)
    g_cfg.uiIcon = 0;
  g_cfg.autoMinimize =
      GetPrivateProfileIntW(L"OntyTask", L"auto_minimize", 0, ini.c_str());
  if (g_cfg.speed < -5 || g_cfg.speed > 1000)
    g_cfg.speed = 1;
  if (g_cfg.loops < 1 || g_cfg.loops > 99999)
    g_cfg.loops = 1;
  if (g_cfg.theme < 0 || g_cfg.theme > 2)
    g_cfg.theme = defTheme;
  if (g_cfg.lang < 0 || g_cfg.lang > 2)
    g_cfg.lang = 1;
  g_cfg.recordMods =
      GetPrivateProfileIntW(L"OntyTask", L"record_mods", -1, ini.c_str());
  g_cfg.recordVk =
      GetPrivateProfileIntW(L"OntyTask", L"record_vk", -1, ini.c_str());
  g_cfg.playMods =
      GetPrivateProfileIntW(L"OntyTask", L"play_mods", -1, ini.c_str());
  g_cfg.playVk =
      GetPrivateProfileIntW(L"OntyTask", L"play_vk", -1, ini.c_str());
  if (g_cfg.recordMods < 0 || g_cfg.recordMods > 15 || g_cfg.recordVk < 0) {
    int k = GetPrivateProfileIntW(L"OntyTask", L"record_key", 0, ini.c_str());
    if (k < 0 || k > 3)
      k = 0;
    static const int mods[4] = {7, 0, 0, 0};
    static const int vk[4] = {0x52, VK_SNAPSHOT, VK_F8, VK_F12};
    g_cfg.recordMods = mods[k];
    g_cfg.recordVk = vk[k];
  }
  if (g_cfg.playMods < 0 || g_cfg.playMods > 15 || g_cfg.playVk < 0) {
    int k = GetPrivateProfileIntW(L"OntyTask", L"play_key", 0, ini.c_str());
    if (k < 0 || k > 3)
      k = 0;
    static const int mods[4] = {7, 0, 0, 0};
    static const int vk[4] = {0x50, VK_SNAPSHOT, VK_F8, VK_F12};
    g_cfg.playMods = mods[k];
    g_cfg.playVk = vk[k];
  }
  if (g_cfg.recordVk == g_cfg.playVk && g_cfg.recordMods == g_cfg.playMods) {
    g_cfg.playMods = 7;
    g_cfg.playVk = 0x50;
  }
  g_recentFiles.clear();
  for (int i = 0; i < 8; i++) {
    wchar_t rk[16], rv[MAX_PATH];
    swprintf_s(rk, L"recent_%d", i);
    GetPrivateProfileStringW(L"Recent", rk, L"", rv, MAX_PATH, ini.c_str());
    if (rv[0])
      g_recentFiles.push_back(rv);
  }
}

void ConfigSave() {
  std::wstring ini = IniPath();
  wchar_t b[32];
  auto w = [&](const wchar_t *k, int v) {
    swprintf_s(b, L"%d", v);
    WritePrivateProfileStringW(L"OntyTask", k, b, ini.c_str());
  };
  w(L"window_x", g_cfg.x);
  w(L"window_y", g_cfg.y);
  w(L"speed", g_cfg.speed);
  swprintf_s(b, L"%.4g", g_cfg.speedCustom);
  WritePrivateProfileStringW(L"OntyTask", L"speed_custom_val", b, ini.c_str());
  w(L"loops", g_cfg.loops);
  w(L"continuous", g_cfg.continuous);
  w(L"topmost", g_cfg.topmost);
  w(L"theme", g_cfg.theme);
  w(L"lang", g_cfg.lang);
  w(L"statusbar_top", g_cfg.statusbarTop);
  w(L"statusbar_show", g_cfg.statusbarShow);
  w(L"statusbar_follow_monitor", g_cfg.statusbarFollowCursor);
  w(L"panic_pause", g_cfg.panicPause);
  w(L"panic_scroll", g_cfg.panicScroll);
  w(L"panic_esc", g_cfg.panicEsc);
  w(L"panic_mouse", g_cfg.panicMouseMove);
  w(L"autoupdate", g_cfg.autoupdate);
  w(L"tray_icon", g_cfg.trayIcon);
  w(L"ui_icon", g_cfg.uiIcon);
  w(L"auto_minimize", g_cfg.autoMinimize);
  w(L"record_mods", g_cfg.recordMods);
  w(L"record_vk", g_cfg.recordVk);
  w(L"play_mods", g_cfg.playMods);
  w(L"play_vk", g_cfg.playVk);
  WritePrivateProfileStringW(L"OntyTask", L"record_key", nullptr, ini.c_str());
  WritePrivateProfileStringW(L"OntyTask", L"play_key", nullptr, ini.c_str());
  WritePrivateProfileSectionW(L"Recent", L"", ini.c_str());
  for (size_t i = 0; i < g_recentFiles.size(); i++) {
    wchar_t rk[16];
    swprintf_s(rk, L"recent_%zu", i);
    WritePrivateProfileStringW(L"Recent", rk, g_recentFiles[i].c_str(),
                               ini.c_str());
  }
}

bool IsFileAssociationRegistered() {
  HKEY k = nullptr;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\.onty", 0, KEY_READ,
                    &k) != ERROR_SUCCESS)
    return false;
  wchar_t val[128] = L"";
  DWORD sz = sizeof(val);
  LSTATUS st = RegQueryValueExW(k, nullptr, nullptr, nullptr, (LPBYTE)val, &sz);
  RegCloseKey(k);
  return (st == ERROR_SUCCESS && wcscmp(val, L"OntyTask.Macro") == 0);
}

bool RegisterFileAssociation(bool enable) {
  wchar_t exe[MAX_PATH];
  GetModuleFileNameW(nullptr, exe, MAX_PATH);
  const wchar_t *exeName = wcsrchr(exe, L'\\');
  exeName = exeName ? exeName + 1 : L"OntyTask.exe";

  if (enable) {
    HKEY kExt = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\.onty", 0,
                        nullptr, 0, KEY_WRITE, nullptr, &kExt,
                        nullptr) == ERROR_SUCCESS) {
      const wchar_t progId[] = L"OntyTask.Macro";
      RegSetValueExW(kExt, nullptr, 0, REG_SZ, (const BYTE *)progId,
                     (DWORD)((wcslen(progId) + 1) * sizeof(wchar_t)));
      RegCloseKey(kExt);
    }

    HKEY kOw = nullptr;
    if (RegCreateKeyExW(
            HKEY_CURRENT_USER, L"Software\\Classes\\.onty\\OpenWithProgids", 0,
            nullptr, 0, KEY_WRITE, nullptr, &kOw, nullptr) == ERROR_SUCCESS) {
      const wchar_t dummy[] = L"";
      RegSetValueExW(kOw, L"OntyTask.Macro", 0, REG_SZ, (const BYTE *)dummy,
                     (DWORD)(sizeof(wchar_t)));
      RegCloseKey(kOw);
    }

    HKEY kProg = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\OntyTask.Macro",
                        0, nullptr, 0, KEY_WRITE, nullptr, &kProg,
                        nullptr) == ERROR_SUCCESS) {
      const wchar_t desc[] = L"OntyTask Macro File";
      RegSetValueExW(kProg, nullptr, 0, REG_SZ, (const BYTE *)desc,
                     (DWORD)((wcslen(desc) + 1) * sizeof(wchar_t)));
      RegCloseKey(kProg);
    }

    HKEY kIcon = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
                        L"Software\\Classes\\OntyTask.Macro\\DefaultIcon", 0,
                        nullptr, 0, KEY_WRITE, nullptr, &kIcon,
                        nullptr) == ERROR_SUCCESS) {
      std::wstring iconStr = std::wstring(L"\"") + exe + L"\",0";
      RegSetValueExW(kIcon, nullptr, 0, REG_SZ, (const BYTE *)iconStr.c_str(),
                     (DWORD)((iconStr.size() + 1) * sizeof(wchar_t)));
      RegCloseKey(kIcon);
    }

    HKEY kCmd = nullptr;
    if (RegCreateKeyExW(
            HKEY_CURRENT_USER,
            L"Software\\Classes\\OntyTask.Macro\\shell\\open\\command", 0,
            nullptr, 0, KEY_WRITE, nullptr, &kCmd, nullptr) == ERROR_SUCCESS) {
      std::wstring cmdStr = std::wstring(L"\"") + exe + L"\" \"%1\"";
      RegSetValueExW(kCmd, nullptr, 0, REG_SZ, (const BYTE *)cmdStr.c_str(),
                     (DWORD)((cmdStr.size() + 1) * sizeof(wchar_t)));
      RegCloseKey(kCmd);
    }

    std::wstring appCmdKey =
        std::wstring(L"Software\\Classes\\Applications\\") + exeName +
        L"\\shell\\open\\command";
    HKEY kApp = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, appCmdKey.c_str(), 0, nullptr, 0,
                        KEY_WRITE, nullptr, &kApp, nullptr) == ERROR_SUCCESS) {
      std::wstring cmdStr = std::wstring(L"\"") + exe + L"\" \"%1\"";
      RegSetValueExW(kApp, nullptr, 0, REG_SZ, (const BYTE *)cmdStr.c_str(),
                     (DWORD)((cmdStr.size() + 1) * sizeof(wchar_t)));
      RegCloseKey(kApp);
    }
  } else {
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\.onty");
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\OntyTask.Macro");
    std::wstring appKey =
        std::wstring(L"Software\\Classes\\Applications\\") + exeName;
    RegDeleteTreeW(HKEY_CURRENT_USER, appKey.c_str());
  }
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
  return true;
}
