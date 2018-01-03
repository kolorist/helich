@echo off

pushd %~d0

set msvcdir="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\"
if not defined DevEnvDir call %msvcdir%vcvars64.bat >nul

popd %~d0

if not exist "build" md "build"
if not exist "build/obj" md "build/obj"

rem --------------------------------------------
rem Compiling
rem --------------------------------------------
rem /Fo		object file output path
rem /Fd		target file output path
rem /Zi		generate .pdb
rem /Od		disable optimization (Debug)
rem --------------------------------------------
echo Compiling...
call cl.exe										^
	/nologo										^
	/I "."										^
	/I "./include"								^
	/I "../floral/include"						^
	-D_CRT_SECURE_NO_WARNINGS					^
	-DPLATFORM_WINDOWS							^
	/Fo"./build/obj/"							^
	/Fd"./build/"								^
	/Zi											^
	-Od											^
	-c											^
	src/cu/unitybuild_0.cpp

rem --------------------------------------------
rem Linking
rem --------------------------------------------
echo Linking...
call lib.exe									^
	/nologo										^
	build/obj/unitybuild_0.obj					^
	/OUT:"./build/helich.lib"
