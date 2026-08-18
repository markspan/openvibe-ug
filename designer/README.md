# OpenViBE Designer

This repository contains the Designer graphical interface, based on OpenViBE SDK. It also contains various visualization plugins.

## Build

In order to compile this project you will need to have the openvibe-sdk dependency compiled somewhere. You can install the last released version 
via dependencies or use a folder compiled locally folder.

Let us call this path `PATH_OPENVIBE_SDK`. The other requirement is to know where Certivibe dependencies can be found, let us call this path 
`PATH_OPENVIBE_SDK_DEPENDENCIES`. Finally, let us call the location of the source code `PATH_DESIGNER_SOURCE`.

### Pre-requisites

In order to build Designer, you need a compiler installed. On Windows you need to have Visual Studio 2013 installed, the build system is based on CMake and Ninja, 
if you don't want to install it on your system, then you can unzip it locally with command by:

Going to `PATH_DESIGNER_SOURCE\scripts` and open command terminal, set variable PROXYPASS with `username:passwd` and run (or use `windows-install-dependencies-auth.cmd` -> see below):

```
    powershell.exe -NoProfile -ExecutionPolicy Bypass -file "windows-get-dependencies.ps1" -manifest_file .\windows-build-tools.txt
```

### Windows

#### Installing Designer Dependencies

The simplest way to install dependencies, is, in folder `scripts`:

* copy `windows-install-dependencies-auth.cmd-skeleton`
* rename it to `windows-install-dependencies-auth.cmd` 
* edit line `set PROXYPASS=XXX:XXX` with appropriate usernamee and password
* run it whenever you want to update dependencies.

This file is ignored by Git, since credentials should not be commited to repository.

This script install build tools, SDK dependencies and Designer dependencies. If you have your own CMake install and want to use a local version of openvibe-sdk 
you can comment the lines build-tools.txt.

#### Compile the source code via the script

The build script can be found in `PATH_DESIGNER_SOURCE\scripts`

To build Designer in Release mode run:

     windows-build.cmd --sdk PATH_OPENVIBE_SDK --dep PATH_OPENVIBE_SDK_DEPENDENCIES

#### Creating a Visual Studio project

A Visual Studio project can be created using scripts. A generator can be found in the `PATH_DESIGNER_SOURCE\scripts` folder.

    windows-generate-vs-project.cmd --sdk "PATH_OPENVIBE_SDK" --dep "PATH_OPENVIBE_SDK_DEPENDENCIES"

In order to open the visual studio with the correct paths:

    windows-launch-visual-studio.cmd --sdk "PATH_OPENVIBE_SDK"

Note that currently, only building the project in Release mode is supported if you are using Visual Studio.

### Linux

#### Installing Designer Dependencies

This installation guide supposes that you have already installed the OpenViBE SDK dependencies.

Go to `PATH_DESIGNER_SOURCE/scripts` and run

    perl linux-install-dependencies.pl
    
You will be asked for your root password which you have to grant to the script.

#### Compile the source code via the script

The build script is in `PATH_DESIGNER_SOURCE/scripts`

Run it as so:

    ./unix-build --sdk=PATH_OPENVIBE_SDK
    
This will build and install Designer in release mode into `PATH_DESIGNER_SOURCE/build/dist`
