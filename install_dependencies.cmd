@echo off
setlocal EnableDelayedExpansion
setlocal enableextensions
rem -- Script process:
rem -- 1. Check for CMake in dependencies folder or on the machine: Install CMake in dependencies folder if needed
rem -- 2. Build dependencies CMake project - New way of installing dependencies
rem -- 3. Install remaining dependencies using the scripts
rem -- 4. The aim is that over time, all dependencies will be delt with through CMake, and old scripts will be removed.

set platformTarget=x64
set buildType=Release

:parameter_parse
if /i "%1"=="-h" (
	Goto print_usage
) else if /i "%1"=="--help" (
	Goto print_usage
) else if /i "%1"=="--debug" (
    set buildType=Debug
    SHIFT
	Goto parameter_parse
) else if /i "%1"=="--release" (
    set buildType=Release
    SHIFT
	Goto parameter_parse
) else if /i "%1"=="--platform-target" (
	set platformTarget=%2
	SHIFT
	SHIFT
	Goto parameter_parse
) else if /i "%1" neq "" (
	echo Unknown parameter "%1"
	Goto print_usage
)

rem -- #############################################################################
rem -- Check for CMake Version
rem -- Install CMake in dependencies dir if needed
rem -- #############################################################################

rem -- CMake minimum version required (major.minor)
set versionMajor=3
set versionMinor=16
set versionPatch=9


set workDir=%cd%
set baseDir=%~dp0
set dependenciesDir=%baseDir%\dependencies
if /i "%platformTarget%" neq "x86" (
	set dependenciesDir=%dependenciesDir%_%platformTarget%
)

set dependenciesDirArchives=%dependenciesDir%\arch

if not EXIST %dependenciesDir% (
    md %dependenciesDir%
)
if not EXIST %dependenciesDirArchives% (
    md %dependenciesDirArchives%
)

set PATH=%PATH%;%dependenciesDir%\cmake\bin

WHERE cmake >NUL
if not errorlevel 1 (
    rem get cmake version
    set firstLine=1
    for /f "usebackq tokens=3" %%i in (`cmake --version`) do (
        if !firstLine!==1 set cmakeVersionFound=%%i
        set firstLine=0
    )

    rem check version found
    for /f "tokens=1,2 delims=." %%i in ("!cmakeVersionFound!") do (
        if  %%i LSS %versionMajor% goto cmakeIsNeeded
        if  %%i GTR %versionMajor% goto cmakeIsNotNeeded
        if  %%j LSS %versionMinor% goto cmakeIsNeeded
        goto cmakeIsNotNeeded
    )

    :cmakeIsNeeded
    echo CMake version found [!cmakeVersionFound!] is lower than required [%versionMajor%.%versionMinor%.%versionPatch%]
    set cmakeNeeded=y
    goto next

    :cmakeIsNotNeeded
    echo CMake version found is Ok
    set cmakeNeeded=n
    goto next
) else (
    echo CMake not found on the machine
    set cmakeNeeded=y
)

:next
if  "%cmakeNeeded%" == "y" (
    echo Installing CMake version %versionMajor%.%VersionMinor% in %dependenciesDir%

    if /i "%platformTarget%" equ "x64" (
        set cmakeFolder=cmake-%versionMajor%.%VersionMinor%.%versionPatch%-win64-x64
    ) else (
        set cmakeFolder=cmake-%versionMajor%.%VersionMinor%.%versionPatch%-win32-x86
    )

    cd %dependenciesDirArchives%
    if not exist !cmakeFolder! (
        if not exist !cmakeFolder!.zip (
            rem get archive
            powershell -NoProfile -Command "Invoke-WebRequest  http://www.cmake.org/files/v%versionMajor%.%versionMinor%/!cmakeFolder!.zip -OutFile !cmakeFolder!".zip
        )
        rem extract archive
        echo Extract cmake archive
        powershell -NoProfile -Command "Expand-Archive !cmakeFolder!.zip -DestinationPath ."
    )
    powershell -NoProfile -Command "Copy-Item -Path !cmakeFolder! -Destination %dependenciesDir%\cmake -Recurse -Force"

    cd %workDir%
)

rem -- #############################################################################
rem -- Dependencies install - CMake project
rem -- New preferred method which is cross-platform
rem -- #############################################################################

mkdir %baseDir%\external_projects\build\%buildType% >NUL
cd %baseDir%\external_projects\build\%buildType%

rem Initialise compiler environment and cmakeGenerator variable
call %baseDir%\windows-init-env.cmd --platform-target %platformTarget%

if /i "%platformTarget%" equ "x64" (
	set generatorPlatform=x64
) else (
    set generatorPlatform=Win32
)

cmake ..\.. -G %cmakeGenerator% -DCMAKE_BUILD_TYPE=%buildType% -DEP_DEPENDENCIES_DIR=%dependenciesDir%
msbuild Dependencies.sln /p:Configuration=%buildType% /p:Platform=!generatorPlatform! /verbosity:minimal

rem -- #############################################################################
rem -- Install remaining dependencies - original script method
rem -- Deprecated method
rem -- #############################################################################

echo Installing dependencies for build target %platformTarget%

rem The dependencies are hosted by Inria
set PROXYPASS=anon:anon
set URL=http://openvibe.inria.fr/dependencies/win32/3.5.0/

if not exist "%dependenciesDir%\arch\data" ( mkdir "%dependenciesDir%\arch\data" )
if not exist "%dependenciesDir%\arch\build\windows" ( mkdir "%dependenciesDir%\arch\build\windows" )

cd %baseDir%\sdk\scripts
powershell.exe -NoProfile -ExecutionPolicy Bypass -file "%baseDir%\sdk\scripts\windows-get-dependencies.ps1" -manifest_file .\windows-build-tools.txt -dest_dir %dependenciesDir%
call :check_errors !errorlevel! "Build tools" || exit /b !_errlevel!

powershell.exe -NoProfile -ExecutionPolicy Bypass -file "%baseDir%\sdk\scripts\windows-get-dependencies.ps1" -manifest_file .\windows-dependencies-%platformTarget%.txt -dest_dir %dependenciesDir%
call :check_errors !errorlevel! "SDK" || exit /b !_errlevel!

powershell.exe -NoProfile -ExecutionPolicy Bypass -file "%baseDir%\sdk\scripts\windows-get-dependencies.ps1" -manifest_file .\tests-data.txt -dest_dir %dependenciesDir%
call :check_errors !errorlevel! "SDK tests" || exit /b !_errlevel!


echo Installing Designer dependencies
cd %baseDir%\designer\scripts
powershell.exe -NoProfile -ExecutionPolicy Bypass -file "%baseDir%\sdk\scripts\windows-get-dependencies.ps1" -manifest_file  .\windows-dependencies-%platformTarget%.txt -dest_dir %dependenciesDir%
call :check_errors !errorlevel! "Designer" || exit /b !_errlevel!

echo Installing OpenViBE extras dependencies
cd %baseDir%\extras\scripts
powershell.exe -NoProfile -ExecutionPolicy Bypass -file "%baseDir%\sdk\scripts\windows-get-dependencies.ps1" -manifest_file .\windows-dependencies-%platformTarget%.txt -dest_dir %dependenciesDir%
call :check_errors !errorlevel! "Extras" || exit /b !_errlevel!

echo Creating OpenViBE extras dependency path setup script
set "dependencyCmd=%dependenciesDir%\windows-dependencies.cmd"
echo @ECHO OFF >%dependencyCmd%
echo. >>%dependencyCmd%
echo SET "dependencies_base=%dependenciesDir%" >>%dependencyCmd%
echo. >>%dependencyCmd%
type %baseDir%\extras\scripts\windows-dependencies.cmd-base >>%dependencyCmd%

echo Done.
exit /b 0

:print_usage
echo "Usage: install_dependencies.cmd [--platform-target <target>]"
echo "    --platform-target <target>. Options are "x64" and "x86". Default is x64."
pause
exit 0

:check_errors
SET _errlevel=%1
SET _stageName=%2
if !_errlevel! neq 0 (
	echo Error while installing !_stageName! dependencies
	exit /b !_errlevel!
) else (
    echo All good!
)
