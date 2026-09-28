#pragma once
#include <windows.h>
#include <string>
#include <vector>

// Formatos de números y fechas en español de Chile.
namespace util {

std::wstring FormatNumber(double value, int decimals);  // 1.450 / 14,5
std::wstring Kcal(double value);                        // "1.450 kcal"
std::wstring Grams(double value);                       // "14,5 g"
std::wstring PlainNumber(double value);                 // "14.5" (para campos editables)
bool ParseNumber(const std::wstring& text, double* out);

SYSTEMTIME Today();
SYSTEMTIME AddDays(const SYSTEMTIME& date, int days);
SYSTEMTIME FromKey(const std::wstring& key);
std::wstring DateKey(const SYSTEMTIME& date);           // 2026-09-27
std::wstring DateShort(const SYSTEMTIME& date);         // 27-09-2026
std::wstring DateLong(const SYSTEMTIME& date);          // Domingo 27 de septiembre de 2026
std::wstring WeekdayShort(const SYSTEMTIME& date);      // Dom

std::string ToUtf8(const std::wstring& text);
std::wstring FromUtf8(const std::string& text);
std::vector<std::wstring> Split(const std::wstring& text, wchar_t sep);
std::wstring Trim(const std::wstring& text);

}  // namespace util
