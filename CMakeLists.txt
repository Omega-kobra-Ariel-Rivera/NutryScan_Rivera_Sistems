cmake_minimum_required(VERSION 3.20)
project(NutryScan VERSION 1.0.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(NutryScan WIN32
  src/main.cpp
  src/state.cpp
  src/util.cpp
  src/ui_draw.cpp
  src/report.cpp
  src/add_food_dialog.cpp
  res/app.rc
  res/app.manifest
)

target_include_directories(NutryScan PRIVATE src res)
target_compile_definitions(NutryScan PRIVATE UNICODE _UNICODE NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_link_libraries(NutryScan PRIVATE comctl32 gdiplus uxtheme comdlg32 shell32 ole32 gdi32 user32)

if(MSVC)
  target_compile_options(NutryScan PRIVATE /utf-8 /W3 /permissive-)
  # CRT estático: el .exe funciona sin instalar Visual C++ Redistributable
  set_property(TARGET NutryScan PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
