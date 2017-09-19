@echo off
if exist build (
	rmdir /s /q build
)
mkdir build
pushd "%~dp0"
cd build
call cmake -G "Visual Studio 14 2015" ../tests
call cmake --build . --target ALL_BUILD --config Debug
popd
echo Done clean-building. Enjoy :D
