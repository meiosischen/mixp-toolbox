@echo off
setlocal
cd /d "%~dp0"
if not defined BCB6_ROOT set "BCB6_ROOT=C:\Program Files\Borland\CBuilder6"
call build.bat Release "%BCB6_ROOT%"
if errorlevel 1 exit /b 1
for %%I in ("%BCB6_ROOT%") do set "BCB6_SHORT=%%~sI"
"%BCB6_ROOT%\Bin\make.exe" -fbuild.mak CFG=Release BCB="%BCB6_SHORT%" tests
if errorlevel 1 exit /b 1
build\Release\registry-tests.exe
if errorlevel 1 exit /b 1
build\Release\gui-tests.exe
if errorlevel 1 exit /b 1
build\Release\gui-tests.exe --production-smoke
exit /b %errorlevel%
