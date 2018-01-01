@echo off

pushd %~d0

set msvcdir="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\"
if not defined DevEnvDir call %msvcdir%vcvars64.bat >nul

popd %~d0

cl 	/nologo										^
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

lib	/nologo										^
	build/obj/unitybuild_0.obj					^
	/OUT:"./build/helich.lib"
