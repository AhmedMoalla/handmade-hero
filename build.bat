@echo off

mkdir build
pushd build
cl /Zi /Od /MDd /Fehandmade.exe ..\src\win32_handmade.cpp /link user32.lib gdi32.lib
popd
