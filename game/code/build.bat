@echo off

mkdir ..\..\build
pushd ..\..\build
cl -Zi ..\game\code\win32_game.cpp user32.lib gdi32.lib
popd
