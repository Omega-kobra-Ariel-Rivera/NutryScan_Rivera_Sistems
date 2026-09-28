#include "add_food_dialog.h"

#include <string>

#include "catalog.h"
#include "resource.h"
#include "util.h"

namespace {

struct DialogContext {
  int meal;
  FoodEntry* out;
};

std::wstring GetText(HWND dlg, int id) {
  wchar_t buf[512] = {};
  GetDlgItemTextW(dlg, id, buf, 512);
  return util::Trim(buf);
}

void FillFromCatalog(HWND dlg, const CatalogFood& f) {
  SetDlgItemTextW(dlg, IDC_PORTION, f.portion);
  SetDlgItemTextW(dlg, IDC_CALORIES, util::PlainNumber(f.calories).c_str());
  SetDlgItemTextW(dlg, IDC_PROTEIN, util::PlainNumber(f.protein).c_str());
  SetDlgItemTextW(dlg, IDC_CARBS, util::PlainNumber(f.carbs).c_str());
  SetDlgItemTextW(dlg, IDC_FAT, util::PlainNumber(f.fat).c_str());
}

bool Warn(HWND dlg, int focusId, const wchar_t* message) {
  MessageBoxW(dlg, message, L"Revisa los datos", MB_ICONWARNING);
  SetFocus(GetDlgItem(dlg, focusId));
  return false;
}

bool ReadNumber(HWND dlg, int id, const wchar_t* message, double* out) {
  if (!util::ParseNumber(GetText(dlg, id), out) || *out < 0) return Warn(dlg, id, message);
  return true;
}

bool Collect(HWND dlg, FoodEntry* e) {
  e->name = GetText(dlg, IDC_FOOD_NAME);
  if (e->name.empty()) return Warn(dlg, IDC_FOOD_NAME, L"Escribe el nombre del alimento o elígelo del catálogo.");

  const std::wstring portion = GetText(dlg, IDC_PORTION);
  if (portion.empty()) return Warn(dlg, IDC_PORTION, L"Escribe la porción, por ejemplo: 1 taza.");

  double quantity = 0;
  if (!util::ParseNumber(GetText(dlg, IDC_QUANTITY), &quantity) || quantity <= 0)
    return Warn(dlg, IDC_QUANTITY, L"La cantidad de porciones debe ser mayor a 0.");

  double cal = 0, prot = 0, carb = 0, fat = 0;
  if (!ReadNumber(dlg, IDC_CALORIES, L"Ingresa las calorías por porción.", &cal)) return false;
  if (!ReadNumber(dlg, IDC_PROTEIN, L"Ingresa las proteínas por porción.", &prot)) return false;
  if (!ReadNumber(dlg, IDC_CARBS, L"Ingresa los carbohidratos por porción.", &carb)) return false;
  if (!ReadNumber(dlg, IDC_FAT, L"Ingresa las grasas por porción.", &fat)) return false;

  const int meal = static_cast<int>(SendDlgItemMessageW(dlg, IDC_MEAL, CB_GETCURSEL, 0, 0));
  e->meal = (meal >= 0 && meal < kMealCount) ? meal : 1;
  e->portion = quantity == 1.0 ? portion : util::FormatNumber(quantity, 2) + L" x " + portion;
  e->calories = cal * quantity;
  e->protein = prot * quantity;
  e->carbs = carb * quantity;
  e->fat = fat * quantity;
  return true;
}

INT_PTR CALLBACK AddFoodProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_INITDIALOG: {
      SetWindowLongPtrW(dlg, DWLP_USER, lp);
      const auto* ctx = reinterpret_cast<DialogContext*>(lp);

      HWND name = GetDlgItem(dlg, IDC_FOOD_NAME);
      for (size_t i = 0; i < kCatalogCount; ++i)
        SendMessageW(name, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(kCatalog[i].name));

      HWND meal = GetDlgItem(dlg, IDC_MEAL);
      for (int m = 0; m < kMealCount; ++m) SendMessageW(meal, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(MealLabel(m)));
      SendMessageW(meal, CB_SETCURSEL, static_cast<WPARAM>(ctx->meal), 0);

      SetDlgItemTextW(dlg, IDC_QUANTITY, L"1");
      SetFocus(name);
      return FALSE;  // ya pusimos el foco
    }

    case WM_COMMAND:
      switch (LOWORD(wp)) {
        case IDC_FOOD_NAME:
          if (HIWORD(wp) == CBN_SELCHANGE) {
            const auto i = static_cast<int>(SendDlgItemMessageW(dlg, IDC_FOOD_NAME, CB_GETCURSEL, 0, 0));
            if (i >= 0 && static_cast<size_t>(i) < kCatalogCount) FillFromCatalog(dlg, kCatalog[i]);
          }
          return TRUE;

        case IDOK: {
          auto* ctx = reinterpret_cast<DialogContext*>(GetWindowLongPtrW(dlg, DWLP_USER));
          FoodEntry entry;
          if (Collect(dlg, &entry)) {
            *ctx->out = entry;
            EndDialog(dlg, IDOK);
          }
          return TRUE;
        }

        case IDCANCEL:
          EndDialog(dlg, IDCANCEL);
          return TRUE;
      }
      break;
  }
  return FALSE;
}

}  // namespace

bool ShowAddFoodDialog(HWND owner, int initialMeal, FoodEntry* out) {
  DialogContext ctx{initialMeal, out};
  const INT_PTR r = DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_ADD_FOOD), owner, AddFoodProc,
                                    reinterpret_cast<LPARAM>(&ctx));
  return r == IDOK;
}
