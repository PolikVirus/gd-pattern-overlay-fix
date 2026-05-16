@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
set GEODE_SDK=C:\Users\OasenHase\Documents\Geode
set GEODE_DISABLE_PRECOMPILED_HEADERS=ON
set CMAKE_GENERATOR_INSTANCE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools
cd /d C:\Users\OasenHase\gdr-pattern-geode\build
cmake .. -DGEODE_DISABLE_PRECOMPILED_HEADERS=ON
cmake --build . --config Release --target GDRPatternReader -- /m:1
exit /b %ERRORLEVEL%
