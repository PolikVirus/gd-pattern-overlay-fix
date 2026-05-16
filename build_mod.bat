@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
set GEODE_SDK=C:\Users\OasenHase\Documents\Geode
set GEODE_DISABLE_PRECOMPILED_HEADERS=ON
set CMAKE_GENERATOR_INSTANCE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools
set "CMAKE_GENERATOR=Visual Studio 17 2022"
set CMAKE_GENERATOR_PLATFORM=x64
cd /d C:\Users\OasenHase\gdr-pattern-geode\build
cmake .. -DGEODE_DISABLE_PRECOMPILED_HEADERS=ON
if errorlevel 1 exit /b 1
cmake --build . --config Release --target GDRPatternReader -- /m:1
exit /b %ERRORLEVEL%
