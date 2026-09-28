#pragma once
#include <windows.h>

#include "state.h"

// Muestra el diálogo "Agregar alimento". Devuelve true si el usuario lo confirmó.
bool ShowAddFoodDialog(HWND owner, int initialMeal, FoodEntry* out);
