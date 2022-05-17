@echo off

SETLOCAL EnableDelayedExpansion

ctime -begin kengine.ctm

set CommonCompilerFlags=-diagnostics:column -WL -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -FC -Z7 -GS- -Gs9999999
set CommonCompilerFlags=-DKENGINE_INTERNAL=1 %CommonCompilerFlags%

REM unreferenced formal parameter
set CommonCompilerFlags=-wd4100 %CommonCompilerFlags%
REM local variable is initialized but not referenced
set CommonCompilerFlags=-wd4189 %CommonCompilerFlags%
REM nonstandard extension used: nameless struct/union
set CommonCompilerFlags=-wd4201 %CommonCompilerFlags%

set CommonLinkerFlags=-STACK:0x100000,0x100000 -incremental:no -opt:ref kernel32.lib

IF NOT EXIST data mkdir data
IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

del *.pdb > NUL 2> NUL

REM Preprocessor
cl %CommonCompilerFlags% -MTd ..\kengine\code\win32_kengine_preprocessor.c /link /NODEFAULTLIB /SUBSYSTEM:console %CommonLinkerFlags%
if !ERRORLEVEL! neq 0 ( goto cleanup )
pushd ..\kengine\code
..\..\build\win32_kengine_preprocessor.exe > kengine_generated.h
popd

REM Unit tests
cl %CommonCompilerFlags% -MTd ..\kengine\code\win32_kengine_tests.c /link /NODEFAULTLIB /SUBSYSTEM:console %CommonLinkerFlags%
if !ERRORLEVEL! neq 0 ( goto cleanup )
win32_kengine_tests.exe

REM Win32 platform
cl %CommonCompilerFlags% -MTd ..\kengine\code\win32_kengine.c /link /NODEFAULTLIB /SUBSYSTEM:windows %CommonLinkerFlags% Gdi32.lib User32.lib Winmm.lib

REM App
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% ..\kengine\code\kengine.c -LD /link %CommonLinkerFlags% Imm32.lib -PDB:kengine_%random%.pdb -EXPORT:AppUpdateAndRender User32.lib
del lock.tmp

:cleanup

popd

set LastError=%ERRORLEVEL%
ctime -end kengine.ctm %LastError%