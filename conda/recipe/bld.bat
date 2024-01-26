@echo off

mkdir build
cd build

set BUILD_CONFIG=Release

cmake ..  -G "Ninja" ^
    -Wno-dev ^
    -DCMAKE_INSTALL_PREFIX=%LIBRARY_PREFIX% ^
    -DCMAKE_PREFIX_PATH=%LIBRARY_PREFIX% ^
    -DCMAKE_INSTALL_RPATH:STRING=%LIBRARY_LIB% ^
    -DCMAKE_INSTALL_NAME_DIR=%LIBRARY_LIB% ^
    -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 exit 1

cmake --build . --target install
if errorlevel 1 exit 1
