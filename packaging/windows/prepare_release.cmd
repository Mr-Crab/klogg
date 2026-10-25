@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem Keep all paths independent of the directory from which this script is run.
for %%I in ("%~dp0..\..") do set "ROOT_DIR=%%~fI"
cd /d "%ROOT_DIR%" || goto :fail

if not defined KLOGG_WORKSPACE set "KLOGG_WORKSPACE=%ROOT_DIR%"
if not defined KLOGG_ARCH set "KLOGG_ARCH=x64"
if not defined KLOGG_QT set "KLOGG_QT=Qt6"

for %%I in ("%KLOGG_WORKSPACE%") do set "KLOGG_WORKSPACE=%%~fI"

if defined KLOGG_BUILD_ROOT goto :build_root_ready
for %%D in (cmake-build-release build-release cmake-build-relwithdebinfo build) do (
  if not defined KLOGG_BUILD_ROOT if exist "%KLOGG_WORKSPACE%\%%D\generated\version.h" if exist "%KLOGG_WORKSPACE%\%%D\output\klogg.exe" set "KLOGG_BUILD_ROOT=%%D"
)
if not defined KLOGG_BUILD_ROOT (
  echo [ERROR] No packaged build directory was found.
  echo [ERROR] Build klogg and klogg_portable, or set KLOGG_BUILD_ROOT explicitly.
  goto :fail
)

:build_root_ready
set "BUILD_DIR=%KLOGG_WORKSPACE%\%KLOGG_BUILD_ROOT%"
if "%KLOGG_BUILD_ROOT:~1,1%"==":" set "BUILD_DIR=%KLOGG_BUILD_ROOT%"
if "%KLOGG_BUILD_ROOT:~0,1%"=="\" set "BUILD_DIR=%KLOGG_BUILD_ROOT%"
set "OUTPUT_DIR=%BUILD_DIR%\output"
set "RELEASE_DIR=%KLOGG_WORKSPACE%\release"

if not defined KLOGG_VERSION (
  if exist "%BUILD_DIR%\generated\version.h" (
    for /f "tokens=3 delims= " %%V in ('findstr /C:"#define KLOGG_VERSION" "%BUILD_DIR%\generated\version.h" 2^>nul') do set "KLOGG_VERSION=%%~V"
  )
)

if defined KLOGG_QT_DIR goto :qt_dir_ready
set "QT_CMAKE_DIR="
if exist "%BUILD_DIR%\CMakeCache.txt" (
  for /f "tokens=2 delims==" %%V in ('findstr /B /C:"%KLOGG_QT%_DIR:PATH=" "%BUILD_DIR%\CMakeCache.txt" 2^>nul') do set "QT_CMAKE_DIR=%%V"
)
if defined QT_CMAKE_DIR (
  for %%I in ("%QT_CMAKE_DIR%\..\..\..") do set "KLOGG_QT_DIR=%%~fI"
)

:qt_dir_ready
if not defined KLOGG_VERSION goto :missing_version
if not defined KLOGG_QT_DIR goto :missing_qt

echo Building klogg %KLOGG_VERSION% release files
echo   workspace: %KLOGG_WORKSPACE%
echo   build dir: %BUILD_DIR%
echo   output dir: %OUTPUT_DIR%
echo   Qt version: %KLOGG_QT%
echo   Qt dir: %KLOGG_QT_DIR%

if not exist "%OUTPUT_DIR%" (
  echo [ERROR] Build output directory does not exist: "%OUTPUT_DIR%"
  echo [ERROR] Configure and build target ci_build before packaging.
  goto :fail
)

set "QTDIR=%KLOGG_QT_DIR:/=\%"
if not exist "%QTDIR%\bin" (
  echo [ERROR] Invalid KLOGG_QT_DIR, missing bin directory: "%QTDIR%\bin"
  goto :fail
)

if /I "%KLOGG_QT%"=="Qt6" (
  set "QT_COMPAT_DLL=%QTDIR%\bin\Qt6Core5Compat.dll"
  set "QT_STYLE_DLL=%QTDIR%\plugins\styles\qmodernwindowsstyle.dll"
) else (
  set "QT_COMPAT_DLL="
  set "QT_STYLE_DLL=%QTDIR%\plugins\styles\qwindowsvistastyle.dll"
)
set "QT_PLATFORM_DLL=%QTDIR%\plugins\platforms\qwindows.dll"

if /I "%KLOGG_ARCH%"=="x64" (
  set "VCRT_ARCH=x64"
  set "DEFAULT_SSL_ARCH=-x64"
) else if /I "%KLOGG_ARCH%"=="arm64" (
  set "VCRT_ARCH=arm64"
  set "DEFAULT_SSL_ARCH=-arm64"
) else (
  set "VCRT_ARCH=x86"
  set "DEFAULT_SSL_ARCH="
)

if defined VCToolsRedistDir goto :vc_runtime_ready

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
  for /f "delims=" %%V in ('"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath') do set "VSINSTALLDIR=%%V"
)
if defined VSINSTALLDIR (
  for /f "delims=" %%V in ('dir /b /ad /o-n "%VSINSTALLDIR%\VC\Redist\MSVC" 2^>nul ^| findstr /R "^14\."') do if not defined VCRT_VERSION set "VCRT_VERSION=%%V"
)
if defined VCRT_VERSION (
  set "VCToolsRedistDir=%VSINSTALLDIR%\VC\Redist\MSVC\%VCRT_VERSION%\"
)
if not defined VCToolsRedistDir (
  echo [ERROR] Visual C++ runtime directory could not be found.
  echo [ERROR] Set VCToolsRedistDir or run from a Native Tools Command Prompt for VS.
  goto :fail
)

:vc_runtime_ready
if not "%VCToolsRedistDir:~-1%"=="\" set "VCToolsRedistDir=%VCToolsRedistDir%\"
set "VCRT_DIR="
for /f "delims=" %%D in ('dir /b /ad /o-n "%VCToolsRedistDir%%VCRT_ARCH%\Microsoft.VC*.CRT" 2^>nul') do (
  if not defined VCRT_DIR set "VCRT_DIR=%VCToolsRedistDir%%VCRT_ARCH%\%%D"
)
if not defined VCRT_DIR (
  echo [ERROR] Visual C++ runtime directory could not be found under:
  echo         "%VCToolsRedistDir%%VCRT_ARCH%"
  goto :fail
)

if exist "%RELEASE_DIR%" rmdir /s /q "%RELEASE_DIR%" || goto :fail
mkdir "%RELEASE_DIR%" || goto :fail
mkdir "%RELEASE_DIR%\platforms" || goto :fail
mkdir "%RELEASE_DIR%\styles" || goto :fail
if /I "%KLOGG_QT%"=="Qt6" mkdir "%RELEASE_DIR%\tls" || goto :fail

echo Copying klogg binaries...
call :copy_file "%OUTPUT_DIR%\klogg.exe" "%RELEASE_DIR%" || goto :fail
call :copy_optional "%OUTPUT_DIR%\klogg.pdb" "%RELEASE_DIR%" || goto :fail
call :copy_file "%OUTPUT_DIR%\klogg_portable.exe" "%RELEASE_DIR%" || goto :fail
call :copy_optional "%OUTPUT_DIR%\klogg_portable.pdb" "%RELEASE_DIR%" || goto :fail
call :copy_optional "%OUTPUT_DIR%\klogg_crashpad_handler.exe" "%RELEASE_DIR%" || goto :fail
call :copy_optional "%OUTPUT_DIR%\klogg_minidump_dump.exe" "%RELEASE_DIR%" || goto :fail

rem TBB is written to output by the klogg post-build step. Do not depend on
rem oneTBB's compiler-version-specific output directory.
call :copy_file "%OUTPUT_DIR%\tbb12.dll" "%RELEASE_DIR%" || goto :fail
call :copy_optional "%OUTPUT_DIR%\tbb12.pdb" "%RELEASE_DIR%" || goto :fail

echo Copying documentation and license files...
call :copy_file "%BUILD_DIR%\generated\documentation.html" "%RELEASE_DIR%" || goto :fail
call :copy_file "%KLOGG_WORKSPACE%\COPYING" "%RELEASE_DIR%" || goto :fail
call :copy_file "%KLOGG_WORKSPACE%\NOTICE" "%RELEASE_DIR%" || goto :fail
call :copy_file "%KLOGG_WORKSPACE%\README.md" "%RELEASE_DIR%" || goto :fail
call :copy_file "%KLOGG_WORKSPACE%\DOCUMENTATION.md" "%RELEASE_DIR%" || goto :fail

echo Copying Visual C++ runtime...
call :copy_file "%VCRT_DIR%\msvcp140.dll" "%RELEASE_DIR%" || goto :fail
call :copy_file "%VCRT_DIR%\msvcp140_1.dll" "%RELEASE_DIR%" || goto :fail
call :copy_file "%VCRT_DIR%\msvcp140_2.dll" "%RELEASE_DIR%" || goto :fail
call :copy_file "%VCRT_DIR%\vcruntime140.dll" "%RELEASE_DIR%" || goto :fail
call :copy_file "%VCRT_DIR%\vcruntime140_1.dll" "%RELEASE_DIR%" || goto :fail

if defined SSL_DIR (
  if not defined SSL_ARCH set "SSL_ARCH=%DEFAULT_SSL_ARCH%"
  echo Copying OpenSSL runtime...
  call :copy_file "%SSL_DIR%\libcrypto-1_1%SSL_ARCH%.dll" "%RELEASE_DIR%" || goto :fail
  call :copy_file "%SSL_DIR%\libssl-1_1%SSL_ARCH%.dll" "%RELEASE_DIR%" || goto :fail
) else (
  echo [WARN] SSL_DIR is not set. OpenSSL will not be included in the package.
)

echo Copying Qt runtime...
call :copy_file "%QTDIR%\bin\%KLOGG_QT%Core.dll" "%RELEASE_DIR%" || goto :fail
call :copy_file "%QTDIR%\bin\%KLOGG_QT%Gui.dll" "%RELEASE_DIR%" || goto :fail
call :copy_file "%QTDIR%\bin\%KLOGG_QT%Network.dll" "%RELEASE_DIR%" || goto :fail
call :copy_file "%QTDIR%\bin\%KLOGG_QT%Widgets.dll" "%RELEASE_DIR%" || goto :fail
call :copy_file "%QTDIR%\bin\%KLOGG_QT%Concurrent.dll" "%RELEASE_DIR%" || goto :fail
call :copy_file "%QTDIR%\bin\%KLOGG_QT%Xml.dll" "%RELEASE_DIR%" || goto :fail
if defined QT_COMPAT_DLL (
  call :copy_file "%QT_COMPAT_DLL%" "%RELEASE_DIR%" || goto :fail
)

call :copy_file "%QT_PLATFORM_DLL%" "%RELEASE_DIR%\platforms" || goto :fail
call :copy_file "%QT_STYLE_DLL%" "%RELEASE_DIR%\styles" || goto :fail

if /I not "%KLOGG_QT%"=="Qt6" goto :qt_tls_done

echo Copying Qt TLS backends...
set "TLS_BACKEND_COUNT=0"
for %%F in ("%QTDIR%\plugins\tls\*backend.dll") do (
  call :copy_file "%%~fF" "%RELEASE_DIR%\tls" || goto :fail
  set /a TLS_BACKEND_COUNT+=1
)
if "%TLS_BACKEND_COUNT%"=="0" (
  echo [ERROR] No Qt TLS backends found in "%QTDIR%\plugins\tls".
  goto :fail
)

:qt_tls_done
echo Copying NSIS scripts...
call :copy_file "%ROOT_DIR%\packaging\windows\klogg.nsi" "%ROOT_DIR%" || goto :fail
call :copy_file "%ROOT_DIR%\packaging\windows\FileAssociation.nsh" "%ROOT_DIR%" || goto :fail

if not defined SEVEN_ZIP set "SEVEN_ZIP=7z.exe"
if /I "%SEVEN_ZIP%"=="7z.exe" (
  where 7z.exe >nul 2>nul
  if errorlevel 1 (
    echo [ERROR] 7-Zip was not found in PATH.
    echo [ERROR] Install 7-Zip or set SEVEN_ZIP to its executable.
    goto :fail
  )
) else (
  if not exist "%SEVEN_ZIP%" (
    echo [ERROR] 7-Zip executable does not exist: "%SEVEN_ZIP%"
    goto :fail
  )
)

echo Creating portable and PDB archives...
if exist "%KLOGG_WORKSPACE%\klogg-%KLOGG_VERSION%-%KLOGG_ARCH%-%KLOGG_QT%-portable.zip" del /f /q "%KLOGG_WORKSPACE%\klogg-%KLOGG_VERSION%-%KLOGG_ARCH%-%KLOGG_QT%-portable.zip" || goto :fail
if exist "%KLOGG_WORKSPACE%\klogg-%KLOGG_VERSION%-%KLOGG_ARCH%-%KLOGG_QT%-pdb.zip" del /f /q "%KLOGG_WORKSPACE%\klogg-%KLOGG_VERSION%-%KLOGG_ARCH%-%KLOGG_QT%-pdb.zip" || goto :fail
"%SEVEN_ZIP%" a -r "%KLOGG_WORKSPACE%\klogg-%KLOGG_VERSION%-%KLOGG_ARCH%-%KLOGG_QT%-portable.zip" "@%ROOT_DIR%\packaging\windows\7z_klogg_listfile.txt" || goto :fail

if not exist "%RELEASE_DIR%\klogg.pdb" goto :skip_pdb_archive
if not exist "%RELEASE_DIR%\klogg_portable.pdb" goto :skip_pdb_archive
if not exist "%RELEASE_DIR%\tbb12.pdb" goto :skip_pdb_archive
"%SEVEN_ZIP%" a -r "%KLOGG_WORKSPACE%\klogg-%KLOGG_VERSION%-%KLOGG_ARCH%-%KLOGG_QT%-pdb.zip" "@%ROOT_DIR%\packaging\windows\7z_pdb_listfile.txt" || goto :fail
goto :pdb_archive_done

:skip_pdb_archive
echo [INFO] Debug symbols are not available, skipping the PDB archive.

:pdb_archive_done

echo Release files are ready in "%RELEASE_DIR%".
exit /b 0

:copy_file
if not exist "%~1" (
  echo [ERROR] Missing required file: "%~1"
  exit /b 1
)
copy /Y "%~1" "%~2" >nul
if errorlevel 1 (
  echo [ERROR] Failed to copy "%~1" to "%~2"
  exit /b 1
)
exit /b 0

:copy_optional
if not exist "%~1" (
  echo [INFO] Optional file not found: "%~1"
  exit /b 0
)
copy /Y "%~1" "%~2" >nul
if errorlevel 1 (
  echo [ERROR] Failed to copy "%~1" to "%~2"
  exit /b 1
)
exit /b 0

:missing_version
echo [ERROR] KLOGG_VERSION is not set and could not be read from:
echo         "%BUILD_DIR%\generated\version.h"
goto :fail

:missing_qt
echo [ERROR] KLOGG_QT_DIR is not set.
echo [ERROR] Example:
echo         set "KLOGG_QT_DIR=E:\software\QT\6.7.3\msvc2022_64"
goto :fail

:fail
echo.
echo [ERROR] Windows release preparation failed.
exit /b 1
