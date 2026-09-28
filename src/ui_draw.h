#pragma once
#include <string>

#include "gfx.h"

// Dibujo con GDI+: colores de la marca y componentes reutilizables.
namespace ui {

namespace col {
inline Gdiplus::Color Rgb(BYTE r, BYTE g, BYTE b, BYTE a = 255) { return Gdiplus::Color(a, r, g, b); }
inline Gdiplus::Color Ink() { return Rgb(0x1C, 0x2B, 0x24); }
inline Gdiplus::Color Green() { return Rgb(0x2F, 0x6B, 0x4F); }
inline Gdiplus::Color GreenPressed() { return Rgb(0x24, 0x56, 0x3F); }
inline Gdiplus::Color GreenDark() { return Rgb(0x1F, 0x4D, 0x38); }
inline Gdiplus::Color Sage() { return Rgb(0xEE, 0xF4, 0xEF); }
inline Gdiplus::Color Amber() { return Rgb(0xC9, 0x82, 0x2B); }
inline Gdiplus::Color Muted() { return Rgb(0x6B, 0x7A, 0x72); }
inline Gdiplus::Color Line() { return Rgb(0xD5, 0xDE, 0xD8); }
inline Gdiplus::Color Bg() { return Rgb(0xF4, 0xF7, 0xF5); }
inline Gdiplus::Color White() { return Rgb(0xFF, 0xFF, 0xFF); }
inline Gdiplus::Color Zebra() { return Rgb(0xF7, 0xFA, 0xF8); }
inline Gdiplus::Color Protein() { return Rgb(0x3B, 0x6E, 0xA8); }
inline Gdiplus::Color Carbs() { return Rgb(0xC9, 0x82, 0x2B); }
inline Gdiplus::Color Fat() { return Rgb(0x9A, 0x4F, 0x6B); }
inline Gdiplus::Color SideText() { return Rgb(0xDC, 0xE8, 0xE0); }
inline Gdiplus::Color SideMuted() { return Rgb(0xB9, 0xD3, 0xC4); }
}  // namespace col

enum class Align { Left, Center, Right };

Gdiplus::RectF Rc(double x, double y, double w, double h);
void FillRound(Gdiplus::Graphics& g, const Gdiplus::RectF& r, float radius, const Gdiplus::Color& c);
void StrokeRound(Gdiplus::Graphics& g, const Gdiplus::RectF& r, float radius, const Gdiplus::Color& c, float width = 1.0f);
void FillRect(Gdiplus::Graphics& g, const Gdiplus::RectF& r, const Gdiplus::Color& c);
void HLine(Gdiplus::Graphics& g, float x1, float x2, float y, const Gdiplus::Color& c, float width = 1.0f);

// Texto de una línea (con "…" si no cabe) o con saltos si wrap = true.
void Text(Gdiplus::Graphics& g, const std::wstring& s, const Gdiplus::RectF& r, float size, bool bold,
          const Gdiplus::Color& c, Align align = Align::Left, bool wrap = false, bool vcenter = false);

void Panel(Gdiplus::Graphics& g, const Gdiplus::RectF& r, const std::wstring& title);
void Logo(Gdiplus::Graphics& g, float x, float y, float size);
void CalorieRing(Gdiplus::Graphics& g, float cx, float cy, float radius, double consumed, double goal);
void MacroBar(Gdiplus::Graphics& g, const Gdiplus::RectF& r, const std::wstring& label, double value, double goal,
              const Gdiplus::Color& color);

}  // namespace ui
