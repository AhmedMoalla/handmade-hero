@echo off
setlocal

call "%ProgramFiles%\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvarsall.bat" x64

mkdir build 2>nul
pushd build
cl /Zi /Od /MDd /Fehandmade.exe ..\src\win32_handmade.cpp /link user32.lib gdi32.lib
set "BUILD_ERROR=%ERRORLEVEL%"
popd

endlocal & exit /b %BUILD_ERROR%
