@echo off
rem ============================================================
rem  One-click build: find MSBuild and build Release|x64
rem  Output: bin\x64\Release\Calculator.exe
rem ============================================================
setlocal
set "MSB="
where msbuild >nul 2>nul && set "MSB=msbuild"
if not defined MSB (
  for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe 2^>nul`) do set "MSB=%%i"
)
if not defined MSB (
  echo [ERROR] MSBuild not found. Please install Visual Studio with "Desktop development with C++".
  pause
  exit /b 1
)
echo Using MSBuild: %MSB%
"%MSB%" Calculator.sln /p:Configuration=Release /p:Platform=x64 /m /nologo
if errorlevel 1 (
  echo [ERROR] Build failed. Check the errors above.
  pause
  exit /b 1
)
echo.
echo Build succeeded! Executable: bin\x64\Release\Calculator.exe
pause
