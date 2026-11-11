#include "lang/lang.h"
#include "core/config.h"
#include "core/json.h"
#include <map>

struct LangData {
  std::wstring code;
  std::wstring name;
  std::wstring text[L_COUNT];
  bool has[L_COUNT] = {};
};

static std::wstring g_tab[L_COUNT];
static LangData g_langs[3];
static LangData g_custom;
static bool g_builtinsLoaded = false;

static const int kLangResources[3] = {101, 102, 103};

static const char *kKeys[L_COUNT] = {
    "app",
    "record",
    "stop",
    "play",
    "open",
    "save",
    "speed",
    "speed05",
    "speed1",
    "speed2",
    "speed5",
    "speed10",
    "speed_turbo",
    "speed_max",
    "speed_custom",
    "loops",
    "loops1",
    "loops2",
    "loops3",
    "loops5",
    "loops10",
    "loops100",
    "continuous",
    "loops_custom2",
    "interface_theme",
    "interface",
    "theme_dark",
    "theme_light",
    "theme_acrylic",
    "language",
    "lang_en",
    "lang_ru",
    "lang_zh",
    "lang_import",
    "lang_translate",
    "hotkeys",
    "hk_record",
    "hk_play",
    "panic_keys",
    "panic_pause",
    "panic_scroll",
    "panic_esc",
    "panic_mouse",
    "statusbar",
    "sb_enable",
    "sb_top",
    "sb_bottom",
    "sb_left",
    "sb_center",
    "sb_right",
    "sb_follow_monitor",
    "assoc_onty",
    "assoc_success",
    "assoc_removed",
    "topmost",
    "update",
    "update_available",
    "exit",
    "about",
    "status_rec_stop",
    "status_play",
    "status_play_stop",
    "status_saved",
    "status_nothing",
    "status_invalid",
    "status_conflict",
    "elevated_warn",
    "elevated_cursor",
    "status_max",
    "status_uptodate",
    "status_updatefail",
    "status_translated",
    "status_loaded",
    "about_desc",
    "github",
    "license",
    "prompt_loops",
    "prompt_speed",
    "ok",
    "cancel",
    "save_fail",
    "minimize",
    "hide",
    "settings",
    "resave",
    "saved",
    "hk_press",
    "yes",
    "no",
    "new_macro",
    "developed",
    "credits",
    "donate",
    "lic_line",
    "tray_icon",
    "tray_auto",
    "ui_icon",
    "stop_bind",
    "autominimize",
    "recent_files",
    "clear_recent",
    "no_recent",
    "clipboard",
    "copy_clipboard",
    "paste_clipboard",
    "clipboard_loaded",
    "clipboard_copied",
    "clipboard_empty",
    "create_shortcut",
    "shortcut_success",
    "shortcut_fail",
    "install_local",
    "install_success",
    "install_fail",
    "uninstall_local",
    "uninstall_confirm",
    "uninstall_success",
    "uninstall_fail",
    "mode",
    "mode_onty",
    "mode_clicker",
    "file",
    "save_as",
    "events",
    "click_start",
    "click_stop",
    "click_interval",
    "click_target",
    "click_mouse",
    "click_keyboard",
    "click_button",
    "click_key",
    "click_left",
    "click_right",
    "click_middle",
    "click_type",
    "click_single",
    "click_double",
    "key_press",
    "q_interval",
    "ms",
    "click_stopped",
    "click_needkey",
    "click_hold",
    "q_hold",
    "hk_clicker",
    "panic_key",
    "panic_add",
    "panic_change",
    "panic_remove",
    "tray_enable",
    "loop_delay",
    "q_loop_delay",
    "click_advanced",
    "click_jitter",
    "q_click_jitter",
    "click_limit",
    "q_click_limit",
    "click_time_limit",
    "q_click_time_limit",
    "click_jiggle",
    "q_click_jiggle",
    "click_hold_key",
    "seconds",
    "off",
    "click_hold_hint",
    "rec_countdown",
    "q_rec_countdown",
    "countdown",
    "q_play_countdown",
    "countdown_play",
    "opacity",
    "q_opacity",
    "accent",
    "reset",
    "theme_import",
    "scale",
    "progress",
    "eta",
};

static bool ReadResourceText(int id, std::string &out) {
  HRSRC h = FindResourceW(g_hInst, MAKEINTRESOURCEW(id), RT_RCDATA);
  if (!h)
    return false;
  DWORD size = SizeofResource(g_hInst, h);
  HGLOBAL res = LoadResource(g_hInst, h);
  if (!res)
    return false;
  const void *p = LockResource(res);
  if (!p)
    return false;
  out.assign(static_cast<const char *>(p), size);
  return true;
}

static bool ParseLangJson(const std::wstring &src, LangData &out) {
  out = LangData();

  std::map<std::wstring, int> keyIndex;
  for (int i = 0; i < L_COUNT; i++)
    keyIndex[json::ToWide(kKeys[i])] = i;

  json::Js j{src.c_str(), src.c_str() + src.size()};
  if (!json::Eat(j, L'{'))
    return false;

  bool sawStrings = false;
  for (;;) {
    json::Ws(j);
    if (json::Eat(j, L'}'))
      break;
    std::wstring key;
    if (!json::Str(j, key) || !json::Eat(j, L':'))
      return false;

    if (key == L"code") {
      if (!json::Str(j, out.code))
        return false;
    } else if (key == L"name") {
      if (!json::Str(j, out.name))
        return false;
    } else if (key == L"strings") {
      sawStrings = true;
      if (!json::Eat(j, L'{'))
        return false;
      for (;;) {
        json::Ws(j);
        if (json::Eat(j, L'}'))
          break;
        std::wstring sk, text;
        if (!json::Str(j, sk) || !json::Eat(j, L':') || !json::Str(j, text))
          return false;
        auto it = keyIndex.find(sk);
        if (it != keyIndex.end()) {
          out.text[it->second] = text;
          out.has[it->second] = true;
        }
        if (!json::Eat(j, L',')) {
          json::Ws(j);
          if (!json::Eat(j, L'}'))
            return false;
          break;
        }
      }
    } else if (!json::SkipVal(j)) {
      return false;
    }

    if (!json::Eat(j, L',')) {
      json::Ws(j);
      if (!json::Eat(j, L'}'))
        return false;
      break;
    }
  }
  return sawStrings;
}

static bool LoadBuiltin(int index, int resourceId) {
  std::string bytes;
  if (!ReadResourceText(resourceId, bytes))
    return false;
  return ParseLangJson(json::ToWide(bytes), g_langs[index]);
}

static void LoadBuiltins() {
  if (g_builtinsLoaded)
    return;
  g_builtinsLoaded = true;
  for (int i = 0; i < 3; i++)
    LoadBuiltin(i, kLangResources[i]);
}

static void ApplyLang(const LangData &data) {
  for (int i = 0; i < L_COUNT; i++) {
    if (data.has[i] && !data.text[i].empty())
      g_tab[i] = data.text[i];
    else if (g_langs[0].has[i])
      g_tab[i] = g_langs[0].text[i];
    else
      g_tab[i].clear();
  }
}

const wchar_t *L(int id) { return g_tab[id].c_str(); }

static std::wstring CustomLangPath() {
  wchar_t p[MAX_PATH];
  GetModuleFileNameW(nullptr, p, MAX_PATH);
  wchar_t *s = wcsrchr(p, L'\\');
  if (s)
    *(s + 1) = 0;
  lstrcatW(p, L"OntyTask.lang.json");
  return p;
}

void LangInit() {
  LoadBuiltins();
  if (g_cfg.lang == 0) {
    if (LangImport(CustomLangPath().c_str()))
      return;
    g_cfg.lang = 1;
  }
  int index = (g_cfg.lang >= 1 && g_cfg.lang <= 3) ? g_cfg.lang - 1 : 0;
  ApplyLang(g_langs[index]);
}

bool LangImport(const wchar_t *path) {
  LoadBuiltins();
  std::string bytes;
  if (!json::ReadTextFile(path, bytes))
    return false;
  LangData data;
  if (!ParseLangJson(json::ToWide(bytes), data))
    return false;
  bool any = false;
  for (int i = 0; i < L_COUNT; i++) {
    if (data.has[i]) {
      any = true;
      break;
    }
  }
  if (!any)
    return false;
  g_custom = data;
  ApplyLang(g_custom);
  std::wstring customPath = CustomLangPath();
  if (_wcsicmp(path, customPath.c_str()) != 0)
    CopyFileW(path, customPath.c_str(), FALSE);
  return true;
}
