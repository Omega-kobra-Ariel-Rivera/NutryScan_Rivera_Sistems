#include "report.h"

#include <commdlg.h>

#include <cmath>

#include "ui_draw.h"
#include "util.h"

using namespace Gdiplus;
using ui::Align;
using ui::Rc;
namespace col = ui::col;

namespace {

constexpr float M = 42.0f;                 // margen
constexpr float CW = kPageWidth - 2 * M;   // ancho útil
constexpr float kRowH = 19.0f;
constexpr float kHeaderH = 20.0f;
constexpr float kTableEnd = 600.0f;        // deja espacio para observaciones y pie
constexpr float kFirstTableY = 266.0f;
constexpr float kNextTableY = 124.0f;

int RowsFirstPage() { return static_cast<int>((kTableEnd - kFirstTableY - kHeaderH) / kRowH); }
int RowsNextPages() { return static_cast<int>((kTableEnd - kNextTableY - kHeaderH) / kRowH); }

const std::wstring kContact = L"nutryscan.contacto@gmail.com  |  WhatsApp +56 9 50265723  |  riverasystems.netlify.app";

void Header(Graphics& g, const ReportData& d) {
  ui::Logo(g, M, 38, 40);
  ui::Text(g, L"NutryScan", Rc(M + 52, 36, 300, 28), 21.0f, true, col::Green());
  ui::Text(g, L"Informe nutricional diario", Rc(M + 52, 62, 300, 16), 10.5f, false, col::Muted());
  ui::Text(g, util::DateShort(d.date), Rc(M, 48, CW, 18), 11.0f, true, col::Ink(), Align::Right);
  ui::HLine(g, M, M + CW, 90, col::Green(), 2.0f);
}

void Footer(Graphics& g, int page, int pages) {
  ui::HLine(g, M, M + CW, kPageHeight - M - 14, col::Line(), 0.8f);
  ui::Text(g, kContact, Rc(M, kPageHeight - M - 8, CW - 80, 14), 8.0f, false, col::Muted());
  ui::Text(g, L"Página " + std::to_wstring(page + 1) + L" de " + std::to_wstring(pages),
           Rc(M + CW - 90, kPageHeight - M - 8, 90, 14), 8.0f, false, col::Muted(), Align::Right);
}

void Summary(Graphics& g, const ReportData& d) {
  const Totals t = SumEntries(d.entries);
  const Profile& p = d.profile;

  // Datos del usuario
  const float colW = CW / 3;
  const std::wstring labels[3] = {L"Nombre", L"Correo", L"Meta diaria"};
  const std::wstring values[3] = {p.name, p.email.empty() ? L"-" : p.email, util::Kcal(p.calorieGoal)};
  for (int i = 0; i < 3; ++i) {
    ui::Text(g, labels[i], Rc(M + colW * i, 104, colW - 8, 12), 8.5f, false, col::Muted());
    ui::Text(g, values[i], Rc(M + colW * i, 117, colW - 8, 16), 11.0f, true, col::Ink());
  }

  // Tarjetas de totales
  const float cardW = (CW - 24) / 4;
  struct Card { const wchar_t* label; std::wstring value; std::wstring goal; };
  const Card cards[4] = {
      {L"Calorías", util::Kcal(t.calories), util::Kcal(p.calorieGoal)},
      {L"Proteínas", util::Grams(t.protein), util::Grams(p.proteinGoal)},
      {L"Carbohidratos", util::Grams(t.carbs), util::Grams(p.carbsGoal)},
      {L"Grasas", util::Grams(t.fat), util::Grams(p.fatGoal)},
  };
  for (int i = 0; i < 4; ++i) {
    const bool isMain = i == 0;
    const RectF r = Rc(M + (cardW + 8) * i, 142, cardW, 56);
    ui::FillRound(g, r, 6, isMain ? col::Green() : col::Sage());
    const Color soft = isMain ? col::White() : col::Muted();
    const Color strong = isMain ? col::White() : col::Ink();
    ui::Text(g, cards[i].label, Rc(r.X + 10, r.Y + 8, r.Width - 20, 12), 8.5f, false, soft);
    ui::Text(g, cards[i].value, Rc(r.X + 10, r.Y + 20, r.Width - 20, 20), 14.5f, true, strong);
    ui::Text(g, L"Meta: " + cards[i].goal, Rc(r.X + 10, r.Y + 40, r.Width - 20, 12), 8.0f, false, soft);
  }

  // Barra de avance
  const double pct = p.calorieGoal > 0 ? t.calories / p.calorieGoal * 100.0 : 0.0;
  const bool over = pct > 100.0;
  ui::Text(g, std::wstring(over ? L"Superó la meta calórica: " : L"Avance de la meta calórica: ") +
                  util::FormatNumber(pct, 1) + L" %",
           Rc(M, 208, CW, 14), 9.5f, false, over ? col::Amber() : col::Ink());
  const RectF bar = Rc(M, 224, CW, 7);
  ui::FillRound(g, bar, 3.5f, col::Sage());
  const double fill = std::min(pct / 100.0, 1.0);
  if (fill > 0) ui::FillRound(g, Rc(M, 224, std::max(7.0, CW * fill), 7), 3.5f, over ? col::Amber() : col::Green());

  ui::Text(g, L"Registro de alimentos", Rc(M, 242, CW, 18), 12.5f, true, col::Ink());
}

// Dibuja las filas [first, last) de la tabla; la fila entries.size() es el total.
float Table(Graphics& g, const ReportData& d, float y, int first, int last) {
  static const float weights[7] = {1.3f, 2.6f, 1.9f, 0.9f, 1.0f, 1.0f, 1.1f};
  static const wchar_t* heads[7] = {L"Comida", L"Alimento", L"Porción", L"kcal", L"Prot. (g)", L"Carb. (g)", L"Grasas (g)"};
  float sum = 0;
  for (float w : weights) sum += w;
  float xs[8];
  xs[0] = M;
  for (int i = 0; i < 7; ++i) xs[i + 1] = xs[i] + CW * weights[i] / sum;

  auto row = [&](float top, const std::wstring cells[7], bool bold, const Color& ink) {
    for (int c = 0; c < 7; ++c) {
      ui::Text(g, cells[c], Rc(xs[c] + 5, top, xs[c + 1] - xs[c] - 10, kRowH), 9.0f, bold, ink,
               c >= 3 ? Align::Right : Align::Left, false, true);
    }
  };

  ui::FillRect(g, Rc(M, y, CW, kHeaderH), col::Green());
  const std::wstring headCells[7] = {heads[0], heads[1], heads[2], heads[3], heads[4], heads[5], heads[6]};
  for (int c = 0; c < 7; ++c) {
    ui::Text(g, headCells[c], Rc(xs[c] + 5, y, xs[c + 1] - xs[c] - 10, kHeaderH), 8.5f, true, col::White(),
             c >= 3 ? Align::Right : Align::Left, false, true);
  }
  y += kHeaderH;

  const Totals t = SumEntries(d.entries);
  const int count = static_cast<int>(d.entries.size());
  for (int i = first; i < last; ++i) {
    std::wstring cells[7];
    const bool total = i == count;
    if (total) {
      cells[0] = L"Total";
      cells[3] = util::FormatNumber(t.calories, 0);
      cells[4] = util::FormatNumber(t.protein, 1);
      cells[5] = util::FormatNumber(t.carbs, 1);
      cells[6] = util::FormatNumber(t.fat, 1);
    } else {
      const FoodEntry& e = d.entries[static_cast<size_t>(i)];
      cells[0] = MealLabel(e.meal);
      cells[1] = e.name;
      cells[2] = e.portion;
      cells[3] = util::FormatNumber(e.calories, 0);
      cells[4] = util::FormatNumber(e.protein, 1);
      cells[5] = util::FormatNumber(e.carbs, 1);
      cells[6] = util::FormatNumber(e.fat, 1);
      if (i % 2 == 1) ui::FillRect(g, Rc(M, y, CW, kRowH), col::Zebra());
    }
    row(y, cells, total, col::Ink());
    ui::HLine(g, M, M + CW, y + kRowH, col::Line(), 0.6f);
    y += kRowH;
  }
  return y;
}

}  // namespace

int ReportPageCount(const ReportData& d) {
  if (d.entries.empty()) return 1;
  const int rows = static_cast<int>(d.entries.size()) + 1;
  const int first = RowsFirstPage();
  if (rows <= first) return 1;
  return 1 + static_cast<int>(std::ceil(static_cast<double>(rows - first) / RowsNextPages()));
}

void DrawReportPage(Graphics& g, const ReportData& d, int page) {
  const int pages = ReportPageCount(d);
  ui::FillRect(g, Rc(0, 0, kPageWidth, kPageHeight), col::White());
  Header(g, d);

  float y;
  if (page == 0) {
    Summary(g, d);
    y = kFirstTableY;
  } else {
    ui::Text(g, L"Registro de alimentos (continuación)", Rc(M, 102, CW, 18), 12.5f, true, col::Ink());
    y = kNextTableY;
  }

  if (d.entries.empty()) {
    ui::Text(g, L"No hay alimentos registrados este día.", Rc(M, y, CW, 16), 10.0f, false, col::Muted());
    y += 20;
  } else {
    const int rows = static_cast<int>(d.entries.size()) + 1;
    const int first = page == 0 ? 0 : RowsFirstPage() + (page - 1) * RowsNextPages();
    const int last = std::min(rows, first + (page == 0 ? RowsFirstPage() : RowsNextPages()));
    y = Table(g, d, y, first, last);
  }

  if (page == pages - 1) {
    y += 18;
    const std::wstring notes = util::Trim(d.notes);
    if (!notes.empty()) {
      ui::Text(g, L"Observaciones", Rc(M, y, CW, 16), 11.0f, true, col::Ink());
      ui::Text(g, notes, Rc(M, y + 18, CW, kPageHeight - M - 60 - (y + 18)), 9.5f, false, col::Ink(), Align::Left, true);
    }
    ui::Text(g, L"Valores nutricionales referenciales. Este informe no reemplaza la evaluación de un profesional de la salud.",
             Rc(M, kPageHeight - M - 38, CW, 14), 8.0f, false, col::Muted());
  }
  Footer(g, page, pages);
}

bool PrintReport(HWND owner, const ReportData& data) {
  PRINTDLGW pd = {};
  pd.lStructSize = sizeof(pd);
  pd.hwndOwner = owner;
  pd.Flags = PD_RETURNDC | PD_NOSELECTION | PD_NOPAGENUMS | PD_USEDEVMODECOPIESANDCOLLATE;
  if (!PrintDlgW(&pd)) return false;  // cancelado

  bool ok = false;
  HDC dc = pd.hDC;
  if (dc) {
    const std::wstring docName = L"NutryScan_informe_" + util::DateShort(data.date);
    DOCINFOW di = {};
    di.cbSize = sizeof(di);
    di.lpszDocName = docName.c_str();

    if (StartDocW(dc, &di) > 0) {
      ok = true;
      const int pages = ReportPageCount(data);
      const float dpiX = static_cast<float>(GetDeviceCaps(dc, LOGPIXELSX));
      const float dpiY = static_cast<float>(GetDeviceCaps(dc, LOGPIXELSY));
      const float offX = static_cast<float>(GetDeviceCaps(dc, PHYSICALOFFSETX));
      const float offY = static_cast<float>(GetDeviceCaps(dc, PHYSICALOFFSETY));

      for (int i = 0; i < pages && ok; ++i) {
        if (StartPage(dc) <= 0) {
          ok = false;
          break;
        }
        {
          Graphics g(dc);
          g.SetPageUnit(UnitPixel);
          g.SetSmoothingMode(SmoothingModeAntiAlias);
          g.SetTextRenderingHint(TextRenderingHintAntiAlias);
          g.TranslateTransform(-offX, -offY);
          g.ScaleTransform(dpiX / 72.0f, dpiY / 72.0f);
          DrawReportPage(g, data, i);
        }
        if (EndPage(dc) <= 0) ok = false;
      }
      if (ok) EndDoc(dc);
      else AbortDoc(dc);
    }
    DeleteDC(dc);
  }
  if (pd.hDevMode) GlobalFree(pd.hDevMode);
  if (pd.hDevNames) GlobalFree(pd.hDevNames);

  if (!ok) MessageBoxW(owner, L"No se pudo imprimir el informe. Revisa la impresora.", L"NutryScan", MB_ICONWARNING);
  return ok;
}
