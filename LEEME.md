# NutryScan para Windows en C++

Programa de escritorio nativo en C++ con la API de Windows (Win32) y GDI+. No usa librerías externas: se compila en un solo NutryScan.exe que funciona sin instalar nada. Guarda los datos en el PC y funciona sin internet.

## Qué hace

- Inicio: calorías del día con anillo de avance, proteínas, carbohidratos y grasas, gráfico de los últimos 7 días y lo último registrado.
- Registro diario: alimentos por comida (desayuno, almuerzo, once, cena y colación), con navegación por fecha. Para eliminar, selecciona uno y usa el botón o la tecla Supr.
- Agregar alimento: catálogo de comidas chilenas que rellena los valores solo, o ingreso manual. Multiplica por la cantidad de porciones.
- Informes: vista previa de la hoja en pantalla, observaciones y botón "Imprimir o guardar PDF". Para PDF, elige la impresora "Microsoft Print to PDF", que viene en Windows 10 y 11. Si hay muchos alimentos, el informe usa varias páginas.
- Perfil y metas: nombre, correo y metas diarias que aparecen en el informe.

Los datos se guardan en %APPDATA%\NutryScan\datos.txt.

## Opción A: compilar en GitHub (sin instalar nada)

1. Crea un repositorio nuevo en GitHub y sube todo el contenido de esta carpeta, incluida la carpeta oculta .github.
2. Entra a la pestaña Actions y abre "Compilar NutryScan C++ (Windows)". Si no partió solo, presiona "Run workflow".
3. Al terminar (2 a 3 minutos), descarga NutryScan-Windows-Cpp desde Artifacts. Adentro está NutryScan.exe, listo para abrir.

## Opción B: compilar en tu PC

1. Instala Visual Studio Community (gratis) y marca "Desarrollo para el escritorio con C++". Ese paquete incluye el compilador, CMake y Ninja.
2. Doble clic en COMPILAR.bat. Al terminar se abre el programa. El .exe queda en build\NutryScan.exe.

En VS Code: instala las extensiones "C/C++" y "CMake Tools", abre la carpeta y elige el kit "Visual Studio ... amd64".

## Estructura

- src/main.cpp: ventana, menú lateral, pantallas y eventos.
- src/state.*: datos (perfil y diario) y guardado.
- src/ui_draw.*: colores, paneles, logo, anillo y barras (GDI+).
- src/report.*: hoja del informe e impresión con paginación.
- src/add_food_dialog.*: diálogo "Agregar alimento".
- src/util.*: formatos de números y fechas en español.
- src/catalog.h: catálogo de alimentos.
- res/: ícono, diálogo, versión y manifiesto (estilo moderno y nitidez en pantallas de alta resolución).
- CMakeLists.txt: configuración de compilación.

## Personalizar

- Contacto del pie del informe: src/report.cpp (kContact).
- Colores: src/ui_draw.h.
- Alimentos del catálogo: src/catalog.h.
- Ícono del programa: reemplaza res/app.ico.
