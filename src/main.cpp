// NutryScan para Windows (C++ / Win32 + GDI+).
#include "gfx.h"

#include <commctrl.h>
#include <uxtheme.h>

#include <cmath>
#include <string>
#include <vector>

#include "add_food_dialog.h"
#include "report.h"
#include "resource.h"
#include "state.h"
#include "ui_draw.h"
#include "util.h"

using namespace Gdiplus;
using ui::Align;
using ui::Rc;
namespace col = ui::col;

namespace {

// ---------------------------------------------------------------- Constantes

enum Page { kInicio = 0, kRegistro, kInformes, kPerfil, kPageCount };
const wchar_t* kNavLabels[kPageCount] = {L"Inicio", L"Registro diario", L"Informes", L"Perfil y metas"};

enum ControlId {
  ID_DASH_ADD = 2001,
  ID_DASH_PRINT,
  ID_DIARY_PREV,
  ID_DIARY_DATE,
  ID_DIARY_NEXT,
  ID_DIARY_ADD,
  ID_DIARY_DELETE,
  ID_DIARY_LIST,
  ID_REPORT_DATE,
  ID_REPORT_NOTES,
  ID_REPORT_PRINT,
  ID_PROFILE_NAME,
  ID_PROFILE_EMAIL,
  ID_PROFILE_CAL,
  ID_PROFILE_PROT,
  ID_PROFILE_CARB,
  ID_PROFILE_FAT,
  ID_PROFILE_SAVE,
};

constexpr double kSidebarW = 240;
constexpr double kNavTop = 112;
constexpr double kNavItemH = 46;
constexpr double kNavGap = 4;

// ---------------------------------------------------------------- Estado global

HINSTANCE g_inst = nullptr;
HWND g_main = nullptr;
AppState g_state;
Page g_page = kInicio;
double g_scale = 1.0;
HFONT g_font = nullptr;
SYSTEMTIME g_diaryDate;
SYSTEMTIME g_reportDate;

struct PageControl {
  HWND hwnd;
  Page page;
};
std::vector<PageControl> g_controls;

HWND g_dashAdd, g_dashPrint;
HWND g_diaryPrev, g_diaryDatePicker, g_diaryNext, g_diaryAdd, g_diaryDelete, g_diaryList;
HWND g_reportDatePicker, g_reportNotes, g_reportPrint;
HWND g_profileEdits[6];
HWND g_profileSave;

int S(double v) { return static_cast<int>(std::lround(v * g_scale)); }

std::wstring WindowText(HWND h) {
  const int n = GetWindowTextLengthW(h);
  std::wstring s(static_cast<size_t>(n) + 1, L'\0');
  GetWindowTextW(h, &s[0], n + 1);
  s.resize(static_cast<size_t>(n));
  return s;
}

// ---------------------------------------------------------------- Creación de controles

HWND Track(HWND h, Page page) {
  SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
  g_controls.push_back({h, page});
  return h;
}

HWND MakeButton(int id, const wchar_t* text, bool primary, Page page) {
  HWND b = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_TABSTOP | BS_OWNERDRAW, 0, 0, 10, 10, g_main,
                           reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), g_inst, nullptr);
  SetWindowLongPtrW(b, GWLP_USERDATA, primary ? 1 : 0);
  return Track(b, page);
}

HWND MakeEdit(int id, DWORD extraStyle, Page page) {
  HWND e = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_TABSTOP | extraStyle, 0, 0, 10, 10, g_main,
                           reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), g_inst, nullptr);
  return Track(e, page);
}

HWND MakeDatePicker(int id, Page page) {
  HWND d = CreateWindowExW(0, DATETIMEPICK_CLASSW, L"", WS_CHILD | WS_TABSTOP | DTS_SHORTDATEFORMAT, 0, 0, 10, 10,
                           g_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), g_inst, nullptr);
  SendMessageW(d, DTM_SETMCFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
  SYSTEMTIME range[2] = {};
  range[1] = util::Today();
  DateTime_SetRange(d, GDTR_MAX, range);
  return Track(d, page);
}

void CreateControls() {
  g_dashPrint = MakeButton(ID_DASH_PRINT, L"Imprimir informe de hoy", false, kInicio);
  g_dashAdd = MakeButton(ID_DASH_ADD, L"+ Agregar alimento", true, kInicio);

  g_diaryPrev = MakeButton(ID_DIARY_PREV, L"‹", false, kRegistro);
  g_diaryDatePicker = MakeDatePicker(ID_DIARY_DATE, kRegistro);
  g_diaryNext = MakeButton(ID_DIARY_NEXT, L"›", false, kRegistro);
  g_diaryAdd = MakeButton(ID_DIARY_ADD, L"+ Agregar alimento", true, kRegistro);
  g_diaryDelete = MakeButton(ID_DIARY_DELETE, L"Eliminar seleccionado", false, kRegistro);

  g_diaryList = CreateWindowExW(0, WC_LISTVIEWW, L"",
                                WS_CHILD | WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                0, 0, 10, 10, g_main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_DIARY_LIST)),
                                g_inst, nullptr);
  ListView_SetExtendedListViewStyle(g_diaryList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
  SetWindowTheme(g_diaryList, L"Explorer", nullptr);
  Track(g_diaryList, kRegistro);
  const wchar_t* cols[7] = {L"Comida", L"Alimento", L"Porción", L"kcal", L"Proteínas (g)", L"Carbohidratos (g)", L"Grasas (g)"};
  for (int i = 0; i < 7; ++i) {
    LVCOLUMNW c = {};
    c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    c.fmt = i >= 3 ? LVCFMT_RIGHT : LVCFMT_LEFT;
    c.cx = 100;
    c.pszText = const_cast<LPWSTR>(cols[i]);
    ListView_InsertColumn(g_diaryList, i, &c);
  }

  g_reportDatePicker = MakeDatePicker(ID_REPORT_DATE, kInformes);
  g_reportNotes = MakeEdit(ID_REPORT_NOTES, ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL, kInformes);
  g_reportPrint = MakeButton(ID_REPORT_PRINT, L"Imprimir o guardar PDF", true, kInformes);

  const int ids[6] = {ID_PROFILE_NAME, ID_PROFILE_EMAIL, ID_PROFILE_CAL, ID_PROFILE_PROT, ID_PROFILE_CARB, ID_PROFILE_FAT};
  for (int i = 0; i < 6; ++i) g_profileEdits[i] = MakeEdit(ids[i], ES_AUTOHSCROLL, kPerfil);
  g_profileSave = MakeButton(ID_PROFILE_SAVE, L"Guardar cambios", true, kPerfil);
}

// ---------------------------------------------------------------- Geometría

struct Content {
  double w, h;      // área cliente en DIP
  double x0, right; // columna de contenido
  double cw;        // ancho de contenido
};

Content GetContent() {
  RECT rc;
  GetClientRect(g_main, &rc);
  Content c;
  c.w = rc.right / g_scale;
  c.h = rc.bottom / g_scale;
  c.x0 = kSidebarW + 40;
  c.right = c.w - 40;
  c.cw = c.right - c.x0;
  return c;
}

struct ProfileGeom {
  double colX[2];
  double colW;
  double labelY[3];
};

ProfileGeom GetProfileGeom(const Content& c) {
  ProfileGeom p;
  p.colW = (c.cw - 44 - 24) / 2;
  p.colX[0] = c.x0 + 22;
  p.colX[1] = c.x0 + 22 + p.colW + 24;
  p.labelY[0] = 160;
  p.labelY[1] = 236;
  p.labelY[2] = 312;
  return p;
}

RECT PreviewRect(const Content& c) {  // en DIP
  const double ax = c.x0 + 360, ay = 104, aw = c.right - ax, ah = c.h - 24 - ay;
  double pw = std::min(aw, ah * kPageWidth / kPageHeight);
  if (pw < 50) pw = 50;
  const double ph = pw * kPageHeight / kPageWidth;
  RECT r;
  r.left = static_cast<LONG>(ax + (aw - pw) / 2);
  r.top = static_cast<LONG>(ay);
  r.right = static_cast<LONG>(r.left + pw);
  r.bottom = static_cast<LONG>(ay + ph);
  return r;
}

void Layout() {
  const Content c = GetContent();
  auto place = [](HWND h, double x, double y, double w, double hh) { MoveWindow(h, S(x), S(y), S(w), S(hh), TRUE); };

  // Inicio
  place(g_dashAdd, c.right - 196, 36, 196, 42);
  place(g_dashPrint, c.right - 196 - 12 - 220, 36, 220, 42);

  // Registro
  place(g_diaryAdd, c.right - 196, 36, 196, 42);
  place(g_diaryNext, c.right - 196 - 12 - 42, 36, 42, 42);
  place(g_diaryDatePicker, c.right - 196 - 12 - 42 - 8 - 150, 41, 150, 32);
  place(g_diaryPrev, c.right - 196 - 12 - 42 - 8 - 150 - 8 - 42, 36, 42, 42);
  place(g_diaryList, c.x0, 200, c.cw, c.h - 200 - 78);
  place(g_diaryDelete, c.right - 220, c.h - 62, 220, 42);

  const double lw = c.cw - 4;
  const double w1 = lw * 0.13, w2 = lw * 0.25, w3 = lw * 0.19, w4 = lw * 0.09, w5 = lw * 0.11, w6 = lw * 0.12;
  const double widths[7] = {w1, w2, w3, w4, w5, w6, lw - w1 - w2 - w3 - w4 - w5 - w6};
  for (int i = 0; i < 7; ++i) ListView_SetColumnWidth(g_diaryList, i, S(widths[i]));

  // Informes
  place(g_reportDatePicker, c.x0, 134, 320, 32);
  place(g_reportNotes, c.x0, 220, 320, 160);
  place(g_reportPrint, c.x0, 396, 320, 44);

  // Perfil
  const ProfileGeom p = GetProfileGeom(c);
  for (int i = 0; i < 6; ++i) place(g_profileEdits[i], p.colX[i % 2], p.labelY[i / 2] + 22, p.colW, 32);
  place(g_profileSave, c.right - 200, 400, 200, 44);
}

// ---------------------------------------------------------------- Datos de pantalla

ReportData BuildReport(const SYSTEMTIME& date, const std::wstring& notes) {
  ReportData r;
  r.profile = g_state.profile;
  r.date = date;
  r.entries = g_state.EntriesFor(util::DateKey(date));
  r.notes = notes;
  return r;
}

bool SaveOrWarn() {
  if (g_state.Save()) return true;
  MessageBoxW(g_main, (L"No se pudieron guardar los datos en:\n" + g_state.DataPath()).c_str(), L"NutryScan",
              MB_ICONERROR);
  return false;
}

void RefreshDiaryList() {
  ListView_DeleteAllItems(g_diaryList);
  const auto entries = g_state.EntriesFor(util::DateKey(g_diaryDate));
  int row = 0;
  for (const auto& e : entries) {
    LVITEMW it = {};
    it.mask = LVIF_TEXT | LVIF_PARAM;
    it.iItem = row;
    it.pszText = const_cast<LPWSTR>(MealLabel(e.meal));
    it.lParam = static_cast<LPARAM>(e.id);
    ListView_InsertItem(g_diaryList, &it);
    const std::wstring cells[6] = {e.name,
                                   e.portion,
                                   util::FormatNumber(e.calories, 0),
                                   util::FormatNumber(e.protein, 1),
                                   util::FormatNumber(e.carbs, 1),
                                   util::FormatNumber(e.fat, 1)};
    for (int c = 0; c < 6; ++c) ListView_SetItemText(g_diaryList, row, c + 1, const_cast<LPWSTR>(cells[c].c_str()));
    ++row;
  }
  const bool isToday = util::DateKey(g_diaryDate) == util::DateKey(util::Today());
  EnableWindow(g_diaryNext, !isToday);
}

void LoadProfileFields() {
  const Profile& p = g_state.profile;
  SetWindowTextW(g_profileEdits[0], p.name.c_str());
  SetWindowTextW(g_profileEdits[1], p.email.c_str());
  SetWindowTextW(g_profileEdits[2], util::PlainNumber(p.calorieGoal).c_str());
  SetWindowTextW(g_profileEdits[3], util::PlainNumber(p.proteinGoal).c_str());
  SetWindowTextW(g_profileEdits[4], util::PlainNumber(p.carbsGoal).c_str());
  SetWindowTextW(g_profileEdits[5], util::PlainNumber(p.fatGoal).c_str());
}

void ShowPage(Page page) {
  g_page = page;
  for (const auto& pc : g_controls) ShowWindow(pc.hwnd, pc.page == page ? SW_SHOW : SW_HIDE);
  if (page == kRegistro) {
    DateTime_SetSystemtime(g_diaryDatePicker, GDT_VALID, &g_diaryDate);
    RefreshDiaryList();
  } else if (page == kPerfil) {
    LoadProfileFields();
  }
  InvalidateRect(g_main, nullptr, FALSE);
}

// ---------------------------------------------------------------- Acciones

void AddFood(const SYSTEMTIME& date) {
  FoodEntry entry;
  if (!ShowAddFoodDialog(g_main, 1, &entry)) return;
  entry.date = util::DateKey(date);
  g_state.Add(entry);
  SaveOrWarn();
  if (g_page == kRegistro) RefreshDiaryList();
  InvalidateRect(g_main, nullptr, FALSE);
}

void DeleteSelected() {
  const int sel = ListView_GetNextItem(g_diaryList, -1, LVNI_SELECTED);
  if (sel < 0) {
    MessageBoxW(g_main, L"Selecciona un alimento de la lista para eliminarlo.", L"NutryScan", MB_ICONINFORMATION);
    return;
  }
  LVITEMW it = {};
  it.mask = LVIF_PARAM;
  it.iItem = sel;
  ListView_GetItem(g_diaryList, &it);
  if (MessageBoxW(g_main, L"¿Eliminar este alimento del registro?", L"NutryScan", MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;
  if (g_state.Remove(static_cast<long long>(it.lParam))) SaveOrWarn();
  RefreshDiaryList();
  InvalidateRect(g_main, nullptr, FALSE);
}

void SaveProfile() {
  Profile p;
  p.name = util::Trim(WindowText(g_profileEdits[0]));
  p.email = util::Trim(WindowText(g_profileEdits[1]));
  if (p.name.empty()) {
    MessageBoxW(g_main, L"Escribe tu nombre.", L"Revisa los datos", MB_ICONWARNING);
    SetFocus(g_profileEdits[0]);
    return;
  }
  double* goals[4] = {&p.calorieGoal, &p.proteinGoal, &p.carbsGoal, &p.fatGoal};
  for (int i = 0; i < 4; ++i) {
    double v = 0;
    if (!util::ParseNumber(WindowText(g_profileEdits[i + 2]), &v) || v <= 0) {
      MessageBoxW(g_main, L"Las metas deben ser números mayores a 0.", L"Revisa los datos", MB_ICONWARNING);
      SetFocus(g_profileEdits[i + 2]);
      return;
    }
    *goals[i] = v;
  }
  g_state.profile = p;
  if (SaveOrWarn()) MessageBoxW(g_main, L"Perfil guardado.", L"NutryScan", MB_ICONINFORMATION);
  InvalidateRect(g_main, nullptr, FALSE);
}

// ---------------------------------------------------------------- Pintado

void PageTitle(Graphics& g, const Content& c, const std::wstring& title, const std::wstring& subtitle,
               double reserveRight) {
  ui::Text(g, title, Rc(c.x0, 30, c.cw - reserveRight, 40), 28.0f, true, col::Ink());
  ui::Text(g, subtitle, Rc(c.x0, 70, c.cw - reserveRight, 22), 14.5f, false, col::Muted());
}

void PaintSidebar(Graphics& g, double h) {
  ui::FillRect(g, Rc(0, 0, kSidebarW, h), col::GreenDark());
  ui::Logo(g, 18, 28, 42);
  ui::Text(g, L"NutryScan", Rc(72, 28, 160, 26), 19.0f, true, col::White());
  ui::Text(g, L"Escritorio", Rc(72, 53, 160, 18), 12.0f, false, col::SideMuted());

  for (int i = 0; i < kPageCount; ++i) {
    const double y = kNavTop + i * (kNavItemH + kNavGap);
    const bool active = i == g_page;
    if (active) ui::FillRound(g, Rc(16, y, kSidebarW - 32, kNavItemH), 10, col::Rgb(255, 255, 255, 38));
    if (active) ui::FillRound(g, Rc(16, y + 12, 4, kNavItemH - 24), 2, col::White());
    ui::Text(g, kNavLabels[i], Rc(36, y, kSidebarW - 60, kNavItemH), 14.5f, active,
             active ? col::White() : col::SideText(), Align::Left, false, true);
  }

  ui::HLine(g, 16, static_cast<float>(kSidebarW - 16), static_cast<float>(h - 72), col::Rgb(255, 255, 255, 50));
  ui::Text(g, L"RiveraSystems", Rc(16, h - 60, kSidebarW - 32, 18), 12.5f, true, col::White());
  ui::Text(g, L"nutryscan.contacto@gmail.com", Rc(16, h - 40, kSidebarW - 32, 18), 11.5f, false, col::SideMuted());
}

void PaintDashboard(Graphics& g, const Content& c) {
  const SYSTEMTIME today = util::Today();
  const std::wstring key = util::DateKey(today);
  const auto entries = g_state.EntriesFor(key);
  const Totals t = SumEntries(entries);
  const Profile& p = g_state.profile;

  PageTitle(g, c, L"Hola, " + p.name, util::DateLong(today), 460);

  const double colW = (c.cw - 20) / 2;
  const double rowY = 104, rowH = 192;

  // Calorías
  ui::Panel(g, Rc(c.x0, rowY, colW, rowH), L"Calorías de hoy");
  const float ringR = 58;
  const float cx = static_cast<float>(c.x0 + 22 + ringR + 6);
  const float cy = static_cast<float>(rowY + 56 + ringR + 4);
  ui::CalorieRing(g, cx, cy, ringR, t.calories, p.calorieGoal);
  const double sx = c.x0 + 22 + ringR * 2 + 40;
  const double remaining = p.calorieGoal - t.calories;
  struct Stat { const wchar_t* label; std::wstring value; Color color; };
  const Stat stats[3] = {
      {L"Meta diaria", util::Kcal(p.calorieGoal), col::Ink()},
      {L"Consumidas", util::Kcal(t.calories), col::Ink()},
      {remaining < 0 ? L"Sobre la meta" : L"Te quedan", util::Kcal(std::fabs(remaining)),
       remaining < 0 ? col::Amber() : col::Green()},
  };
  for (int i = 0; i < 3; ++i) {
    const double y = rowY + 56 + i * 44;
    ui::Text(g, stats[i].label, Rc(sx, y, colW - (sx - c.x0) - 16, 18), 12.5f, false, col::Muted());
    ui::Text(g, stats[i].value, Rc(sx, y + 16, colW - (sx - c.x0) - 16, 24), 17.0f, true, stats[i].color);
  }

  // Macros
  const double mx = c.x0 + colW + 20;
  ui::Panel(g, Rc(mx, rowY, colW, rowH), L"Macronutrientes");
  const double bw = colW - 44;
  ui::MacroBar(g, Rc(mx + 22, rowY + 58, bw, 40), L"Proteínas", t.protein, p.proteinGoal, col::Protein());
  ui::MacroBar(g, Rc(mx + 22, rowY + 100, bw, 40), L"Carbohidratos", t.carbs, p.carbsGoal, col::Carbs());
  ui::MacroBar(g, Rc(mx + 22, rowY + 142, bw, 40), L"Grasas", t.fat, p.fatGoal, col::Fat());

  // Últimos 7 días
  const double wy = rowY + rowH + 16, wh = 192;
  ui::Panel(g, Rc(c.x0, wy, c.cw, wh), L"Últimos 7 días");
  double values[7];
  SYSTEMTIME days[7];
  double maxV = std::max(p.calorieGoal, 1.0);
  for (int i = 0; i < 7; ++i) {
    days[i] = util::AddDays(today, i - 6);
    values[i] = g_state.TotalsFor(util::DateKey(days[i])).calories;
    maxV = std::max(maxV, values[i]);
  }
  const double chartX = c.x0 + 22, chartW = c.cw - 44 - 44;
  const double chartTop = wy + 54, chartBottom = wy + wh - 30, chartH = chartBottom - chartTop;
  const double goalY = chartBottom - p.calorieGoal / maxV * chartH;
  {
    Pen dash(col::Amber(), 1.0f);
    dash.SetDashStyle(DashStyleDash);
    g.DrawLine(&dash, static_cast<REAL>(chartX), static_cast<REAL>(goalY), static_cast<REAL>(chartX + chartW + 6),
               static_cast<REAL>(goalY));
    ui::Text(g, L"Meta", Rc(chartX + chartW + 10, goalY - 9, 40, 16), 11.0f, false, col::Amber());
  }
  const double slot = chartW / 7, barW = std::min(slot * 0.62, 90.0);
  for (int i = 0; i < 7; ++i) {
    const double h = std::max(2.0, values[i] / maxV * chartH);
    const double bx = chartX + slot * i + (slot - barW) / 2;
    ui::FillRound(g, Rc(bx, chartBottom - h, barW, h), 5, values[i] > p.calorieGoal ? col::Amber() : col::Green());
    ui::Text(g, util::WeekdayShort(days[i]), Rc(chartX + slot * i, chartBottom + 6, slot, 18), 12.0f, false,
             col::Muted(), Align::Center);
  }

  // Registrado hoy
  const double ry = wy + wh + 16, rh = c.h - 24 - ry;
  if (rh < 90) return;
  ui::Panel(g, Rc(c.x0, ry, c.cw, rh), L"Registrado hoy");
  if (entries.empty()) {
    ui::Text(g, L"Aún no registras alimentos hoy. Usa \"+ Agregar alimento\" para comenzar.",
             Rc(c.x0 + 22, ry + 56, c.cw - 44, 20), 14.0f, false, col::Muted());
    return;
  }
  const int maxRows = static_cast<int>((rh - 64) / 40);
  int shown = 0;
  for (auto it = entries.rbegin(); it != entries.rend() && shown < maxRows; ++it, ++shown) {
    const double y = ry + 52 + shown * 40;
    if (shown > 0) ui::HLine(g, static_cast<float>(c.x0 + 22), static_cast<float>(c.right - 22), static_cast<float>(y), col::Line());
    ui::Text(g, it->name, Rc(c.x0 + 22, y + 4, c.cw * 0.4, 18), 14.0f, true, col::Ink());
    ui::Text(g, it->portion + L" · " + MealLabel(it->meal), Rc(c.x0 + 22, y + 21, c.cw * 0.4, 16), 12.0f, false,
             col::Muted());
    ui::Text(g, util::Kcal(it->calories), Rc(c.x0 + c.cw * 0.42, y, c.cw * 0.18, 40), 14.0f, true, col::Ink(),
             Align::Right, false, true);
    ui::Text(g,
             L"P " + util::FormatNumber(it->protein, 1) + L" g · C " + util::FormatNumber(it->carbs, 1) + L" g · G " +
                 util::FormatNumber(it->fat, 1) + L" g",
             Rc(c.x0 + c.cw * 0.6, y, c.cw * 0.4 - 22, 40), 12.5f, false, col::Muted(), Align::Right, false, true);
  }
}

void PaintDiary(Graphics& g, const Content& c) {
  PageTitle(g, c, L"Registro diario", util::DateLong(g_diaryDate), 520);
  const Totals t = g_state.TotalsFor(util::DateKey(g_diaryDate));
  const double goal = g_state.profile.calorieGoal;

  ui::Panel(g, Rc(c.x0, 104, c.cw, 80), L"");
  const double w = (c.cw - 44) / 4;
  struct Item { const wchar_t* label; std::wstring value; Color color; };
  const Item items[4] = {
      {L"Calorías", util::FormatNumber(t.calories, 0) + L" / " + util::Kcal(goal),
       t.calories > goal ? col::Amber() : col::Green()},
      {L"Proteínas", util::Grams(t.protein), col::Protein()},
      {L"Carbohidratos", util::Grams(t.carbs), col::Carbs()},
      {L"Grasas", util::Grams(t.fat), col::Fat()},
  };
  for (int i = 0; i < 4; ++i) {
    ui::Text(g, items[i].label, Rc(c.x0 + 22 + w * i, 120, w - 12, 18), 12.5f, false, col::Muted());
    ui::Text(g, items[i].value, Rc(c.x0 + 22 + w * i, 138, w - 12, 30), 20.0f, true, items[i].color);
  }
  ui::Text(g, L"Tip: selecciona un alimento y presiona Supr para eliminarlo.", Rc(c.x0, c.h - 52, c.cw - 240, 20), 12.5f,
           false, col::Muted());
}

void PaintReports(Graphics& g, const Content& c) {
  PageTitle(g, c, L"Informes", L"Genera un informe profesional para imprimir o guardar como PDF.", 0);

  const auto entries = g_state.EntriesFor(util::DateKey(g_reportDate));
  const Totals t = SumEntries(entries);
  ui::Text(g, L"Día del informe", Rc(c.x0, 110, 320, 20), 13.0f, true, col::Muted());
  ui::Text(g,
           entries.empty() ? std::wstring(L"Sin alimentos registrados este día.")
                           : std::to_wstring(entries.size()) + L" alimentos · " + util::Kcal(t.calories),
           Rc(c.x0, 170, 320, 18), 12.5f, false, col::Muted());
  ui::Text(g, L"Observaciones (opcional)", Rc(c.x0, 196, 320, 20), 13.0f, true, col::Muted());
  ui::Text(g, L"Para guardar como PDF, elige la impresora \"Microsoft Print to PDF\" en el diálogo de impresión.",
           Rc(c.x0, 452, 320, 60), 12.5f, false, col::Muted(), Align::Left, true);

  // Vista previa de la primera página
  const RECT pr = PreviewRect(c);
  const double pw = pr.right - pr.left;
  ui::FillRound(g, Rc(pr.left + 3, pr.top + 4, pw, pr.bottom - pr.top), 2, col::Rgb(0, 0, 0, 18));
  const ReportData data = BuildReport(g_reportDate, WindowText(g_reportNotes));
  const GraphicsState saved = g.Save();
  g.TranslateTransform(static_cast<REAL>(pr.left), static_cast<REAL>(pr.top));
  const REAL s = static_cast<REAL>(pw / kPageWidth);
  g.ScaleTransform(s, s);
  g.SetClip(RectF(0, 0, kPageWidth, kPageHeight));
  DrawReportPage(g, data, 0);
  g.Restore(saved);

  const int pages = ReportPageCount(data);
  if (pages > 1) {
    ui::Text(g, L"Vista previa: página 1 de " + std::to_wstring(pages), Rc(pr.left, pr.top - 22, pw, 18), 12.0f, false,
             col::Muted(), Align::Right);
  }
}

void PaintProfile(Graphics& g, const Content& c) {
  PageTitle(g, c, L"Perfil y metas", L"Estos datos aparecen en tus informes impresos.", 0);
  ui::Panel(g, Rc(c.x0, 104, c.cw, 280), L"Datos y metas diarias");
  const ProfileGeom p = GetProfileGeom(c);
  const wchar_t* labels[6] = {L"Nombre", L"Correo (opcional)", L"Calorías (kcal)", L"Proteínas (g)",
                              L"Carbohidratos (g)", L"Grasas (g)"};
  for (int i = 0; i < 6; ++i) {
    ui::Text(g, labels[i], Rc(p.colX[i % 2], p.labelY[i / 2], p.colW, 20), 13.0f, true, col::Muted());
  }
  ui::Text(g, L"Consulta tus metas con un nutricionista si tienes dudas.", Rc(c.x0, 412, c.cw - 240, 20), 12.5f, false,
           col::Muted());
}

void OnPaint(HWND hwnd) {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hwnd, &ps);
  RECT rc;
  GetClientRect(hwnd, &rc);
  if (rc.right > 0 && rc.bottom > 0) {
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HGDIOBJ old = SelectObject(mem, bmp);
    {
      Graphics g(mem);
      g.SetSmoothingMode(SmoothingModeAntiAlias);
      g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
      g.SetPixelOffsetMode(PixelOffsetModeHalf);
      g.Clear(col::Bg());
      g.ScaleTransform(static_cast<REAL>(g_scale), static_cast<REAL>(g_scale));

      const Content c = GetContent();
      PaintSidebar(g, c.h);
      switch (g_page) {
        case kInicio: PaintDashboard(g, c); break;
        case kRegistro: PaintDiary(g, c); break;
        case kInformes: PaintReports(g, c); break;
        case kPerfil: PaintProfile(g, c); break;
        default: break;
      }
    }
    BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteDC(mem);
  }
  EndPaint(hwnd, &ps);
}

void DrawButton(const DRAWITEMSTRUCT* di) {
  const bool primary = GetWindowLongPtrW(di->hwndItem, GWLP_USERDATA) == 1;
  const bool pressed = (di->itemState & ODS_SELECTED) != 0;
  const bool disabled = (di->itemState & ODS_DISABLED) != 0;
  const bool focused = (di->itemState & ODS_FOCUS) != 0;
  const std::wstring text = WindowText(di->hwndItem);

  const RECT r = di->rcItem;
  Graphics g(di->hDC);
  g.SetSmoothingMode(SmoothingModeAntiAlias);
  g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
  g.SetPixelOffsetMode(PixelOffsetModeHalf);
  g.Clear(col::Bg());
  g.ScaleTransform(static_cast<REAL>(g_scale), static_cast<REAL>(g_scale));

  const double w = (r.right - r.left) / g_scale, h = (r.bottom - r.top) / g_scale;
  const RectF box = Rc(0.5, 0.5, w - 1, h - 1);
  Color fill = primary ? (pressed ? col::GreenPressed() : col::Green()) : (pressed ? col::Sage() : col::White());
  Color ink = primary ? col::White() : col::Ink();
  if (disabled) {
    fill = primary ? col::Rgb(0x2F, 0x6B, 0x4F, 110) : col::White();
    ink = col::Rgb(0x6B, 0x7A, 0x72, 140);
  }
  ui::FillRound(g, box, 10, fill);
  if (!primary) ui::StrokeRound(g, box, 10, col::Line());
  if (focused && !pressed) ui::StrokeRound(g, Rc(2, 2, w - 4, h - 4), 8, primary ? col::Rgb(255, 255, 255, 120) : col::Green());
  const float size = (text == L"‹" || text == L"›") ? 20.0f : 14.0f;
  ui::Text(g, text, Rc(6, 0, w - 12, h), size, true, ink, Align::Center, false, true);
}

int NavHit(int px, int py) {
  const double x = px / g_scale, y = py / g_scale;
  if (x < 16 || x > kSidebarW - 16) return -1;
  for (int i = 0; i < kPageCount; ++i) {
    const double top = kNavTop + i * (kNavItemH + kNavGap);
    if (y >= top && y <= top + kNavItemH) return i;
  }
  return -1;
}

// ---------------------------------------------------------------- Ventana

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_CREATE:
      g_main = hwnd;
      CreateControls();
      Layout();
      ShowPage(kInicio);
      return 0;

    case WM_SIZE:
      Layout();
      InvalidateRect(hwnd, nullptr, FALSE);
      return 0;

    case WM_GETMINMAXINFO: {
      auto* mm = reinterpret_cast<MINMAXINFO*>(lp);
      mm->ptMinTrackSize.x = S(1180);
      mm->ptMinTrackSize.y = S(820);
      return 0;
    }

    case WM_ERASEBKGND:
      return 1;

    case WM_PAINT:
      OnPaint(hwnd);
      return 0;

    case WM_DRAWITEM: {
      const auto* di = reinterpret_cast<const DRAWITEMSTRUCT*>(lp);
      if (di->CtlType == ODT_BUTTON) {
        DrawButton(di);
        return TRUE;
      }
      break;
    }

    case WM_LBUTTONDOWN: {
      const int hit = NavHit(static_cast<short>(LOWORD(lp)), static_cast<short>(HIWORD(lp)));
      if (hit >= 0 && hit != g_page) ShowPage(static_cast<Page>(hit));
      return 0;
    }

    case WM_SETCURSOR:
      if (LOWORD(lp) == HTCLIENT) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);
        if (NavHit(pt.x, pt.y) >= 0) {
          SetCursor(LoadCursorW(nullptr, IDC_HAND));
          return TRUE;
        }
      }
      break;

    case WM_COMMAND:
      switch (LOWORD(wp)) {
        case ID_DASH_ADD: AddFood(util::Today()); return 0;
        case ID_DIARY_ADD: AddFood(g_diaryDate); return 0;
        case ID_DASH_PRINT: PrintReport(hwnd, BuildReport(util::Today(), L"")); return 0;
        case ID_DIARY_PREV:
        case ID_DIARY_NEXT: {
          const SYSTEMTIME next = util::AddDays(g_diaryDate, LOWORD(wp) == ID_DIARY_PREV ? -1 : 1);
          if (util::DateKey(next) <= util::DateKey(util::Today())) {
            g_diaryDate = next;
            DateTime_SetSystemtime(g_diaryDatePicker, GDT_VALID, &g_diaryDate);
            RefreshDiaryList();
            InvalidateRect(hwnd, nullptr, FALSE);
          }
          return 0;
        }
        case ID_DIARY_DELETE: DeleteSelected(); return 0;
        case ID_REPORT_PRINT: PrintReport(hwnd, BuildReport(g_reportDate, WindowText(g_reportNotes))); return 0;
        case ID_PROFILE_SAVE: SaveProfile(); return 0;
        case ID_REPORT_NOTES:
          if (HIWORD(wp) == EN_CHANGE) InvalidateRect(hwnd, nullptr, FALSE);
          return 0;
      }
      break;

    case WM_NOTIFY: {
      const auto* hdr = reinterpret_cast<const NMHDR*>(lp);
      if (hdr->code == DTN_DATETIMECHANGE) {
        const auto* dt = reinterpret_cast<const NMDATETIMECHANGE*>(lp);
        if (dt->dwFlags == GDT_VALID) {
          const SYSTEMTIME picked = util::AddDays(dt->st, 0);
          if (hdr->idFrom == ID_DIARY_DATE) {
            g_diaryDate = picked;
            RefreshDiaryList();
          } else if (hdr->idFrom == ID_REPORT_DATE) {
            g_reportDate = picked;
          }
          InvalidateRect(hwnd, nullptr, FALSE);
        }
      } else if (hdr->idFrom == ID_DIARY_LIST && hdr->code == LVN_KEYDOWN) {
        const auto* kd = reinterpret_cast<const NMLVKEYDOWN*>(lp);
        if (kd->wVKey == VK_DELETE) DeleteSelected();
      }
      break;
    }

    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProcW(hwnd, msg, wp, lp);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCmd) {
  g_inst = instance;

  INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_DATE_CLASSES | ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES};
  InitCommonControlsEx(&icc);

  GdiplusStartupInput gdiplusInput;
  ULONG_PTR gdiplusToken = 0;
  GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr);

  HDC screen = GetDC(nullptr);
  g_scale = GetDeviceCaps(screen, LOGPIXELSX) / 96.0;
  ReleaseDC(nullptr, screen);
  if (g_scale < 1.0) g_scale = 1.0;

  g_font = CreateFontW(-S(15), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

  g_state.Load();
  g_diaryDate = util::Today();
  g_reportDate = util::Today();

  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = instance;
  wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP));
  wc.hIconSm = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON, 16, 16, 0));
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.lpszClassName = L"NutryScanMainWindow";
  RegisterClassExW(&wc);

  const int w = S(1320), h = S(880);
  RECT work;
  SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
  const int x = work.left + std::max(0, static_cast<int>((work.right - work.left - w) / 2));
  const int y = work.top + std::max(0, static_cast<int>((work.bottom - work.top - h) / 2));

  HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"NutryScan", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, x, y, w, h,
                              nullptr, nullptr, instance, nullptr);
  if (!hwnd) return 1;
  ShowWindow(hwnd, showCmd);
  UpdateWindow(hwnd);

  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    if (!IsDialogMessageW(hwnd, &msg)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }

  DeleteObject(g_font);
  GdiplusShutdown(gdiplusToken);
  return static_cast<int>(msg.wParam);
}
