@echo off

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
cl -MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DGAME_INTERNAL=1 -DGAME_SLOW=1 -DGAME_WIN32=1 -FC -Z7 -Fmwin32_game.map ..\game\code\win32_game.cpp /link -opt:ref -subsystem:windows,5.1 user32.lib gdi32.lib
popd
