#pragma once
#include <cstddef>

// Alimentos frecuentes. Valores por porción, referenciales.
struct CatalogFood {
  const wchar_t* name;
  const wchar_t* portion;
  double calories;
  double protein;
  double carbs;
  double fat;
};

inline const CatalogFood kCatalog[] = {
    {L"Pan marraqueta", L"1 unidad (100 g)", 270, 9, 56, 1.5},
    {L"Pan integral", L"2 rebanadas (60 g)", 150, 7, 25, 2},
    {L"Palta", L"1/2 unidad (70 g)", 112, 1.4, 6, 10},
    {L"Huevo", L"1 unidad", 72, 6.3, 0.4, 4.8},
    {L"Leche descremada", L"1 taza (200 ml)", 70, 6.6, 9.8, 0.2},
    {L"Avena", L"1/2 taza (40 g)", 150, 5, 27, 2.7},
    {L"Yogur natural", L"1 unidad (125 g)", 80, 4.5, 6, 4},
    {L"Queso fresco", L"1 lámina (30 g)", 60, 5, 1, 4},
    {L"Plátano", L"1 unidad", 105, 1.3, 27, 0.4},
    {L"Manzana", L"1 unidad", 95, 0.5, 25, 0.3},
    {L"Pechuga de pollo", L"150 g", 248, 46, 0, 5.4},
    {L"Carne molida 5 %", L"150 g", 200, 31, 0, 8},
    {L"Salmón", L"150 g", 312, 30, 0, 20},
    {L"Atún en agua", L"1 lata (120 g)", 130, 29, 0, 1},
    {L"Arroz blanco", L"1 taza cocido", 205, 4.3, 45, 0.4},
    {L"Arroz integral", L"1 taza cocido", 216, 5, 45, 1.8},
    {L"Fideos", L"1 taza cocidos", 220, 8, 43, 1.3},
    {L"Papas cocidas", L"1 unidad (150 g)", 130, 3, 30, 0.2},
    {L"Lentejas", L"1 taza cocidas", 230, 18, 40, 0.8},
    {L"Porotos con riendas", L"1 plato", 380, 18, 60, 7},
    {L"Cazuela de vacuno", L"1 plato", 350, 25, 30, 13},
    {L"Ensalada chilena", L"1 plato", 90, 2, 12, 4},
    {L"Empanada de pino", L"1 unidad", 480, 18, 45, 25},
    {L"Completo italiano", L"1 unidad", 450, 13, 40, 26},
    {L"Té o café sin azúcar", L"1 taza", 2, 0, 0, 0},
};

inline constexpr size_t kCatalogCount = sizeof(kCatalog) / sizeof(kCatalog[0]);
