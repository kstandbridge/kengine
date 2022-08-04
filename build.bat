@echo off

set CommonCompilerFlags=-diagnostics:column -WL -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -FC -Z7 -GS- -Gs9999999
set CommonCompilerFlags=%CommonCompilerFlags% -DKENGINE_INTERNAL=1
set CommonCompilerFlags=%CommonCompilerFlags% -DKENGINE_SLOW=1
set CommonLinkerFlags=-STACK:0x100000,0x100000 -incremental:no -opt:ref

IF NOT EXIST data mkdir data
IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

echo WAITING FOR PDB > lock.tmp

cl %CommonCompilerFlags% -MTd -Od ..\kengine\code\win32_kengine_preprocessor.c /link /NODEFAULTLIB /SUBSYSTEM:console %CommonLinkerFlags%

del lock.tmp

popd