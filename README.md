# |openvibe repository| |README|

[![Website](https://img.shields.io/badge/Web-Website-informational)](http://openvibe.inria.fr/)
[![Doxygen Documentation](https://img.shields.io/badge/Doc-Doxygen%20Documentation-informational)](http://openvibe.inria.fr/documentation/latest/)
[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)

OpenViBE project is now a mono-repository. The sdk/, deigner/ and extras/ submodules are now normal directories of this repository — they used to be submodules pointing at the `openvibe/sdk`, `openvibe/designer` and `openvibe/extras` projects, which are now archived.

# Compiling openvibe on your machine
Compilers will come with the pixi environment for **Linux** (*gcc*) and **Macos** (*clang*).

For **Windows**, you should install [*Visual Studio 2026* ](https://my.visualstudio.com/Downloads?q=visual%20studio%202026)

## Prerequesite (**Windows Only**)
Due to difficulties in setting up runtime environment for OpenViBE using the conda package for Qt, windows developers need to install Qt on their machine.

  1. You can download [**Qt for Open-Source**](https://www.qt.io/cs/c/?cta_guid=074ddad0-fdef-4e53-8aa8-5e8a876d6ab4&signature=AAH58kH-aVfoOAf0GHxdaZymVF25a9At1Q&utm_referrer=https%3A%2F%2Fwww.qt.io%2Ffree-trial&portal_id=149513&pageId=12602948080&placement_guid=99d9dd4f-5681-48d2-b096-470725510d34&click=2cc78381-f5e0-4ef0-af60-e4585364739e&redirect_url=APefjpEj7GufSYld4otxB_iM2cw7wDN2tMqxAfVfhPL5eouDBv5NOz7lEGvqxU4Nl2Sg71FooE71esKnaWhhehCgh99xZsBhYkRouoFAfXqIf464lWnkOa3JmITCZJsH7uBPckxhF1aCs2leK0Ff33rmNmf4kg_QqzmCmnkAt9wi5gdWWFOc-yLO8a98BsM2YsTA5LDxEwBeZH5oguV6kFHFfgNO1ZP8k3u9kQR_OdqJzAV12dDHjdP0v2DzssIqSJmXnHiHmP1ZtHixFFhsJt3pgl1K_lwaug&hsutk=47c9b2a88af5696f8ae07d56b9dc1a44&canon=https%3A%2F%2Fwww.qt.io%2Fdownload-open-source&__hstc=152220518.47c9b2a88af5696f8ae07d56b9dc1a44.1710851878834.1712223149367.1713185696258.6&__hssc=152220518.9.1713185696258&__hsfp=3802836240&contentType=standard-page/)
  2. Install Qt 6.9.3

## Steps (All platforms)

  1. install the package manager [**pixi**] (https://pixi.prefix.dev/latest/installation/)
(for **Windows**, use a powershell for the following steps)
  2. clone OpenVibe: `git clone https://gitlab.inria.fr/openvibe/openvibe.git`
  3. go to the directory: `cd openvibe`
  4. build openvibe: `pixi run build`

 7. Launch:
    - 7.1 **Linux and Mac** : `./bin/designer`
    - 7.2 **Windows** `.\bin\designer.exe`

8. **Optional**:  Launch the tests
    - 8.1 move to the build directory: `cd build`
    - 8.2 run the tests: `ctest -T Test --output-on-failure`

## Updating the repository

```bash
git pull
```
