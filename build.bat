@echo off
setlocal
cd /d "%~dp0"
rem Usage: build.bat [Release|Debug] [C++Builder6 directory]
if not "%~2"=="" set "BCB6_ROOT=%~2"
if not defined BCB6_ROOT set "BCB6_ROOT=C:\Program Files\Borland\CBuilder6"
if not exist "%BCB6_ROOT%\Bin\bcc32.exe" (
  echo ERROR: Cannot find BCB6 at "%BCB6_ROOT%".
  echo Set BCB6_ROOT or pass it as the second argument.
  exit /b 1
)
for %%I in ("%BCB6_ROOT%") do set "BCB6_SHORT=%%~sI"
set "PATH=%BCB6_ROOT%\Bin;%PATH%"
set "MIXP_CFG=%~1"
if not defined MIXP_CFG set "MIXP_CFG=Release"
if not exist "build\%MIXP_CFG%" mkdir "build\%MIXP_CFG%"
"%BCB6_ROOT%\Bin\make.exe" -fbuild.mak CFG=%MIXP_CFG% BCB="%BCB6_SHORT%"
exit /b %errorlevel%
