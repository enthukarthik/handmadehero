@echo off

REM Switch to build directory and then build to get build artifacts in the 'build' directory. Push the current directory into stack
pushd ..\build

REM /Zi enables debug information
REM /FC Full path name in diagnostics
REM /Fe File name for the executable
REM /W4 Enable warning level 4
REM /WX Treat warnings as errors
cl /std:c17 /W4 /WX /wd4191 /wd5045 /FC /Zi /FeHandmadeHero ..\code\win32_handmade.c user32.lib gdi32.lib
REM C4191 : unsafe conversion from 'FARPROC' to 'XInputGetStatePtr (__cdecl *)'
REM C5045 : Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

REM pop the last pushed directory
popd
