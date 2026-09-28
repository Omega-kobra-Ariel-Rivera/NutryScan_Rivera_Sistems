#include "util.h"

#include <cmath>
#include <cstdio>
#include <cwchar>

namespace util {

std::wstring FormatNumber(double value, int decimals) {
  const bool negative = value < 0;
  value = std::fabs(value);
  long long factor = 1;
  for (int i = 0; i < decimals; ++i) factor *= 10;
  const long long scaled = std::llround(value * static_cast<double>(factor));
  const long long integer = scaled / factor;
  const long long fraction = scaled % factor;

  std::wstring s = std::to_wstring(integer);
  for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3) s.insert(static_cast<size_t>(i), L".");

  if (decimals > 0 && fraction != 0) {
    wchar_t buf[32];
    swprintf(buf, 32, L"%0*lld", decimals, fraction);
    std::wstring f = buf;
    while (!f.empty() && f.back() == L'0') f.pop_back();
    s += L"," + f;
  }
  return (negative && s != L"0") ? L"-" + s : s;
}

std::wstring Kcal(double value) { return FormatNumber(value, 0) + L" kcal"; }
std::wstring Grams(double value) { return FormatNumber(value, 1) + L" g"; }

std::wstring PlainNumber(double value) {
  wchar_t buf[64];
  swprintf(buf, 64, L"%.2f", value);
  std::wstring s = buf;
  while (!s.empty() && s.back() == L'0') s.pop_back();
  if (!s.empty() && s.back() == L'.') s.pop_back();
  return s;
}

bool ParseNumber(const std::wstring& text, double* out) {
  std::wstring t = Trim(text);
  if (t.empty()) return false;
  for (auto& c : t) {
    if (c == L',') c = L'.';
  }
  wchar_t* end = nullptr;
  const double v = std::wcstod(t.c_str(), &end);
  if (end == t.c_str() || *end != L'\0' || !std::isfinite(v)) return false;
  *out = v;
  return true;
}

SYSTEMTIME AddDays(const SYSTEMTIME& date, int days) {
  SYSTEMTIME s = date;
  s.wHour = 12;
  s.wMinute = s.wSecond = s.wMilliseconds = 0;
  FILETIME ft;
  SystemTimeToFileTime(&s, &ft);
  ULARGE_INTEGER u;
  u.LowPart = ft.dwLowDateTime;
  u.HighPart = ft.dwHighDateTime;
  const long long delta = static_cast<long long>(days) * 864000000000LL;  // 1 día en 100 ns
  u.QuadPart = static_cast<ULONGLONG>(static_cast<long long>(u.QuadPart) + delta);
  ft.dwLowDateTime = u.LowPart;
  ft.dwHighDateTime = u.HighPart;
  SYSTEMTIME result;
  FileTimeToSystemTime(&ft, &result);
  result.wHour = result.wMinute = result.wSecond = result.wMilliseconds = 0;
  return result;
}

SYSTEMTIME Today() {
  SYSTEMTIME st;
  GetLocalTime(&st);
  return AddDays(st, 0);
}

SYSTEMTIME FromKey(const std::wstring& key) {
  int y = 2026, m = 1, d = 1;
  swscanf_s(key.c_str(), L"%d-%d-%d", &y, &m, &d);
  SYSTEMTIME st = {};
  st.wYear = static_cast<WORD>(y);
  st.wMonth = static_cast<WORD>(m);
  st.wDay = static_cast<WORD>(d);
  return AddDays(st, 0);
}

std::wstring DateKey(const SYSTEMTIME& d) {
  wchar_t buf[16];
  swprintf(buf, 16, L"%04d-%02d-%02d", d.wYear, d.wMonth, d.wDay);
  return buf;
}

std::wstring DateShort(const SYSTEMTIME& d) {
  wchar_t buf[16];
  swprintf(buf, 16, L"%02d-%02d-%04d", d.wDay, d.wMonth, d.wYear);
  return buf;
}

static const wchar_t* kDays[] = {L"Domingo", L"Lunes", L"Martes", L"Miércoles", L"Jueves", L"Viernes", L"Sábado"};
static const wchar_t* kDaysShort[] = {L"Dom", L"Lun", L"Mar", L"Mié", L"Jue", L"Vie", L"Sáb"};
static const wchar_t* kMonths[] = {L"enero", L"febrero", L"marzo", L"abril", L"mayo", L"junio", L"julio",
                                   L"agosto", L"septiembre", L"octubre", L"noviembre", L"diciembre"};

std::wstring DateLong(const SYSTEMTIME& date) {
  const SYSTEMTIME d = AddDays(date, 0);
  return std::wstring(kDays[d.wDayOfWeek % 7]) + L" " + std::to_wstring(d.wDay) + L" de " +
         kMonths[(d.wMonth + 11) % 12] + L" de " + std::to_wstring(d.wYear);
}

std::wstring WeekdayShort(const SYSTEMTIME& date) { return kDaysShort[AddDays(date, 0).wDayOfWeek % 7]; }

std::string ToUtf8(const std::wstring& text) {
  if (text.empty()) return {};
  const int n = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
  std::string out(static_cast<size_t>(n), '\0');
  WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &out[0], n, nullptr, nullptr);
  return out;
}

std::wstring FromUtf8(const std::string& text) {
  if (text.empty()) return {};
  const int n = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
  std::wstring out(static_cast<size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &out[0], n);
  return out;
}

std::vector<std::wstring> Split(const std::wstring& text, wchar_t sep) {
  std::vector<std::wstring> parts;
  size_t start = 0;
  for (;;) {
    const size_t pos = text.find(sep, start);
    if (pos == std::wstring::npos) {
      parts.push_back(text.substr(start));
      break;
    }
    parts.push_back(text.substr(start, pos - start));
    start = pos + 1;
  }
  return parts;
}

std::wstring Trim(const std::wstring& text) {
  const size_t a = text.find_first_not_of(L" \t\r\n");
  if (a == std::wstring::npos) return {};
  const size_t b = text.find_last_not_of(L" \t\r\n");
  return text.substr(a, b - a + 1);
}

}  // namespace util
