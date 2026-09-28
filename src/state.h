#pragma once
#include <string>
#include <vector>

constexpr int kMealCount = 5;
const wchar_t* MealLabel(int meal);  // 0 Desayuno, 1 Almuerzo, 2 Once, 3 Cena, 4 Colación

struct FoodEntry {
  long long id = 0;
  std::wstring date;  // yyyy-mm-dd
  int meal = 1;
  std::wstring name;
  std::wstring portion;
  double calories = 0;
  double protein = 0;
  double carbs = 0;
  double fat = 0;
};

struct Profile {
  std::wstring name = L"Usuario";
  std::wstring email;
  double calorieGoal = 2000;
  double proteinGoal = 120;
  double carbsGoal = 230;
  double fatGoal = 65;
};

struct Totals {
  double calories = 0;
  double protein = 0;
  double carbs = 0;
  double fat = 0;
};

Totals SumEntries(const std::vector<FoodEntry>& entries);

// Guarda el perfil y el diario en %APPDATA%\NutryScan\datos.txt (UTF-8).
class AppState {
 public:
  bool Load();
  bool Save() const;

  std::vector<FoodEntry> EntriesFor(const std::wstring& dateKey) const;  // ordenados por comida
  Totals TotalsFor(const std::wstring& dateKey) const;
  std::vector<std::wstring> DaysWithEntries() const;  // más reciente primero

  void Add(FoodEntry entry);
  bool Remove(long long id);

  std::wstring DataPath() const;

  Profile profile;

 private:
  std::vector<FoodEntry> entries_;
  long long nextId_ = 1;
};
