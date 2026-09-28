#include "ui_draw.h"

#include "util.h"

using namespace Gdiplus;

namespace ui {

RectF Rc(double x, double y, double w, double h) {
  return RectF(static_cast<REAL>(x), static_cast<REAL>(y), static_cast<REAL>(w), static_cast<REAL>(h));
}

static void RoundPath(GraphicsPath& p, const RectF& r, float radius) {
  const float d = std::min(radius * 2.0f, std::min(r.Width, r.Height));
  if (d <= 0.5f) {
    p.AddRectangle(r);
    return;
  }
  p.AddArc(r.X, r.Y, d, d, 180.0f, 90.0f);
  p.AddArc(r.X + r.Width - d, r.Y, d, d, 270.0f, 90.0f);
  p.AddArc(r.X + r.Width - d, r.Y + r.Height - d, d, d, 0.0f, 90.0f);
  p.AddArc(r.X, r.Y + r.Height - d, d, d, 90.0f, 90.0f);
  p.CloseFigure();
}

void FillRound(Graphics& g, const RectF& r, float radius, const Color& c) {
  GraphicsPath p;
  RoundPath(p, r, radius);
  SolidBrush b(c);
  g.FillPath(&b, &p);
}

void StrokeRound(Graphics& g, const RectF& r, float radius, const Color& c, float width) {
  GraphicsPath p;
  RoundPath(p, r, radius);
  Pen pen(c, width);
  g.DrawPath(&pen, &p);
}

void FillRect(Graphics& g, const RectF& r, const Color& c) {
  SolidBrush b(c);
  g.FillRectangle(&b, r);
}

void HLine(Graphics& g, float x1, float x2, float y, const Color& c, float width) {
  Pen pen(c, width);
  g.DrawLine(&pen, x1, y, x2, y);
}

void Text(Graphics& g, const std::wstring& s, const RectF& r, float size, bool bold, const Color& c, Align align,
          bool wrap, bool vcenter) {
  if (s.empty()) return;
  FontFamily family(L"Segoe UI");
  Font font(&family, size, bold ? FontStyleBold : FontStyleRegular, UnitPixel);
  SolidBrush brush(c);
  StringFormat fmt;
  fmt.SetAlignment(align == Align::Left ? StringAlignmentNear
                   : align == Align::Center ? StringAlignmentCenter
                                            : StringAlignmentFar);
  fmt.SetLineAlignment(vcenter ? StringAlignmentCenter : StringAlignmentNear);
  if (!wrap) {
    fmt.SetFormatFlags(StringFormatFlagsNoWrap);
    fmt.SetTrimming(StringTrimmingEllipsisCharacter);
  } else {
    fmt.SetTrimming(StringTrimmingWord);
  }
  g.DrawString(s.c_str(), static_cast<INT>(s.size()), &font, r, &fmt, &brush);
}

void Panel(Graphics& g, const RectF& r, const std::wstring& title) {
  FillRound(g, r, 14.0f, col::White());
  StrokeRound(g, r, 14.0f, col::Line());
  if (!title.empty()) Text(g, title, Rc(r.X + 22, r.Y + 18, r.Width - 44, 24), 16.5f, true, col::Ink());
}

void Logo(Graphics& g, float x, float y, float s) {
  FillRound(g, RectF(x, y, s, s), s * 0.22f, col::Green());

  // Esquinas de "escáner"
  Pen corner(col::White(), s * 0.06f);
  corner.SetStartCap(LineCapRound);
  corner.SetEndCap(LineCapRound);
  const float a = s * 0.18f, b = s * 0.32f, e = s * 0.82f, f = s * 0.68f;
  g.DrawLine(&corner, x + a, y + b, x + a, y + a);
  g.DrawLine(&corner, x + a, y + a, x + b, y + a);
  g.DrawLine(&corner, x + f, y + a, x + e, y + a);
  g.DrawLine(&corner, x + e, y + a, x + e, y + b);
  g.DrawLine(&corner, x + a, y + f, x + a, y + e);
  g.DrawLine(&corner, x + a, y + e, x + b, y + e);
  g.DrawLine(&corner, x + f, y + e, x + e, y + e);
  g.DrawLine(&corner, x + e, y + f, x + e, y + e);

  // Hoja
  auto P = [&](float px, float py) { return PointF(x + px * s, y + py * s); };
  GraphicsPath leaf;
  leaf.AddBezier(P(0.33f, 0.67f), P(0.26f, 0.42f), P(0.42f, 0.28f), P(0.68f, 0.32f));
  leaf.AddBezier(P(0.68f, 0.32f), P(0.72f, 0.58f), P(0.58f, 0.74f), P(0.33f, 0.67f));
  leaf.CloseFigure();
  SolidBrush white(col::White());
  g.FillPath(&white, &leaf);
  Pen vein(col::Green(), s * 0.035f);
  g.DrawLine(&vein, P(0.37f, 0.63f), P(0.62f, 0.38f));
}

void CalorieRing(Graphics& g, float cx, float cy, float radius, double consumed, double goal) {
  const float stroke = radius * 0.22f;
  const RectF box(cx - radius, cy - radius, radius * 2, radius * 2);
  Pen track(col::Sage(), stroke);
  g.DrawEllipse(&track, box);

  const double p = goal > 0 ? consumed / goal : 0.0;
  if (p > 0) {
    Pen value(p > 1.0 ? col::Amber() : col::Green(), stroke);
    value.SetStartCap(LineCapRound);
    value.SetEndCap(LineCapRound);
    const float sweep = static_cast<float>(std::min(p, 1.0) * 359.9);
    g.DrawArc(&value, box, -90.0f, sweep);
  }
  Text(g, util::FormatNumber(consumed, 0), RectF(cx - radius, cy - 24, radius * 2, 36), 30.0f, true, col::Ink(),
       Align::Center);
  Text(g, L"kcal", RectF(cx - radius, cy + 12, radius * 2, 20), 13.0f, false, col::Muted(), Align::Center);
}

void MacroBar(Graphics& g, const RectF& r, const std::wstring& label, double value, double goal, const Color& color) {
  FillRound(g, RectF(r.X, r.Y + 5, 10, 10), 3.0f, color);
  Text(g, label, RectF(r.X + 18, r.Y, r.Width * 0.6f, 20), 14.0f, true, col::Ink());
  Text(g, util::FormatNumber(value, 1) + L" / " + util::Grams(goal), RectF(r.X + r.Width * 0.4f, r.Y + 1, r.Width * 0.6f, 20),
       13.0f, false, col::Muted(), Align::Right);
  const RectF bar(r.X, r.Y + 28, r.Width, 9);
  FillRound(g, bar, 4.5f, col::Sage());
  const double p = goal > 0 ? std::min(value / goal, 1.0) : 0.0;
  if (p > 0) FillRound(g, RectF(bar.X, bar.Y, std::max(9.0f, static_cast<float>(bar.Width * p)), bar.Height), 4.5f, color);
}

}  // namespace ui
