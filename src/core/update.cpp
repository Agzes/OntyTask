#include "core/update.h"
#include "core/config.h"
#include <atomic>
#include <string>
#include <wininet.h>

const wchar_t *APP_VERSION = L"v.0.1";
const wchar_t *APP_VERSION_RAW = L"0.1.0";
static const wchar_t *VER_URL =
    L"https://raw.githubusercontent.com/Agzes/OntyTask/main/version";
const wchar_t *URL_RELEASES = L"https://github.com/Agzes/OntyTask/releases";
const wchar_t *URL_GITHUB = L"https://github.com/Agzes/OntyTask";
const wchar_t *URL_ISSUE = L"https://github.com/Agzes/OntyTask/issues/new";
const wchar_t *URL_LICENSE =
    L"https://github.com/Agzes/OntyTask/blob/main/LICENSE";

static const int CUR_VERSION = 100000;

static std::atomic<bool> g_found{false};
static std::atomic<int> g_state2{UR_NONE};

bool Update_Found() { return g_found.load(); }
int Update_State() { return g_state2.load(); }

static bool HttpGetVersion(int &out) {
  HINTERNET h = InternetOpenW(L"OntyTask/0.1", INTERNET_OPEN_TYPE_PRECONFIG,
                              nullptr, nullptr, 0);
  if (!h)
    return false;
  DWORD t = 5000;
  InternetSetOptionW(h, INTERNET_OPTION_CONNECT_TIMEOUT, &t, sizeof(t));
  InternetSetOptionW(h, INTERNET_OPTION_SEND_TIMEOUT, &t, sizeof(t));
  InternetSetOptionW(h, INTERNET_OPTION_RECEIVE_TIMEOUT, &t, sizeof(t));
  HINTERNET c = InternetOpenUrlW(
      h, VER_URL, nullptr, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);
  if (!c) {
    InternetCloseHandle(h);
    return false;
  }
  char buf[128] = {};
  DWORD rd = 0;
  std::string data;
  while (InternetReadFile(c, buf, sizeof(buf) - 1, &rd) && rd) {
    data.append(buf, rd);
    if (data.size() >= 64)
      break;
  }
  InternetCloseHandle(c);
  InternetCloseHandle(h);
  if (data.empty())
    return false;
  out = atoi(data.c_str());
  return out > 0;
}

static DWORD WINAPI CheckProc(LPVOID param) {
  bool manual = param != nullptr;
  int srv = 0;
  bool ok = HttpGetVersion(srv);
  int result = !ok ? -1 : (srv > CUR_VERSION ? 1 : 0);
  if (result == 1) {
    g_found = true;
    g_state2 = UR_FOUND;
  } else
    g_state2 = result == 0 ? UR_LATEST : UR_FAIL;
  PostMessageW(g_hwnd, WM_APP + 2, manual ? 1 : 0, result);
  return 0;
}

static void Spawn(bool manual) {
  HANDLE h = CreateThread(nullptr, 0, CheckProc,
                          (LPVOID)(INT_PTR)(manual ? 1 : 0), 0, nullptr);
  if (h)
    CloseHandle(h);
}

void Update_AutoCheck() {
  if (g_cfg.autoupdate)
    Spawn(false);
}

void Update_ManualCheck() {
  if (g_state2.load() != UR_FOUND)
    g_state2 = UR_CHECKING;
  Spawn(true);
}
