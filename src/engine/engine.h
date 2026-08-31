#pragma once
#include <atomic>
#include <string>
#include <utility>
#include <vector>
#include <windows.h>

enum { A_MD = 0, A_MU, A_KD, A_KU, A_MOVE, A_WH };
enum { HK_CTRL = 1, HK_SHIFT = 2, HK_ALT = 4, HK_WIN = 8 };
enum { ST_IDLE = 0, ST_REC = 1, ST_PLAY = 2 };

struct MAc {
  int type;
  DWORD delay;
  LONG x, y;
  BYTE btn;
  BYTE vk;
  DWORD flags;
  bool mDown;
  bool mUp;
  bool relative;
  std::vector<std::pair<WORD, WORD>> path;
  std::vector<WORD> pdelays;
  SHORT wheel;
  MAc()
      : type(0), delay(0), x(0), y(0), btn(0), vk(0), flags(0), mDown(false),
        mUp(false), relative(false), wheel(0) {}
};

extern std::vector<MAc> g_events;
extern std::atomic<int> g_state;
extern std::atomic<int> g_playLoop;
extern std::atomic<bool> g_playStopped;
extern std::atomic<bool> g_mouseMovedDuringPlay;
extern DWORD g_startTick;

bool HotkeyDown(int mods, int vk);
std::wstring FmtHk(int mods, int vk);

void Engine_InitRawInput();
void Engine_RawInput(LPARAM l);
void Engine_FlushMoves();

bool Engine_Record();
bool Engine_RecordStop();
bool Engine_Play();
void Engine_PlayStop();
void Engine_Shutdown();
bool Engine_Save(const wchar_t *path);
bool Engine_Load(const wchar_t *path);
bool Engine_ToJson(std::string &outJson);
bool Engine_FromJson(const std::string &jsonStr);
