@echo off

REM Call VS batch file to enable cl.exe available in the path
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

REM Add handmade hero path where we can execute the build executables and batch scripts
set PATH=W:\code;W:\build;%PATH%
