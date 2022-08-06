@echo off

set CommonCompilerFlags=-diagnostics:column -WL -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -FC -Z7 -GS- -Gs9999999
set CommonCompilerFlags=%CommonCompilerFlags% -DKENGINE_INTERNAL=1
set CommonCompilerFlags=%CommonCompilerFlags% -DKENGINE_SLOW=1

REM warning C4310: cast truncates constant value
REM set CommonCompilerFlags=%CommonCompilerFlags% -wd4310

set CommonLinkerFlags=-STACK:0x100000,0x100000 -incremental:no -opt:ref

IF NOT EXIST data mkdir data
IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

if "%~1"=="build_dependencies" (

	del win32_*.pdb > NUL 2> NUL

	REM Preprocessor
	cl %CommonCompilerFlags% -MTd -Od ..\kengine\code\win32_kengine_preprocessor.c /link /NODEFAULTLIB /SUBSYSTEM:console %CommonLinkerFlags%
	pushd ..\kengine\code
	..\..\build\win32_kengine_preprocessor.exe win32_kengine_types.h > win32_kengine_generated.c
	..\..\build\win32_kengine_preprocessor.exe kengine_types.h > kengine_generated.c
	popd

	REM Unit tests
	cl %CommonCompilerFlags% -MTd -Od ..\kengine\code\win32_kengine_tests.c /link /NODEFAULTLIB /SUBSYSTEM:console %CommonLinkerFlags%
	win32_kengine_tests.exe

	REM Win32 platform
	cl %CommonCompilerFlags% -MTd -Od ..\kengine\code\win32_kengine.c /link /NODEFAULTLIB /SUBSYSTEM:windows %CommonLinkerFlags%

) else (

	del kengine_*.pdb > NUL 2> NUL

	REM Application
	echo WAITING FOR PDB > lock.tmp
	cl %CommonCompilerFlags% -MTd -Od ..\kengine\code\kengine.c -LD /link /NODEFAULTLIB %CommonLinkerFlags% -PDB:kengine_%random%.pdb -EXPORT:AppUpdateFrame
	del lock.tmp
	
	del /q *.exp
	del /q *.lib
)

del /q *.obj

popd