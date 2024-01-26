# |OpenViBE meta repository| |README|

[![Website](https://img.shields.io/badge/Web-Website-informational)](http://openvibe.inria.fr/)
[![Doxygen Documentation](https://img.shields.io/badge/Doc-Doxygen%20Documentation-informational)](http://openvibe.inria.fr/documentation/latest/)
[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)

OpenViBE project is now divided into 3 parts :

- SDK, that contains the certifiable core and plugins of OpenViBE
- Designer, the graphical interface for OpenViBE
- Extras, for community plugins and contributions

The current repository, OpenViBE-meta, exists to bring the three repositories together and build the project.

# User Install

Here are the installation steps For **Using Openvibe** . If you want to develop in Openvibe, please refer to the section *Developer Install*: 

1. install miniconda: https://docs.conda.io/projects/miniconda/en/latest/
2. create an empty conda environment and activate it: `conda create -n openvibe` and `conda activate openvibe`
3. install openvibe package: `conda install -c openvibe -c conda-forge openvibe`
4. Launch: `openvibe-designer`


# Developer Install
Compilers will come with the conda environment for **Linux** (*gcc*) and **Macos** (*clang*). For **Windows**, you should install [*Visual Studio 2019* ](https://my.visualstudio.com/Downloads?q=visual%20studio%202019&wt.mc_id=o~msft~vscom~older-downloads)

1. install miniconda: https://docs.conda.io/projects/miniconda/en/latest/
(for **Windows**, use a powershell for the following steps)
2. clone OpenVibe: `git clone --recurse-submodules git@gitlab.inria.fr:openvibe/meta.git`
3. install openvibe dependencies: `conda env update -f conda/env_{linux|osx|windows}.yaml`
4. activate the environment: `conda activate openvibe`
5. make build dir: `mkdir build ; cd build`
  - 5.1 **Windows Only** set up the shell:
```
$vsPath = &"${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -version '[16.0,17.0)' -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationpath
Import-Module (Get-ChildItem $vsPath -Recurse -File -Filter Microsoft.VisualStudio.DevShell.dll).FullName
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64'
```
6. configure and build:
  - 6.1 **Linux and Mac**: `cmake .. && make -j4`
  - 6.2 **Windows**: `cmake .. -G Ninja ; ninja`

7. Launch:
  - 7.1 **Linux and Mac** : `./bin/openvibe-designer`
  - 7.2 **Windows** `.\bin\openvibe-designer`

8. **Optional**:  Launch the tests (from the build dir)
  - 8.1 Extras `cd extras; cTest -T Test --output-on-failure ; cd ..`
  - 8.2 unit-test: `cd sdk/unit-test ; cTest -T Test --output-on-failure ; cd ../..`
  - 8.2 validation-test: `cd sdk/validation-test ; cTest -T Test --output-on-failure ; cd ../..`


## Updating the repository

You can update the whole directory (including submodules) with :

```bash
git pull
git submodule sync --recursive
git submodule update --init --recursive
```

Aliases can be created to ease the global update process :
`git config --global alias.spull '!git pull && git submodule sync --recursive && git submodule update --init --recursive'`
