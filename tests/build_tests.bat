@echo off
rem ============================================================
rem  Core engine unit test build script (requires Visual Studio)
rem  Run from project root: tests\build_tests.bat
rem ============================================================
setlocal
set "VSPATH="
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath 2^>nul`) do set "VSPATH=%%i"
if not defined VSPATH (
  echo [ERROR] Visual Studio not found.
  exit /b 1
)
call "%VSPATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 ( echo [ERROR] VsDevCmd failed. & exit /b 1 )
cl /nologo /std:c++17 /utf-8 /EHsc /W3 /O2 /I src /D_CRT_SECURE_NO_WARNINGS ^
  tests\core_test.cpp ^
  src\core\NumberFormat.cpp src\core\ExpressionEngine.cpp src\core\IntegerEngine.cpp ^
  src\core\UnitConverter.cpp src\core\DateCalculator.cpp src\core\HistoryStore.cpp ^
  /Fe:tests\core_test.exe shell32.lib
if errorlevel 1 ( echo [BUILD FAILED] & exit /b 1 )
tests\core_test.exe
