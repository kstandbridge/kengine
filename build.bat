@echo off

SETLOCAL EnableDelayedExpansion

set CommonCompilerFlags=-diagnostics:column -WL -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -FC -Z7 -GS- -Gs9999999
set CommonCompilerFlags=-DKENGINE_INTERNAL=1 %CommonCompilerFlags%

REM unreferenced formal parameter
REM set CommonCompilerFlags=-wd4100 %CommonCompilerFlags%
REM local variable is initialized but not referenced
set CommonCompilerFlags=-wd4189 %CommonCompilerFlags%
REM nonstandard extension used: nameless struct/union
set CommonCompilerFlags=-wd4201 %CommonCompilerFlags%
REM 'type cast': pointer truncation
set CommonCompilerFlags=-wd4311 %CommonCompilerFlags%
REM 'type cast': conversion
set CommonCompilerFlags=-wd4312 %CommonCompilerFlags%

set CommonLinkerFlags=-STACK:0x100000,0x100000 -incremental:no -opt:ref kernel32.lib

IF NOT EXIST data mkdir data
IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

if "%~1"=="build_dependencies" (
	
	del win32_*.pdb > NUL 2> NUL

	REM Preprocessor
	cl %CommonCompilerFlags% -MTd -Od ..\kengine\code\win32_kengine_preprocessor.c /link /NODEFAULTLIB /SUBSYSTEM:console %CommonLinkerFlags%
	if !ERRORLEVEL! neq 0 ( goto cleanup )
	pushd ..\kengine\code
	..\..\build\win32_kengine_preprocessor.exe > kengine_generated.h
	popd

	REM Unit tests
	cl %CommonCompilerFlags% -MTd -Od ..\kengine\code\win32_kengine_tests.c /link /NODEFAULTLIB /SUBSYSTEM:console %CommonLinkerFlags%
	if !ERRORLEVEL! neq 0 ( goto cleanup )
	win32_kengine_tests.exe

	REM Win32 platform
	cl %CommonCompilerFlags% -MTd -Od ..\kengine\code\win32_kengine.c /link /NODEFAULTLIB /SUBSYSTEM:windows %CommonLinkerFlags% Gdi32.lib User32.lib opengl32.lib

) else (

	del kengine_*.pdb > NUL 2> NUL

	REM App
	echo WAITING FOR PDB > lock.tmp
	cl %CommonCompilerFlags% -MTd -Od ..\kengine\code\kengine.c -LD /link %CommonLinkerFlags%  -PDB:kengine_%random%.pdb -EXPORT:AppUpdateFrame User32.lib
	del lock.tmp

	del /q *.exp
	del /q *.lib
)

:cleanup

del /q *.obj

popd