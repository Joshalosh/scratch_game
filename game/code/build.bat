@echo off

ctime -begin game.ctm

set CommonCompilerFlags= -Od -MTd -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4459 -wd4456 -wd4505 -wd4244 -wd4127 -wd4838 -DGAME_INTERNAL=1 -DGAME_SLOW=1 -DGAME_WIN32=1 -FC -Z7 
set CommonCompilerFlags= -DGAME_INTERNAL=1 -DGAME_SLOW=1 -DGAME_WIN32=1 %CommonCompilerFlags%
set CommonLinkerFlags=  -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib 

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

del *.pdb > NUL 2> NUL

REM Simple preprocessor 
REM cl %CommonCompilerFlags% -D_CRT_SECURE_NO_WARNINGS ..\game\code\simple_preprocessor.cpp /link %CommonLinkerFlags%

REM Asset file builder build
REM cl %CommonCompilerFlags% -DTRANSLATION_UNIT_INDEX=0 -D_CRT_SECURE_NO_WARNINGS ..\game\code\test_asset_builder.cpp /link %CommonLinkerFlags%

REM 32-bit build
REM cl %CommonCompilerFlags% ..\game\code\win32_game.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
REM Optimisation switches /wO2 
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% -DTRANSLATION_UNIT_INDEX=1 -O2 -I..\iaca-win64\ -c ..\game\code\game_optimised.cpp -Fogame_optimised.obj -LD
cl %CommonCompilerFlags% -DTRANSLATION_UNIT_INDEX=0 -I..\iaca-win64\ ..\game\code\game.cpp game_optimised.obj -Fmgame.map -LD /link -incremental:no -PDB:game_%random%.pdb -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender -EXPORT:DEBUGGameFrameEnd
del lock.tmp
cl %CommonCompilerFlags% -DTRANSLATION_UNIT_INDEX=2 ..\game\code\win32_game.cpp -Fmwin32_game.map /link %CommonLinkerFlags%
popd

ctime -end game.ctm
