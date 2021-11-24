@echo off

ctime -begin game.ctm

set CommonCompilerFlags= -MTd -nologo -fp:fast -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4459 -wd4456 -wd4505 -wd4244 -DGAME_INTERNAL=1 -DGAME_SLOW=1 -DGAME_WIN32=1 -FC -fsanitize=address -Z7 
set CommonLinkerFlags=  -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib 

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

REM 32-bit build
REM cl %CommonCompilerFlags% ..\game\code\win32_game.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
del *.pdb > NUL 2> NUL
REM Optimisation switches /O2 
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% ..\game\code\game.cpp -Fmgame.map -LD /link -incremental:no -PDB:game_%random%.pdb -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender
del lock.tmp
cl %CommonCompilerFlags% ..\game\code\win32_game.cpp -Fmwin32_game.map /link %CommonLinkerFlags%
popd

ctime -end game.ctm
