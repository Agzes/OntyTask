#include "lang/lang.h"
#include "core/config.h"
#include "core/json.h"

struct LEntry {
  const char *key;
  const wchar_t *en;
  const wchar_t *ru;
};

static std::wstring g_tab[L_COUNT];

static const LEntry kTable[L_COUNT] = {
    {"app", L"OntyTask", L"OntyTask"},
    {"record", L"Record", L"Запись"},
    {"stop", L"Stop", L"Остановить"},
    {"play", L"Play", L"Воспроизвести"},
    {"open", L"Open...", L"Открыть..."},
    {"save", L"Save...", L"Сохранить..."},
    {"speed", L"Speed", L"Скорость"},
    {"speed05", L"0.5x", L"0.5x"},
    {"speed1", L"1x (Normal)", L"1x (Обычная)"},
    {"speed2", L"2x (Fast)", L"2x (Быстрая)"},
    {"speed5", L"5x", L"5x"},
    {"speed10", L"10x", L"10x"},
    {"speed_turbo", L"100x Turbo", L"100x Турбо"},
    {"speed_max", L"Max Speed (Instant)", L"Максимальная (Без задержек)"},
    {"speed_custom", L"Custom...", L"Своя..."},
    {"loops", L"Playback loops", L"Повторы"},
    {"loops1", L"1", L"1"},
    {"loops2", L"2", L"2"},
    {"loops3", L"3", L"3"},
    {"loops5", L"5", L"5"},
    {"loops10", L"10", L"10"},
    {"loops100", L"100", L"100"},
    {"continuous", L"Continuous (\u221E)", L"Бесконечно (\u221E)"},
    {"loops_custom2", L"Custom...", L"Своё..."},
    {"theme", L"Theme", L"Тема"},
    {"interface_theme", L"Interface theme", L"Тема интерфейса"},
    {"theme_dark", L"Dark", L"Тёмная"},
    {"theme_light", L"Light", L"Светлая"},
    {"theme_acrylic", L"Acrylic", L"Acrylic"},
    {"language", L"Language", L"Язык"},
    {"lang_en", L"English", L"English"},
    {"lang_ru", L"Русский", L"Русский"},
    {"lang_import", L"Import translation JSON...", L"Импорт перевода JSON..."},
    {"lang_translate", L"Translate on GitHub...", L"Перевести на GitHub..."},
    {"hotkeys", L"Hotkeys", L"Горячие клавиши"},
    {"hk_record", L"Recording", L"Запись"},
    {"hk_play", L"Playback", L"Воспроизведение"},
    {"panic_keys", L"Panic stop keys", L"Экстренная остановка"},
    {"panic_pause", L"Pause / Break", L"Pause / Break"},
    {"panic_scroll", L"Scroll Lock", L"Scroll Lock"},
    {"panic_esc", L"Escape key", L"Клавиша Esc"},
    {"panic_mouse", L"Stop on mouse move", L"Остановка при движении мыши"},
    {"statusbar", L"Status bar", L"Статус-бар"},
    {"sb_enable", L"Show status bar", L"Показывать статус-бар"},
    {"sb_top", L"Top of screen", L"Вверху экрана"},
    {"sb_bottom", L"Bottom of screen", L"Внизу экрана"},
    {"sb_follow_monitor", L"Follow cursor monitor",
     L"Следовать за монитором курсора"},
    {"assoc_onty", L"Associate .onty files", L"Ассоциировать файлы .onty"},
    {"assoc_success", L".onty files registered", L"Файлы .onty привязаны"},
    {"assoc_removed", L".onty association removed", L"Привязка .onty удалена"},
    {"topmost", L"Always on top", L"Поверх всех окон"},
    {"update", L"Check for updates...", L"Проверить обновления..."},
    {"update_available", L"Update available!", L"Доступно обновление!"},
    {"exit", L"Exit", L"Выход"},
    {"about", L"About OntyTask", L"О программе OntyTask"},
    {"status_rec_stop", L"Recording stopped", L"Запись остановлена"},
    {"status_play", L"Playing", L"Воспроизведение"},
    {"status_play_stop", L"Playback stopped", L"Воспроизведение остановлено"},
    {"status_saved", L"Macro saved", L"Макрос сохранён"},
    {"status_nothing", L"Nothing recorded", L"Нет записи"},
    {"status_invalid", L"Invalid file", L"Неверный файл"},
    {"status_conflict", L"Hotkey conflict", L"Конфликт горячих клавиш"},
    {"status_max", L"Event limit reached", L"Достигнут лимит событий"},
    {"status_uptodate", L"You have the latest version",
     L"У вас последняя версия"},
    {"status_updatefail", L"Update check failed",
     L"Не удалось проверить обновления"},
    {"status_translated", L"Translation imported", L"Перевод импортирован"},
    {"status_loaded", L"Macro loaded", L"Макрос загружен"},
    {"about_desc",
     L"An open-source successor to TinyTask for fast desktop and 3D "
     L"automation.",
     L"Открытый наследник TinyTask для быстрой автоматизации на рабочем столе "
     L"и в 3D."},
    {"github", L"GitHub", L"GitHub"},
    {"license", L"License", L"Лицензия"},
    {"prompt_loops", L"Number of playback loops (1-99999)",
     L"Число повторов проигрывания (1-99999)"},
    {"prompt_speed", L"Speed multiplier (0.01-1000)",
     L"Множитель скорости (0.01-1000)"},
    {"ok", L"OK", L"OK"},
    {"cancel", L"Cancel", L"Отмена"},
    {"save_fail", L"Unable to save file", L"Не удалось сохранить файл"},
    {"minimize", L"Minimize", L"Свернуть"},
    {"hide", L"Hide to tray", L"Скрыть в трей"},
    {"settings", L"Settings", L"Настройки"},
    {"resave", L"Save changes", L"Сохранить изменения"},
    {"saved", L"Saved", L"Сохранено"},
    {"hk_press", L"Press a new hotkey (Esc to cancel)",
     L"Нажмите новую комбинацию (Esc - отмена)"},
    {"yes", L"Yes", L"Да"},
    {"no", L"No", L"Нет"},
    {"new_macro", L"New macro", L"Новый макрос"},
    {"developed", L"Developed with \u2764 by Agzes",
     L"Разработано с \u2764 - Agzes"},
    {"lic_line", L"Licensed under GNU GPL v3.0", L"Лицензия: GNU GPL v3.0"},
    {"tray_icon", L"Tray icon", L"Иконка трея"},
    {"tray_auto", L"Follow theme", L"Как тема"},
    {"ui_icon", L"Interface icon", L"Иконка интерфейса"},
    {"stop_bind", L"Stop", L"Остановить"},
    {"autominimize", L"Auto-minimize on Rec/Play",
     L"Авто-сворачивание при записи/старте"},
    {"recent_files", L"Recent files", L"Недавние файлы"},
    {"clear_recent", L"Clear list", L"Очистить список"},
    {"no_recent", L"(No recent files)", L"(Нет недавних файлов)"},
    {"clipboard", L"Clipboard", L"Буфер обмена"},
    {"copy_clipboard", L"Copy macro to clipboard",
     L"Копировать макрос в буфер"},
    {"paste_clipboard", L"Paste macro from clipboard",
     L"Вставить макрос из буфера"},
    {"clipboard_loaded", L"Macro loaded from clipboard",
     L"Макрос загружен из буфера"},
    {"clipboard_copied", L"Macro copied to clipboard",
     L"Макрос скопирован в буфер"},
    {"clipboard_empty", L"Clipboard has no valid macro",
     L"В буфере нет валидного макроса"},
    {"create_shortcut", L"Create Desktop shortcut",
     L"Создать ярлык на рабочем столе"},
    {"shortcut_success", L"Desktop shortcut created",
     L"Ярлык создан на рабочем столе"},
    {"shortcut_fail", L"Failed to create shortcut",
     L"Не удалось создать ярлык"},
    {"install_local", L"Install OntyTask to Local AppData",
     L"Установить OntyTask в Local AppData"},
    {"install_success", L"OntyTask installed successfully",
     L"OntyTask успешно установлен"},
    {"install_fail", L"Failed to install OntyTask",
     L"Не удалось установить OntyTask"}};

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
  if (g_cfg.lang == 0) {
    std::wstring dummy;
    if (LangImport(CustomLangPath().c_str(), dummy))
      return;
    g_cfg.lang = 1;
  }
  for (int i = 0; i < L_COUNT; i++)
    g_tab[i] = (g_cfg.lang == 2) ? kTable[i].ru : kTable[i].en;
}

bool LangImport(const wchar_t *path, std::wstring &nameOut) {
  std::string bytes;
  if (!json::ReadTextFile(path, bytes))
    return false;
  std::wstring src = json::ToWide(bytes);
  json::Js j{src.c_str(), src.c_str() + src.size()};
  if (!json::Eat(j, L'{'))
    return false;
  bool any = false;
  nameOut.clear();
  for (;;) {
    json::Ws(j);
    if (json::Eat(j, L'}'))
      break;
    std::wstring key;
    if (!json::Str(j, key))
      return false;
    if (!json::Eat(j, L':'))
      return false;
    if (key == L"lang") {
      json::Str(j, nameOut);
    } else if (key == L"strings") {
      json::Ws(j);
      if (!json::Eat(j, L'{'))
        return false;
      for (;;) {
        json::Ws(j);
        if (json::Eat(j, L'}'))
          break;
        std::wstring sk, sv;
        if (!json::Str(j, sk) || !json::Eat(j, L':') || !json::Str(j, sv))
          return false;
        for (int i = 0; i < L_COUNT; i++) {
          if (sk == json::ToWide(kTable[i].key)) {
            g_tab[i] = sv;
            any = true;
            break;
          }
        }
        if (!json::Eat(j, L',')) {
          json::Ws(j);
          if (!json::Eat(j, L'}'))
            return false;
          break;
        }
      }
    } else {
      if (!json::SkipVal(j))
        return false;
    }
    if (!json::Eat(j, L',')) {
      json::Ws(j);
      if (!json::Eat(j, L'}'))
        return false;
      break;
    }
  }
  if (any) {
    std::wstring customPath = CustomLangPath();
    if (_wcsicmp(path, customPath.c_str()) != 0) {
      CopyFileW(path, customPath.c_str(), FALSE);
    }
  }
  return any;
}
