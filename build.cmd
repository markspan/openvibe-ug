@echo off
setlocal EnableDelayedExpansion
setlocal enableextensions

set baseDir=%~dp0

if exist "%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/" (
	for /f "usebackq delims=#" %%a in (`"%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/vswhere" -version [16.0^,17.0] -latest -property installationPath`) do (
		set "visualStudioTools=%%a"
	)

	set "vcvarsallPath=/VC/Auxiliary/Build/vcvars64.bat"
)

if exist "%visualStudioTools%%vcvarsallPath%" (
    echo "Found %cmakeGenerator% tools: %visualStudioTools%%vcvarsallPath%"
    call "%visualStudioTools%%vcvarsallPath%" 
) else (
    echo,
    echo **************************************************
    echo,
    if exist "%visualStudioTools%" (
        echo ERROR: Could not find C/C++ support in "%visualStudioTools%"
        echo        Please, ensure that you have selected the "Desktop Development with C++" module in your Visual Studio installation.
    ) else (
        echo ERROR: Could not find Visual Studio 2019.
		echo You can try building OpenViBE manually with other version of Visual Studio.
    )
    echo,
    echo **************************************************
    echo,
    pause
    exit 1
)

mkdir %baseDir%\build\
cd %baseDir%\build\

cmake %baseDir% -G Ninja
if not "!ERRORLEVEL!" == "0" goto terminate_error

ninja -j 4
if not "!ERRORLEVEL!" == "0" goto terminate_error


:terminate_success
exit 0

:terminate_error
echo.
echo An error occured during building process !
echo.
exit /b 1

