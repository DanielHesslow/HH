@echo off
:: this shit is used to print a bunch of whitespace in the beginig of the Program
:: do not remove empty lines, they're there for a reason.... doesn't work without them.

set NLM=^



set NL=^^^%NLM%%NLM%^%NLM%%NLM%

echo %NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%%NL%Starting build...%NL%

:: this defines the environment variables neccessary to run cl
:: running it multiple times will cause something to overflow and shit fail. 
:: thus the hasrun check

IF "%hasrun%"=="" ( call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64)
set hasrun="yes"

set buildmode=.%1

echo buildmode

pushd ..\..\build

set flags=-Z7 -Femain.exe ..\HH\actual_code\win32.cpp user32.lib gdi32.lib
if %buildmode%==.r (
	ctime.exe -begin timings_release.ctm
	cl -O2 -DRELEASE %flags%
) else (
	ctime.exe -begin timings_debug.ctm
	cl -Od -DDEBUG %flags%
)
set err=%errorLevel%

if %buildmode%==.r (
	ctime.exe -end timings_release.ctm %err%
) else (
	ctime.exe -end timings_debug.ctm %err%
)

if %err% == 0 (
	copy "w:\build\main.exe" "w:\build\mainRunning.exe"
	 w:\build\mainRunning.exe
)
popd 