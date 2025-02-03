@echo off

REM Switch to build directory and then build to get build artifacts in the 'build' directory. Push the current directory into stack
pushd ..\build

REM /Zi enables debug information
REM /FC Full path name in diagnostics
cl /FC /Zi ..\code\win32_handmade.cpp user32.lib gdi32.lib xinput.lib

REM pop the last pushed directory
popd
