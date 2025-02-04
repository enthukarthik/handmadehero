@echo off

REM Switch to build directory and then build to get build artifacts in the 'build' directory. Push the current directory into stack
pushd ..\build

REM /Zi enables debug information
REM /FC Full path name in diagnostics
REM /Fe File name for the executable
cl /std:c17 /FC /Zi /FeHandmadeHero ..\code\win32_handmade.c user32.lib gdi32.lib

REM pop the last pushed directory
popd
