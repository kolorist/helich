@echo off
pushd "%~dp0"
if not exist build (
	mkdir build
)
cd build
call cmake -G "Visual Studio 14 2015 Win64" ../tests
call cmake --build . --target ALL_BUILD --config Debug
echo Building done, enjoy :D
popd
pause