#pragma once
#include <cstdlib>
#include <cwchar>
#include <string>
#include <windows.h>

namespace json {

struct Js {
  const wchar_t *p;
  const wchar_t *e;
};

inline void Ws(Js &j) {
  while (j.p < j.e &&
         (*j.p == L' ' || *j.p == L'\t' || *j.p == L'\r' || *j.p == L'\n'))
    ++j.p;
}

inline bool Eat(Js &j, wchar_t c) {
  Ws(j);
  if (j.p < j.e && *j.p == c) {
    ++j.p;
    return true;
  }
  return false;
}

inline bool Num(Js &j, double &out) {
  Ws(j);
  if (j.p >= j.e)
    return false;
  wchar_t *end = nullptr;
  double v = wcstod(j.p, &end);
  if (end == j.p)
    return false;
  out = v;
  j.p = end;
  return true;
}

inline bool Bool(Js &j, bool &out) {
  Ws(j);
  if (j.e - j.p >= 4 && wcsncmp(j.p, L"true", 4) == 0) {
    out = true;
    j.p += 4;
    return true;
  }
  if (j.e - j.p >= 5 && wcsncmp(j.p, L"false", 5) == 0) {
    out = false;
    j.p += 5;
    return true;
  }
  return false;
}

inline wchar_t Hex4(Js &j) {
  if (j.e - j.p < 4)
    return 0;
  wchar_t v = (wchar_t)wcstol(std::wstring(j.p, j.p + 4).c_str(), nullptr, 16);
  j.p += 4;
  return v;
}

inline bool Str(Js &j, std::wstring &out) {
  Ws(j);
  if (j.p >= j.e || *j.p != L'"')
    return false;
  ++j.p;
  out.clear();
  while (j.p < j.e) {
    wchar_t c = *j.p++;
    if (c == L'"')
      return true;
    if (c != L'\\') {
      out += c;
      continue;
    }
    if (j.p >= j.e)
      return false;
    wchar_t e = *j.p++;
    switch (e) {
    case L'"':
      out += L'"';
      break;
    case L'\\':
      out += L'\\';
      break;
    case L'/':
      out += L'/';
      break;
    case L'b':
      out += L'\b';
      break;
    case L'f':
      out += L'\f';
      break;
    case L'n':
      out += L'\n';
      break;
    case L'r':
      out += L'\r';
      break;
    case L't':
      out += L'\t';
      break;
    case L'u': {
      wchar_t v = Hex4(j);
      if (v >= 0xD800 && v <= 0xDBFF && j.e - j.p >= 6 && j.p[0] == L'\\' &&
          j.p[1] == L'u') {
        j.p += 2;
        wchar_t lo = Hex4(j);
        if (lo >= 0xDC00 && lo <= 0xDFFF)
          v = (wchar_t)(0x10000 + (((v - 0xD800) << 10) | (lo - 0xDC00)));
      }
      if (v)
        out += v;
      break;
    }
    default:
      return false;
    }
  }
  return false;
}

inline bool SkipVal(Js &j) {
  Ws(j);
  if (j.p >= j.e)
    return false;
  wchar_t c = *j.p;
  if (c == L'"') {
    std::wstring s;
    return Str(j, s);
  }
  if (c == L'{' || c == L'[') {
    wchar_t open = c, close = (c == L'{') ? L'}' : L']';
    ++j.p;
    int d = 1;
    while (j.p < j.e && d > 0) {
      wchar_t k = *j.p++;
      if (k == open)
        ++d;
      else if (k == close)
        --d;
      else if (k == L'"') {
        --j.p;
        std::wstring s;
        if (!Str(j, s))
          return false;
      }
    }
    return d == 0;
  }
  while (j.p < j.e && *j.p != L',' && *j.p != L'}' && *j.p != L']')
    ++j.p;
  return true;
}

inline std::string ToUtf8(const std::wstring &w) {
  if (w.empty())
    return std::string();
  int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0,
                              nullptr, nullptr);
  std::string s((size_t)n, 0);
  WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr,
                      nullptr);
  return s;
}

inline std::wstring ToWide(const std::string &u) {
  if (u.empty())
    return std::wstring();
  int n = MultiByteToWideChar(CP_UTF8, 0, u.c_str(), (int)u.size(), nullptr, 0);
  std::wstring w((size_t)n, 0);
  MultiByteToWideChar(CP_UTF8, 0, u.c_str(), (int)u.size(), &w[0], n);
  return w;
}

inline bool ReadTextFile(const wchar_t *path, std::string &out) {
  HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                         OPEN_EXISTING, 0, nullptr);
  if (h == INVALID_HANDLE_VALUE)
    return false;
  DWORD sz = GetFileSize(h, nullptr);
  if (sz == INVALID_FILE_SIZE || sz > 32 * 1024 * 1024) {
    CloseHandle(h);
    return false;
  }
  out.resize(sz);
  DWORD rd = 0;
  BOOL ok = sz == 0 || ReadFile(h, &out[0], sz, &rd, nullptr);
  CloseHandle(h);
  return ok && rd == sz;
}

inline bool WriteTextFile(const wchar_t *path, const std::string &data) {
  HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE)
    return false;
  DWORD w = 0;
  BOOL ok = data.empty() ||
            WriteFile(h, data.data(), (DWORD)data.size(), &w, nullptr);
  CloseHandle(h);
  return ok && w == data.size();
}

} // namespace json
