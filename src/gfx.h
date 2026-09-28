#pragma once
// Incluye Windows y GDI+ juntos. GDI+ necesita min/max, que NOMINMAX desactiva.
#include <windows.h>
#include <algorithm>
namespace Gdiplus {
using std::max;
using std::min;
}
#include <gdiplus.h>
