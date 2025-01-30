@echo off

REM Call VS batch file to enable cl.exe available in the path
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64

REM Add handmade hero path where we can execute the build executables and batch scripts
set PATH=W:\code;W:\build;%PATH%
