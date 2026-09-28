#pragma once
#include <string>
#include <vector>

#include "gfx.h"
#include "state.h"

struct ReportData {
  Profile profile;
  SYSTEMTIME date{};
  std::vector<FoodEntry> entries;
  std::wstring notes;
};

// La página se dibuja en puntos (carta: 612 x 792). Quien llama aplica la escala.
constexpr float kPageWidth = 612.0f;
constexpr float kPageHeight = 792.0f;

int ReportPageCount(const ReportData& data);
void DrawReportPage(Gdiplus::Graphics& g, const ReportData& data, int pageIndex);

// Abre el diálogo de impresión. Para PDF: elegir "Microsoft Print to PDF".
bool PrintReport(HWND owner, const ReportData& data);
