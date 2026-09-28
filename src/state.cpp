#include "state.h"

#include <windows.h>
#include <shlobj.h>

#include <algorithm>
#include <cwchar>
#include <fstream>
#include <set>

#include "util.h"

const wchar_t* MealLabel(int meal) {
  static const wchar_t* labels[kMealCount] = {L"Desayuno", L"Almuerzo", L"Once", L"Cena", L"Colación"};
  return (meal >= 0 && meal < kMealCount) ? labels[meal] : labels[1];
}

Totals SumEntries(const std::vector<FoodEntry>& entries) {
  Totals t;
  for (const auto& e : entries) {
    t.calories += e.calories;
    t.protein += e.protein;
    t.carbs += e.carbs;
    t.fat += e.fat;
  }
  return t;
}

namespace {

// Evita que tabulaciones o saltos de línea rompan el archivo.
std::wstring Clean(const std::wstring& s) {
  std::wstring out = s;
  for (auto& c : out) {
    if (c == L'\t' || c == L'\r' || c == L'\n') c = L' ';
  }
  return out;
}

double ToDouble(const std::wstring& s) { return std::wcstod(s.c_str(), nullptr); }

std::wstring Num(double v) {
  wchar_t buf[64];
  swprintf(buf, 64, L"%.3f", v);
  return buf;
}

}  // namespace

std::wstring AppState::DataPath() const {
  PWSTR roaming = nullptr;
  std::wstring dir;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &roaming))) {
    dir = roaming;
    CoTaskMemFree(roaming);
  } else {
    dir = L".";
  }
  dir += L"\\NutryScan";
  CreateDirectoryW(dir.c_str(), nullptr);
  return dir + L"\\datos.txt";
}

bool AppState::Load() {
  entries_.clear();
  std::ifstream in(DataPath().c_str(), std::ios::binary);
  if (!in) return false;

  std::string raw;
  while (std::getline(in, raw)) {
    if (!raw.empty() && raw.back() == '\r') raw.pop_back();
    const auto p = util::Split(util::FromUtf8(raw), L'\t');
    if (p.empty()) continue;

    if (p[0] == L"PROFILE" && p.size() >= 7) {
      profile.name = p[1];
      profile.email = p[2];
      profile.calorieGoal = ToDouble(p[3]);
      profile.proteinGoal = ToDouble(p[4]);
      profile.carbsGoal = ToDouble(p[5]);
      profile.fatGoal = ToDouble(p[6]);
    } else if (p[0] == L"ENTRY" && p.size() >= 9) {
      FoodEntry e;
      e.id = nextId_++;
      e.date = p[1];
      e.meal = std::clamp(static_cast<int>(ToDouble(p[2])), 0, kMealCount - 1);
      e.name = p[3];
      e.portion = p[4];
      e.calories = ToDouble(p[5]);
      e.protein = ToDouble(p[6]);
      e.carbs = ToDouble(p[7]);
      e.fat = ToDouble(p[8]);
      entries_.push_back(e);
    }
  }
  if (profile.calorieGoal <= 0) profile = Profile{};
  return true;
}

bool AppState::Save() const {
  const std::wstring path = DataPath();
  const std::wstring tmp = path + L".tmp";
  {
    std::ofstream out(tmp.c_str(), std::ios::binary | std::ios::trunc);
    if (!out) return false;
    std::wstring line = L"PROFILE\t" + Clean(profile.name) + L"\t" + Clean(profile.email) + L"\t" +
                        Num(profile.calorieGoal) + L"\t" + Num(profile.proteinGoal) + L"\t" +
                        Num(profile.carbsGoal) + L"\t" + Num(profile.fatGoal) + L"\n";
    out << util::ToUtf8(line);
    for (const auto& e : entries_) {
      line = L"ENTRY\t" + e.date + L"\t" + std::to_wstring(e.meal) + L"\t" + Clean(e.name) + L"\t" +
             Clean(e.portion) + L"\t" + Num(e.calories) + L"\t" + Num(e.protein) + L"\t" + Num(e.carbs) +
             L"\t" + Num(e.fat) + L"\n";
      out << util::ToUtf8(line);
    }
    if (!out.good()) return false;
  }
  return MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
}

std::vector<FoodEntry> AppState::EntriesFor(const std::wstring& dateKey) const {
  std::vector<FoodEntry> list;
  for (const auto& e : entries_) {
    if (e.date == dateKey) list.push_back(e);
  }
  std::stable_sort(list.begin(), list.end(), [](const FoodEntry& a, const FoodEntry& b) { return a.meal < b.meal; });
  return list;
}

Totals AppState::TotalsFor(const std::wstring& dateKey) const { return SumEntries(EntriesFor(dateKey)); }

std::vector<std::wstring> AppState::DaysWithEntries() const {
  std::set<std::wstring> days;
  for (const auto& e : entries_) days.insert(e.date);
  return std::vector<std::wstring>(days.rbegin(), days.rend());
}

void AppState::Add(FoodEntry entry) {
  entry.id = nextId_++;
  entries_.push_back(std::move(entry));
}

bool AppState::Remove(long long id) {
  const auto it = std::find_if(entries_.begin(), entries_.end(), [id](const FoodEntry& e) { return e.id == id; });
  if (it == entries_.end()) return false;
  entries_.erase(it);
  return true;
}
