@echo off
title NutryScan - Compilar
cd /d "%~dp0"
echo ============================================
echo   NutryScan C++ - compilar NutryScan.exe
echo ============================================

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto novs

for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH goto novs

call "%VSPATH%\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 >nul
if errorlevel 1 goto error

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto error
cmake --build build
if errorlevel 1 goto error

echo.
echo Listo: build\NutryScan.exe
start "" "build\NutryScan.exe"
goto fin

:novs
echo.
echo No se encontro Visual Studio con "Desarrollo para el escritorio con C++".
echo Instalalo gratis desde https://visualstudio.microsoft.com/es/vs/community/
goto fin

:error
echo.
echo Hubo un error al compilar. Revisa los mensajes de arriba.

:fin
pause
