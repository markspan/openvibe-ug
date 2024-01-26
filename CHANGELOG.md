# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [3.6.0] 2024-01-25

### Added
 - Build: conda env for dependency management
 - Build: OSX support (Intel) except Advanced Visualization
 - Box: PulseRateCalculator
 - Box: Asymmetry Index Metabox
 - CI: gitlab-ci

### Updated
 - Box: LDA Classifier scale dependant
 - Box: Classifier trainer randomized k-fold option move from conf to box settings
 - Dependency: Boost version 1.71 -> 1.77
 - Dependency: Eigen version 3.3.7 -> 3.3.8
 - Dependency: Expat version 2.1.0 -> 2.5.0
 - Dependency: Xerces-C version 3.1.3 -> 3.2.4
 - Dependency: OGG version 1.2.1 -> 1.3.4
 - Dependency: Vorbis version 1.3.2 -> 1.3.7
 - Dependency: Lua version 5.1.4 -> 5.4.6

### Removed
 - Build: CMake ExternalProjects dependencies (now in conda)
 - Build: Scripted dependency management
 - CI: Jenkins CI (now in gitlab)

## [3.5.0] 2023-04-11

### Added
 - Driver: Shimmer3 GSR+ physiological sensor (extras#167)

### Updated
 - Dependency: Boost Regex config (meta#61)
 - Box: Feature Aggregator chunk times checks reduced (sdk#140)
 - Driver: ANTNeuro EEGO SDK (extras#209)
 - Build: Missing includes added for gcc 12.2.1

### Fixed
 - Box: Temporal Filter parameter checks (sdk#139)
 - Box: Regularized CSP matrix inversion check (sdk#145)
 - Box: Univariate Statistics output format (extras#205)
 - Driver: EEGO namespace position
 - Driver: BrainProducts actiCHamp driver (extras#204)
 - DevTool: Skeleton Generator categories parsing (extras#212)

## [3.4.0] 2022-12-05

### Added

- Doc: File formats documentation to doxygen (Issue meta#45)
- SDK: std::string cast to FSettingValueAutoCast (Issue sdk#113)
- Box: Actions in ArticfactAmplitude box (Issue extras#52)
- Box: Stimulation Converter box (Issue extras#198)
- Box: Input Switch box (Issue extras#199)
- Box: Connectivity Spectrum extract (extras#138)
- Box: Matrix 2D to Vector (extras#138)
- Box: Added Burg's method to Connectivity box (extras#138)
- Box: Added PSD output to AutoRegressive Coefficient box (extras#138)
- Build: MSVC 2017 support (Issue meta#46, meta#50, sdk#107, extras#136)
- Build: MSVC 2019 support (Issue meta#57, sdk#138 extras#195)
- Build: Debug build (Issue meta#51, sdk#115 extras#184)
- CI: XML comparison applicaiton (Issue extras#133)
- CI: Non regression tests (Issue extras#132 extras#144)


### Changed
- Driver: BrainProducts actiCHamp and LiveAmp functionnal with latest BrainProducts SDK (Issue meta#58, extras#196)
- SDK: Remove IObject inheritence from CMatrix and CStimulationSet classes (Issue sdk#120)
- SDK: CKernel class refactoring (Issue sdk#114)
- SDK: CIdentifier to use std::string instead of CString (Issue sdk#116 designer#47)
- SDK: StringDirectories refactring (Issue sdk#117 designer#48 extras#156)
- SDK: CMemoryBuffer refactoring (Issue sdk#121 designer#49 extras#157)
- Box: GDFWriter header Physical min/max and Digital min/max (Issue extras#193)
- Build: TinyXML moved to external projects (Issue meta#52 designer#56 extras#163 extras#186)
- CI: README.md for CI status on gitlab (Issue meta#54)
- CI: DartTests refactoring (Issue extras#127)
- CI: Update CSV comparison application (Issue extras#125)
- Doc: Update copyright mention (Issue sdk#128 designer#51 extras#179)
- Run: Update launch scripts (Issue designer#59)
- Dev: Update skeleton-generator application (Issue extras#165)

### Fixed

- Box: CSV File Writer/Reader linebreaks handling in labels (Issue sdk#99)
- Box: CSV File Writer/Reader to handle only stimulations (Issue sdk#100)
- Box: Revert C++14 feature - too advance (Issue sdk#108)
- Box: NoiseGenerator default setting (Issue extras#130)
- Box: KeyboardStimulator string to integer conversion (Issue extras#137)
- Box: InverseDWT header sent header multiple times (extras#143)
- Box: StimulationValidator sent header multiple times (extras#135)
- Box: Sampling frequency in ArtifactAmplitude (extras#180)
- Box: Sampling rate referencing in IFFT and FastICA boxes (extras#181)
- Build: Powershell call on windows install dependencies script (issue meta#44)
- Build: windows build env init for x86 (issue meta#49)
- Build: windows install-dependencies param parsing (Issue meta#56)
- Build: Remove implicit cast warnings (Issue sdk#103 sdk#104 designer#40 designer#42 designer#43)
- Build: Remove CLang warnings (Issue sdk#126 sdk#129 sdk#130 sdk#131, sdk#133 sdk#134 designer#57 designer#58 extras#139 extras#140 extras#148 extras#150 extras#159 extras#160 extras#161 extras#169-extras#177 extras#187-extras#190)
- Build: Remove warning in unit-test (Issue sdk#118 extras#155)
- Doc: pybox-manager documentation generation (extras#182)

### Removed
- Box: Version descriptors (Issue sdk#127 designer#50 extras#178)
- Box: Box description CString casts (Issue sdk#109 extras#141)
- SDK: XML module legacy proxy (Issue sdk#125)
- SDK: Remove unused Date module (Issue sdk#136 designer#53)
- Dev: Redundant semicolon after macro calls (Issue sdk#112 extras#151)
- Dev: Eigen-found guards in dependent classes (Issue sdk#122 extras#158)
- Build: Unneeded library links (Issue sdk#137 designer#55 extras#185)
- Build: NSIS script vcredist install (extras#194)

## [3.3.1] 2022-07-11

### Fixed
 - Box: GDF File Writer Event Table header (extras#166)

## [3.3.0] 2022-04-21

### Added

- Box : LSL generic Communication BOx (Import/Export) (Issue extras#101)
- Dev : LSL Module (Issue extras#108)
- Dev : Eigen Module (Issue extras#107)
- Dev : Guidelines markdown (Issue meta#31)
- Build : External Projects - Dependencies install from sources with CMake (Issue meta#30)

### Changed

- Box : Temporal Filter settings. Allow multiple temporal filters (Issue sdk#71, designer#32, extras#98)
- Box : Stimulation Based Epoching handle multiple events (Merge Request sdk#194)
- Box : Boxes in validation test scenarios (Issue sdk#75)
- Box : Sign Change Detector box changed to Threshold Crossing Detector (Issue extras#123) 
- Dev : CMatrix buffer copy (Issue sdk#73)
- Dev : CMatrix helper methods (Issue sdk#47)
- Dev : CStimulationSet Class helper methods (Issue sdk#76, designer#33, extras#103)
- Dev : CNameValuePairList API (Issue sdk#81)
- Dev : Itpp replaced by Eigen on signal processing plugin, and tests added (Issue extras#114)
- Doc : Doxygen generation parsing all code and functioning on all platforms (Issue meta#33, sdk#90, designer#36, extras#115)
- Doc : Doxygen specify deprecated functions (Issue meta#36, sdk#91, sdk#93)
- Doc : Dox-part parsing for open range (Issue meta#39)
- Build : Dependencies install allowed on any version for Fedora and Ubuntu (Issue sdk#96)
- Build : Boost moved to external projects and version updated to 1.71 (Issue meta#30, sdk#86, designer#35, extras#100)
- Build : LUA moved to external projects (Issue meta#34, extras#116)
- Build : Eigen moved to external projects (Issue meta#37, sdk#94, designer#37, extras#117)
- Build : GTest moved to external projects and version updated to 1.8 (Issue meta#40, sdk#95, extras#119)
- Build : VRPN moved to external projects (Issue meta#41, extras#120)

### Fixed

- Box : Matrix variance computation (Issue extras#92)
- Box : Boxes in CSP trainer metabox (Issue extras#112)
- Box : Classifier setting update (Issue sdk#69)
- Box : Stimulation multiplexer time concurrency (Issue sdk#88)
- Box : xDawn box file saving (Issue sdk#89)
- Build : CMake default install prefix (Issue meta#29)
- Build : Bug Linux install dependencies (Issue meta#38)
- Build : Out of tree compilation (Issue meta#27)
- Dev : XML module read-write parsing (Issue sdk#83)
- Doc : Fix typos in some documentations (Issue extras#113)

### Removed

- Dev : IMatrix interface for CMatrix class (Issue sdk#47)
- Dev : IStimulationSet interface for CStimulationSet Class (Issue sdk#76, designer#33, extras#103)
- Dev : IError interface for CError Class (Issue sdk#77, designer#34, extras#109)
- Dev : PImpl idiom for CNameValuePairList class (Issue sdk#81)

## [3.2.0] 2021-10-25 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=10240))

### Added

- Box : Metabox to perform log of signal power (Issue extras#95)
- Data : Artifacted files for algorithm tests (Issue extras#91)

### Changed

- Build : Refactored CMake build process (Issue meta#14, sdk#49, designer#14, extras#50)
- Dev : Update wildcards in gitignore (Issue meta#24, sdk#63, designer#28, extras#89)
- Box : Update CSV File Writer/Reader - Stimulations only (Issue sdk#55)

### Removed

- Dependency : Ogre games and dependencies (Issue meta#23, sdk#68, designer#27, extras#88)
- Build : Mensia distribution (Issue designer#19)

### Fixed

- Build : Intermittent bug compiler (Issue designer#31)

## [3.1.0] 2021-04-30 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=10203))

### Added

- Box : Artifact Subspace Reconstruction (ASR) Box (Issue extras#45)
- Box : Matrix 3D to 2D Box (Issue extras#64)
- Box : Bidirectionnal Unity games box (Issue extras#61)
- Doc : Changelog (Issue meta#15)

### Changed

- Dependency : Update eigen dependency to 3.3.7 (Issue meta#13, sdk#48, extras#49)
- Box : Changed label Artefact-detection to Artifact (which may contain detection or reconstruction or anything related to artifacts) (Issue extras#45)
- Box : New implementation of Connectivity Measure Box (Issue extras#64)
- Dev : Update Geometry Module For ASR Algorithm (Issue extras#45)
- Dev : Improve Kernel CIdentifier Class (Issue sdk#50, designer#15, extras#53)
- Dev : Improve Kernel CMatrix Class (Issue sdk#47, designer#13, extras#47)
- Dev : LSL timestamps configurable (OpenViBE clock or LSL clock). Default is LSL clock (extras#63)
- Dev : Replace using directives with namespace encapsulation (sdk#32, designer#4, extras#15)
- Dev : libSVM updated to v3.2.4 (Issue extras#58)
- Doc : Replace Mensia namespace (Issue designer#16 & extras#54)
- Doc : Updated the online documentation and tutorials

### Deprecated

### Removed

- Algo: Duplicated legacy code for stimulation based epoching (extras#107)

### Fixed

- Box : P300 Speller feedback display (Issue extras#51)
- Box : TCPWriter header size 
- Build : it++ detection in cmake for unix systems
 

## [3.0.0] - 2020-12-10 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=10160))

### Added

- Driver : Encephalan driver
- Driver : Unicorn driver
- Box : Pybox-manager tool and box to use python sk-learn classifier
- Box : Multimodal Graz visualization
- Box : Artefact detection
- Box : Features selection
- Box : Stimulation validator
- Dev : Geometry module with Riemannian geometry
- Support : Ubuntu 18.04
- Support : Fedora 31

### Changed

- Support : 64Bit is now the standard build.
- Box : Some boxes have been updated (type, name, inputs...) and must be updated in previous scenarios before run.
- Dev : Python2 support dropped in favour of Python3 (Python 3.7 required) for the scripting box. Python provides a tool to help you migrate your scripts: https://docs.python.org/fr/3/library/2to3.html
- Dev : The code has had a lot of modernisation changes, and some refactoring which imply incompatibility with previous versions requiring changes. List of changes necessary if you would like to migrate one of your plugins, compatible with OpenViBE 2:
  - OpenViBE types were removed and replaced with standard types:
    - `OpenViBE::uintXX` -> `uintXX_t`
    - `OpenViBE::intXX` -> `intXX_t`
    - `OpenViBE::float64` -> `double`
    - `OpenViBE::boolean` -> `bool`
  - Some functions from `System::Memory` were removed and replaced with standard equivalent:
    - `System::Memory::copy()` -> `std::memcpy()`
    - `System::Memory::move()` -> `std::memmove()`
    - `System::Memory::set()` -> `std::memset()`
    - `System::Memory::compare()` -> `std::memcmp()`
  - Namespaces starting with OpenViBE having been split:
    - `OpenViBEPlugins` -> `OpenViBE::Plugins`
    - `OpenViBEToolkit` -> `OpenViBE::Toolkit`
    - `OpenViBEVisualizationToolkit` -> `OpenViBE::VisualizationToolkit`
  - Some ClassId variables were renamed:
    - variables containing `StreamedMatrixStreamDecoder` renamed with `StreamedMatrixDecoder`
    - e.g. `OVP_GD_ClassId_Algorithm_StreamedMatrixStreamDecoder` -> `OVP_GD_ClassId_Algorithm_StreamedMatrixDecoder`
    - variables with `StreamedMatrixStreamEncoder` renamed with `StreamedMatrixEncoder`
    - e.g. `OVP_GD_ClassId_Algorithm_StreamedMatrixStreamEncoder` -> `OVP_GD_ClassId_Algorithm_StreamedMatrixEncoder`
    - variables with `SamplingRate` renamed with `Sampling`
    - e.g. `OVP_GD_Algorithm_SignalEncoder_InputParameterId_SamplingRate` -> `OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling`
    - `OVP_ClassId_Plugin_VisualizationContext` -> `OVP_ClassId_Plugin_VisualizationCtx`
  - The include file `<openvibe/ovITimeArithmetics.h>` was removed and functionalities replaced:
    - New header: `#include <openvibe/ovITimeArithmetics.h>` -> `#include <openvibe/CTime.hpp>`
    - `TimeArithmetics::timeToSeconds(timeVar)` -> `CTime(timeVar).toSeconds()`
    - `TimeArrithmetics::SecondsToTime(timeVar)` -> `CTime(timeVar).time()`
    - `TimeArithmetics::TimeToSampleCount(freqVar, timeVar)` -> `CTime(time).toSampleCount(freqVar)`
  - In the package `ovp_main.cpp`, after the call to `OVP_Declare_Begin()`: `rPluginModuleContext` -> `context`
  - Signature of method `processInput()` overriden from class `TBoxAlgorithm` has changed (parameter type `uint32_t` -> `const size_t`):
    - `OpenViBE::boolean processInput(OpenViBE::uint32)` -> `bool processInput(const size_t index)`
  - Signature of method `hasFunctionality()` overridden from class `IPluginObjectDesc` (in sdk/openvibe/include/openvibe/plugins/ovIPluginObjectDesc.h) has changed:
    - `OpenViBE::boolean hasFunctionality(OpenViBE::CIdentifier functionalityIdentifier) const` -> `bool hasFunctionality(const EPluginFunctionality functionality) const`
    - `#define OVD_Functionality_Visualization` replaced by `EPluginFunctionality::Visualization`
  - Method renamed to match coding rules:
    - `TCPTagging::createStimulusSender()` -> `TCPTagging::CreateStimulusSender()`
  - Signature of box processClock() may have changed. Both were used, but now only the latter:
    - `bool processClock(Kernel::IMessageClock& msg)` -> `bool processClock(Kernel::CMessage& msg)`
- Doc : Updated the online documentation and tutorials
- Doc : Updated the CI Build Badge of Readme

### Removed

- Support : Ubuntu under 18.04  (it can work however but official support is no longer provided)
- Support : Fedora under 31 (it can work however but official support is no longer provided)

### Fixed

- Dev : Bug fixes made. No list was compiled.

## [2.2.0] - 2018-12-03 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=10002))

### Added

- Box : Null box that consumes input
- Box : Keypress Emulator box
- Box : Improvements to the ERP Plot box
- Server : Simulated Deviator to study effects of a drifting driver
- Tracker : A new data analysis application allowing to load, freely browse and process multiple EEG files simultaneously
- Scenarios : ERP Plot box tutorial
- Tests : more data to Regularized CSP test to avoid random fails
- Tests : Tests for Streamed Matrix Encoder/Decoder

### Changed

- Box : Added error checking to the Timeout box
- Box : Noise Generator can now also bake Gaussian noise
- Box : Added a warning to the old CSV File Reader
- Box : Added TCP Tagging support to the Keyboard Stimulator
- Box : Make channel rename accept any kind of input
- Dependency : Updated LSL dependency to 1.12
- Dev : Fully 64bit OpenViBE (optional dl): You can use 64bit Python and Matlab versions with it
- Dev : Update name to US style (i.e. normalise -> normalize).
- Doc : Updated the online documentation and tutorials
- Designer : Automatic box update (simple boxes that need update can be sorted out with right-click + update)
- Server : Acquisition Server will now complain if ports are already in use
- Server : Drift Correction can now have an initial skip period
- Scenarios : Updated the P300 Magic Card scenarios
- Scenarios : Update Channel Rename box in box tutorials scenarios after update of SDK to 1.1.0
- Scenarios : Make external processing example depend on toolkit compilation

### Fixed

- Dev : Bug fixes made. Details in release announcement.

## [2.1.0] - 2018-04-11 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=9916))

### Added

- Box : Brain Vision Format Writer
- Scenarios : Example servers for the external processing box

### Changed

- Driver : Added channel scaling to LiveAmp driver
- Dev : Update name to US style (i.e. normalise -> normalize).
- Scenarios : move the default working directory folder to a user folder and add defines for harcoded scenarios folder

### Removed

- Box : Useless "Box About" properties

### Fixed

- Driver : EEGO impedance errors which were the result of multiple instanciations of the factory object
- Dev : More bug fixes detailed in release announcement.

## [2.0.1] - 2018-01-31 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=9876))

### Added

- Driver : Generic Time Signal driver
- Driver : EEGO driver for linux
- Dev : Continuous Integration with jenkins
- Tests : Tests for time arithmetic tests
- Tests : Make tests system agnostic : now uses cmake & git internal commands

### Changed

- Server : Improve functionnality (driver can be deprecated, tests, validity check...)
- Dev : Remove CMake logs except error logs.
- Dev : Simplify ITimeArithmetics formulas to increase precision
- Dev : Add tests and verification in the Kernel. (signal sampling rate, socket...).
- Box : Add tolerance to chunk continuity in stimulation based epoching
- Doc : Updated the online documentation and tutorials

### Fixed

- Driver : Memory leaks in BrainVision Recorder driver
- Server : TCP Tagging
- Dev : Build bugs
- Dev : Memory leaks
- Doc : Documentation generation under linux OS
- More bug fixes detailed in release announcement

## [2.0.0] - 2017-10-30 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=9840))

**Remark** : This version changes everything in the code, you can consider that the project has changed completely. See [this page](http://openvibe.inria.fr/differences-between-1-x-and-2-x-series-of-openvibe/) to see major differences.

### Added

- Driver : BrainProducts LiveAmp driver
- Driver : Sampling rate estimation to the LSL driver
- Driver : LiveAmp8 and LiveAmp16 support for LiveAmp Driver
- Box : Temporal filter Box
- Box : Zero crossing detector Box
- Dev : Bluetooth connection to module socket
- Dev : Some flags for OpenMP to optimize its use when used as a backend for eigen
- Designer : Metabox example

### Changed

- Designer : Move the loglevel of scenario importer from Information to Trace
- Designer : Improve the readability and the number of error message.
- Server : Drift Correction is now disabled by default
- Dev : All the code is separated in 4 repository (meta, sdk, extra, designer) the sdk and the designer have the Certivibe label.
- Dev : Refactor, Update and improve build system (log during build, dependency install, CMake Version and package, Visual studio version, remove old path, python unfound don't stop the build...)
- Dev : Create script for build on each sub repository.
- Dev : Update error management system
- Dev : Refactoring, update and improve all the code, especially sdk and the kernel for the Certivibe.
- Dev : Update some code style to C++11 instead of C or boost function.
- Dev : Reduce cppcheck warning and errors.
- Box : Update and improve CSV file IO.
- Box : Change label for some boxes. 
- Box : Update some checking in box (size or format of input, settings, output...).
- Box : Update some default setting/input/output.
- Scenarios : Update previous scenarios, example and tutorial.

### Deprecated

- Box : Add flag deprecated to old CSV Boxes.
- Box : Add flag deprecated to xDAWN Spatial Trainer INRIA Box.

### Removed

- Dev : Remove unused flag, token and dependencies
- Doc : Remove the non-relevant documentation and boxes from tools, signal-processing, vrpn, classification, stimulation plugin
- Doc : Updated the online documentation and tutorials

### Fixed

- Box : Unicode handling in Boxes and source code.
- Box : Memory leaks. Details in release announcement.
- Dev : Bug fixes made. Details in release announcement.

## [1.3.0] - 2016-12-30 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=9716))

### Added

- Drivers : NeuroServo HID driver

### Changed

- App : Acquisition Server no longer loads unnecessary dlls
- Designer : Skip initialization of 3D context when visualization is disabled
- Drivers : Improvements to gusbamp-gipsa, mostly for multiple gusbamp combinations
  - Channel names can now be specified in multiamp case
  - Chooses the last amp by default if the user doesn't choose
  - If multiple amps are connected, assumes we want to simultaneously record from all of them
  - Should now remember the master amp across runs correctly
  - Fixed 'Show Device Name' not always working in gusbamp-gipsa
  - Improved gusbamp-gipsa multiamp identification
  - Tweaked gusbamp gipsa log levels + added a warning
  - Fixed the impedance checking to work with multiple amps
  - All channels are now correct with both the event channel disabled or enabled.
- Drivers : Emotiv driver support for SDK 3.3.3
  - Updated the web documentation
- Box : Added PCA & Whiten support to the FastICA box
- Box : BrainAmp FileWriter now always creates the marker header
- Box : Changed GDF File Writer to use ITimeArithmetics
- Box : Minor code cleanup & log tweaks
- Box : Refactored Regularized CSP Trainer box code
- Box : Regularized CSP Trainer now supports multiclass
- Box : The FFT plots no longer draw junk during the first frames
- Box : Made Analog VRPN box declare 0 channels before header is received
- Dev : Update cmake script to use --invisible designer option instead of --no-gui
- Dev : Update tests to use --invisible designer option instead of --no-gui
- Dev : Refactored StimulusSender to a new 'tcptagging' module
- Dev : Renamed Mind Shooter source files to avoid confusion
  - Changed Mind Shooter sources to refer to the new filenames
- Dev : Changed Mind Shooter to its own namespace for clarity
- Scenarios : Added stimulation output to ov2csv conversion scenario
- Scenarios : Minor cleanup to the P300 scenarios and scripts
- Scenarios : Simplified Multiclass Mind Shooter
  - Mind Shooter now uses multiclass CSP and a multiclass classifier
  - Scenario pipelines modified and simplified accordingly
  - Corrected channel->control mapping
  - TCP Tagging is now used to pass stims from the Shooter to the AS
  - Added Epoching Delay option to avoid ERP effects
  - Fixed the performance evaluation .lua script
  - Fixed the feedback computation in the ships' parts
  - All config files and logs will now be created under the scenario dir
  - Disabled the feedback by default as its not very intuitive
  - Previous 'Advanced Control' is now called 'Analog Control'
  - Refactored the code to clarify analog and discrete controls
  - The choice between the two can be done in the online scenario
  - Removed unnecessary parts from the scenarios
  - Made the shooter loglevel controllable and default to 'Information'
  - VSync and Full Screen selection are now reflected in the ogre.conf
  - Removed such .lua scripts which were no longer needed
  - Tweaked the performance evaluation scenario & lua to match the new pipeline
  - More safety checks to SSVEP Mind Shooter
  - Changed SSVEP Mind Shooter to use class probabilities
  - Cleaned up junk from the SSVEP acquisition test scenarios
  - Added some comments.
  - Updated the web documentation
- Scenarios : Original Mind Shooter scenarios are available as 'classic version'
- Dev : cmake now avoids printing the same things many times
- Dev : Added some comments.

### Fixed

- Box : BrainAmp File Writer not always creating a header
- Box : GDF File Reader box fix for BCI Competition IV files.
- Box : Acquisition Client crash when requiring update from 0.18
- Box : Inappropriate deallocation in LSL Export box
- Dev : Compilation on Arch Linux
- Dev : Some case issues in cmake scripts.
- Doc : The Lua Stimulator box doc
- Drivers : Channel scale in the OpenBCI driver
- Scenarios : Incorrect type ids in some test scenarios.

## [1.2.2] - 2016-07-27 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=9643))

### Fixed

- Drivers : Stabilization of the OpenBCI driver
- Drivers : gUSBAmp and gMobilab are no longer mutually exclusive in Acquisition Server
- Designer : Editing box parameter types in Designer
- App : Issues with SSVEP and Handball demos
- Dev : Compilation on Ubuntu 16.04
- Dev : Miscellaneous small fixes to examples, scenarios, boxes, etc

## [1.2.1] - 2016-06-20 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=9643))

### Fixed

- Box : Issue with stimulation params of the Matlab box
- Box : Issue with multiple simultaneous Matlab boxes
- Box : Lua and Python plugins not handling *_Number_* stimuli
- Dev : Bug in Windows installer nsi script ('vcomp120.dll not found' issue)
- Scenarios : Wrong placement of the xval param in the classifier trainer boxes
- Scenarios : Incorrect train triggers in the P300 demos

## [1.2.0] - 2016-05-20 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=9643))

### Added

- Drivers : GTec gNautilus driver
- Drivers : gUSBAmp (BCI-Lab) driver for Linux
- Drivers : Impendance check support for the EEGO driver
- Box : New and reliable software tagging facility : TCP tagging
- Box : Simple Outlier Removal box
- Scenarios : How to do elementary incremental learning
- Tests : Tests for Shrinkage LDA

### Changed

- App : Designer will now show version info in the title bar
- App : Changed Win clock granularity to 1ms in AS and Designer
- App : Designer will now inform of special box states on load
- App : Designer now warns if scenarios have unknown box algorithms
- Box : Dropped support for the old LDA classifiers, please retrain
- Box : Classifier Processor can now reload the model on receiving a stimulation
- Box : Various enhancements to the FastICA box
- Scenarios : Changed basic P300 to use the TCP Tagging
- Scenarios : VRPN examples now print out the used device names
- Scenarios : Updated the Motor Imagery scenarios to use TCP Tagging
- Dependency : Support for Ogre 1.9.0 and CEGUI 0.8.4
- Dev : VS2013 compatibility
- Dev : Scenario exporters will now include the openvibe version

### Fixed

- Drivers : Drift bug in gusbamp gipsa driver
- Box : Problem of xval setting placement in Classifier Trainer
- Box : Issue for 0,...,k-1 indexing to classifiers
- Box : Probability output of the SVM classifier
- Box : Coefficient parsing check in Spatial Filter box
- Dev : Addressed Ogre/CEGUI Find problems on Fedora 21
- Dev : Issue with heavy OpenMP/Eigen CPU loads

## [1.1.0] - 2015-10-02 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=9555))

### Added

- Drivers : eemagine EEGO
- Drivers : Brain Rhythm 8, Simulator, SmartBCI and Wearable Sensing Dry Sensor Interface.
- Box : Implemented MLP (MultiLayer Perceptron) classification algorithm.
- Box : Regularized version of the CSP filter trainer.
- Box : New plugin category for evaluation.
- Box : 3 new boxes to evaluation plugin : kappa coefficient, ROCCurve, General Statistics Generator.
- Scenarios : Example of stimulation passing to Python.
- Dev : Enabled the support of SSE2.
- Dev : Enabled multicore and vectorization in Eigen.
- Tests : Tests for Regularized CSP.
- Tests : Tests for ROC Curve, kappa coefficient and statistic generator.
- Tests : Tests for MLP.
- Tests : Tests for import/export matrix in toolkit.
- Tests : Test configuration file for OpenViBE.

### Changed

- App : Disabled the CoAdapt P300 speller which was only working on 0.18. The contributing authors are currently developing an even more robust P300 speller that is also easy to use. Stay tuned.
- Dev : Added ASCII import and export functions for IMatrix matrices
- Designer : Made fails explicit in scheduler.
- Designer : Added more type checking to scenario loader.
- Designer : Restore log level even in no-gui and invisible mode.
- Drivers : Extended Emotiv EPOC support to gnu/linux.
- Drivers : Added Software tagging to the universal TMSI driver.
- Drivers : Moved the check impedance option to be part of driver preferences.
- Drivers : Various updates to BrainProducts VAmp, BrainProducts Brainamp.
- Box : The LDA now has a native multiclass mode.
- Box : The Spatial Filter box now accepts a configuration file which contains a matrix (refer to the doc for the format specification).
- Box : The xDawn and the CSP trainer can generate matrix file for the spatial filter.
- Box : Various optimizations of the Spatial Filter.
- Box : Changed Signal Display refresh paradigm to make it handle bigger datasets.
- Box : TCP writer box sends Streamed Matrix and Signal in row-major order.
- Box : Moved Accurancy Measurement and Confusion Matrix to the evaluation plugin.

### Fixed

- Box : Multiple issues in Signal Display.
- Box : LDA algorithm (wrong shrinkage computation).
- Box : Several fixes and guards added into the classification plugin.
- Box : Properly handle wrong settings value in Temporal Filter and Modifiable Temporal Filter.
- Box : EDF implementation.
- Box : Removed incorrect stream restrictions in Stream Switch.
- Box : TCP writer end of line sequence.
- Server : Shortcuts.
- Server : Now avoids crashes if loading the driver ui failed.
- Server : Various minor fixes.
- Dev : All windows launch scripts to handle spaces better.

## [1.0.1] - 2015-05-05 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=9435))

### Added

- Tests : Tests for GDF Writer

### Changed

- App : Edit the tips of the message interface in the skeleton generator.
- Dev : Enabled NSIS installer logging
- Designer : Designer will now point to the version-specific web documentation

### Fixed

- App : Added verbosity to VR Demo & fixed an unimportant memory leak
- App : Bug with the stimulation stream in the skeleton generator.
- Dev : Build without Ogre
- Doc : Version number in the generated doc. Its now automatically generated.
- Drivers : Added include guards to LSL driver
- Drivers : Generic Oscillator channel naming when > 4chns
- Drivers : Inserted author information where it was missing
- Box : The type stimulation is available in the python box.
- Box : Indexing in sign change detector
- Box : GDF Reader no longer appends extra chunk
- Box : Improved error handling for missing classifiers
- Scenarios : Added instructions to the P300 scenarios
- Scenarios : Convert app to work with spaces in exe paths
- Scenarios : Lua stimulator tutorial (Mantis #172)
- Scenarios : Wrong LDA id.

## [1.0.0] - 2015-03-20 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=9435))

### Added

- LabStreamingLayer support : Acquisition Server will now be able to import and export data from LSL. A compatible LSL Export box has been introduced.
- Server : Acquisition Server autoplay via config token
- Designer : Config tokens can be speficied to Designer & AS on the command line, using --define
- Designer : Box configurations can now change the number and type of entries based on user selections
- Designer : Popup on initilization error to Designer
- Designer : drag and drop scenario file in designer (MANTIS issue #48)
- Box : Wavelet decompositions are supported
- Box : EOG Artifact removal boxes added
- Box : Magnitude Squared Coherence is now available in the Connectivity box
- Box : OpenViBE can now send commands to any OSC enabled target
- Drivers : MCS NVX, mBrainTrain Smarting
- Drivers : Calibration signal support to Gipsa's gUSBAmp driver
- Drivers : Telnet reader now supports arbitrary integer sampling rates
- Drivers : More error checking to gUSBAmp gipsa driver
- Dev : ADD launch option --memcheck for launch executable using valkyrie ( front-end for valgrind)
- App : Memory deallocation to vrpn examples
- Scenarios : Classification testing tutorial
- Scenarios : FastICA box tutorial
- Scenarios : Wavelet & EOG Denoising tutorial
- Scenarios : External Stimulation Connection example will now send 1 stim / sec
- Scenarios : Enabled Clock, Log and Nothing example boxes.
- Tests : Tests for generic stream reader/writer
- Tests : Tests for Classifier
- Tests : Tests for CSV file IO
- Tests : Tests for EBML write and read
- Tests : Script to find out boxes that are not in any scenario (in test/)

### Changed

- Drivers : Some drivers are now able to return the measurement units (e.g. microvolts). If you connect Measurement Unit stream from Acquisition Client to Signal Display, you can enable the unit display (in Toolbar).
- Drivers : Moved the BrainProducts ActiCHamp driver from contrib/ to core
- Drivers : Moved TMSiSDK.dll to be an external dependency
- Drivers : fixed Refa32B include guards
- Drivers : Moved the remaining driver .dlls to be external dependencies
- Designer : Signal Display has been improved to handle better large amounts of channels and to give user more control over data scaling
- Designer : Scenario .xmls contents no longer permute wildly across 'save scenario'
- Designer : allow easier pick for line and easier connection between boxes.
- Designer : Enable shortcuts after drag and drop a box.
- Designer : Designer will now return an error if it cannot open a scenario file for writing
- App : Skeleton Generator no longer depends on sed
- App : Modified plugin inspector to provide a list of all registered boxes
- Box : Channel Selector now supports Streamed Matrix type
- Box : Various improvements to CSV Writer : Added support for Streamed Matrix & Feature Vector, removed unused Compression setting, introduced setting to change the output precision
- Box : Unified all LDA type classifiers to one
- Box : Refactored Signal Concatenation not to ruin stimuli chunk structure
- Box : Changed some boxes to print channel numbers using 1,2,... indexing. No list was compiled.
- Box : Added some checks for invalid xml file to Classifier Processor.
- Box : Transposed the TCP Writer output
- Box : CSV file reader/writer : renamed default out/input (mantis issue 76)
- Box : Allow the automatic renaming of channel selector box based on channel list, mantis 64
- Box : External stimulations should now be passable to multiple clients
- Box : Moved the BioSemi ActiveTwo driver from contrib/ to core
- Box : Moved the TMSi driver from contrib/ to core
- Box : Code cleanup, removed some lingering unnecessary references to IReader.h.
- Box : Moved some plugins from Samples/ category to more reasonable places. No list was compiled.
- Box : Removed a few more obsolete boxes
- Box : Changed output channel names of some boxes to be be more distinct / Boxes affected : FastICA, Noise Generator, Sinus Oscillator, Spatial Filter
- Box : Refactored CSV writer to use the time arithmetics class for conversions
- Box : Separated the Tests and Examples category into two
- Box : Removed some references to unnecessary headers.
- Box : Factorize parameter management.
- Box : Avoid compilation of itpp code if itpp package is there but not at the right place.
- Box : Clean + remove old bliff classifier.
- Box : Remove itpp from dependencies of classification.
- Box : Moved Clock Stimulator box to the Stimulation category
- Box : Renamed obsolete references to SignalProcessingGpl namespace
- Doc : Fixed generating the ERP Plot doc
- Dev : All graphical assets (images & 3D models) in OpenViBE should now have acceptable licenses
- Dev : Added low-order numeric stimuli (0-32, suitable for 1 byte communications using parallel port).
- Dev : EBML streams now follow the EBML specification with a proper header and DTD
- Dev : Changed MicroMed dll search to look for VS2010 based lib
- Dev : Allowed compilation without GTK.
- Dev : All boxes now use the encoder/decoder approach
- Dev : Lots of unused code removed; Automaton, Callbacks, various boxes
- Dev : Renamed samples/ plugin path to data-generation/
- Dev : Linux multicore build now adapts to the number of cores
- Dev : Changed Windows build to use multiple cores
- Dev : Differentiated between vs100 and vs90 lua
- Dev : dropped Presage dependency on VS2008.
- Dev : Improved LUA detection on Fedora20
- Dev : Minor tweak to ITPP detection script
- Dev : Updated VRPN dependency to 7.31
- Dev : Made Ogre 3D optional (3D plugins not usable)
- Dev : Made GTK optional (i.e. builds what is possible the build without GTK)
- Dev : Fix ogre package install on ubuntu 14.10.
- Dev : Removed dependency on VS2005 and VS2008 redists
- Dev : Removed SettingCollectionHelper.
- Dev : Reduced C11 dependencies
- Dev : Renamed system/ modules to avoid conflicts with system files e.g. Time.h -> ovCTime.h etc.
- Dev : Removed unused modules Automation and Stream
- Dev : After move to codecs, removed code of deprecated callbacks

### Fixed

- Drivers : Impedance check jamming the gUSBAmp
- Drivers : BrainVision Recorder driver / Fixed crash with multiple markers in a single message, Fixed memory leaks
- Drivers : BioSemi ActiveTwo driver
- Drivers : Minor fix to MBT Smarting driver
- Drivers : Acquisition Server crash with gtec gipsa driver
- Drivers : MCS NVX driver patch for situations with invalid signal values
- Drivers : handling of non-numeric markers in BrainVision Recorder driver. Additionally, fixed some of the repeating memory reallocations in the driver.
- Drivers : fixed stop crash in external stimulations occurring with Refa32B
- Server : Issue with too many stimuli sent in case of NaNs in the data
  - Fixed time placement of Correct/Incorrect stimuli in cases of NaNs
- Designer : Issue with restoring nonsaved state
- Designer : Tab reordering issue.
- Designer : Restore the previous state of the log expander.
- Designer : Prevent any plugin windows to be displayed if no gui is active.
- Designer : Prevent from display the popup of warning in intialization mechanism if no-gui is active.
- Designer : fix for drag&drop scenario file on windows
- Designer : fix memory leak when getting Enumeration setting value (MANTIS issue #158)
- Designer : fix display of boxes with name containing &
- Box : Time Based Epoching no longer crashes if samplingrate is 0
- Box : Crash in Temporal Filter box with too small chunk sizes
- Box : Canvas space usage in Signal Display with Chn Selector
- Box : Signal Display now only draws the channels that are visible
- Box : Signal Display now detects if the signal sampling rate does not match chunk size/time properties
- Box : CSV file-io boxes
- Box : Fractional band limits in Spectral Analysis
- Box : FFT/IFFT boxes to work with odd chunk sizes
- Box : CSV Reader and Writer boxes will now handle Feature Vectors ok
- Box : Crashes in Stream Switch
- Box : Bug in classification (pairwise strategy selection for OneVsOne, SVM output, Voting)
- Box : More boxes now properly restrict streams to the types they can support
- Box : Various bugfixes
  - Fixed old filename used in signal concatenation example
  - Added FFT decomposition tutorial scenario
  - Fixed unset sampling rate in Windowing Functions box
  - Code cleanup of IFFT box
  - Fixed keyboard stimulator from repeating the stimuli
  - Added Error checking to Spectral Analysis + code cleanup
  - Added input type restrictions to TCP Writer box
- Box : Crash in signal concatenation
- Box : Removed CoSpectra box which did not seem to do anything
- Box : Added check for zero sampling rate to Time Based Epoching box
- Box : Signal Merger box not outputting the sampling rate
- Box : SignalDisplay : Remove redraw of all channel when stimulation is received. Was slowing down the box
- Box : Topographic map display (mantis 97)
- Box : TCPWriter export of undefined stimuli
- Dev : Misc tweaks & code cleanup
- App : Skeleton generator.
- App : issues with Skeleton Generator and spaces in paths
- App : handling of spaces in Windows command arguments
- Dev : kernel/Scheduler, Chunks accumulated at the inputs of muted boxes and were never cleared causing an increase in memory use over time
- Dev : mantis 120. The same change may have to be introduced in other boxes as well
- Dependency : GLFW3 dependency archive requiring Visual Studio debug dll

## [0.18.0] - 2014-07-28 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=9323))

### Added

- Drivers : BioSemi ActiveTwo
- Drivers : Cognionics
- Drivers : TMSi (universal driver)
- Drivers : NeuroElectrics Enobio3G
- Box : Multiclass classification support with three pairing strategies, native, 1-vs-all and 1-vs-1.

### Changed

- Designer : Lots of usability improvements to Designer, including rendering speed
- Designer : Improved signal display, all signals can now be drawn to the same drawing area as is usually done with EEG data
- Box : Advanced P300 'CoAdapt' speller with new keyboard, new flashing strategies, improved timing, optional word prediction, and more
- Box : Support to allow box parameters to be changed during play (for boxes that explicitly declare it)

## [0.17.0] - 2013-12-19 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=9228))

### Added

- Box : New box messaging capabilities. Boxes can be designed to send instant messages to one another.
- Box : Hilbert Transform
- Box : Connectivity Features with Single Trial Phase Locking Value algorithm
- Box : TCP Writer
- Box : Shrinkage LDA
- App : New SSVEP game Mind Shooter

### Changed

- Usability improvements : for example, automatic saving and loading of driver settings in the Acquisition Server

### Fixed

- Dev : Bug fixes made. Details in release announcement.

## [0.16.0] - 2013-07-01 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=5049))

### Added

- Drivers : NeuroSky Mindwave Mobile
- Box : AutoRegressive Coefficients
- Box : Timeout
- Box : Stimulus Voter
- Dev : New build system with full Microsoft Visual Studio IDE support
- Dev : New source code directory organization that is easier to grasp for new developers.
- Dev : Source distribution includes files to build a basic Debian/Ubuntu/Mint binary package

### Changed

- Drivers : Upgraded Gtec driver from gipsa.lab
- Dev : License has been changed to AGPLv3.

### Fixed

- Dev : Bug fixes made. Details in release announcement.

## [0.15.0] - 2013-02-01 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=4887))

### Added

- Drivers : BrainProducts ActiCHamp
- Drivers : BrainMaster Atlantis and Discovery
- Server : Send stimulations from your application to Acquisition Server
- Designer : Self-contained scenario configuration. The $__volatile_ScenarioDir token will always expand to the path of the folder of the current scenario.

## [0.14.0] - 2012-06-18 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=1033))

### Added

- Box : Python scripting
- Drivers : Fieldtrip server
- Drivers : Mitsar EEG device
- Server : Pssibility to use the USBamp’s event channel in the acquisition driver.

### Changed

- Box : The matlab box was re-written and now supports any number of inputs and outputs of any type.
- Dependency : Boost on Linux was updated to 1.49, there is an added dependency to pthreads on Windows.

## [0.13.0] - 2012-01-30 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=695))

### Added

- Designer : The designer now has a search feature which enables quick lookup of available boxes.
- Box : EDF File Writer.
- Box : BCI2000 File Reader.
- Box : A box for calculating differential and integral of any order was added. (The first and second difference detrending boxes are now deprecated)

### Changed

- Dependency : Updated Gtk on Windows (2.22.1)
- Dependency : Used IT++ version 4.0.7 across all platforms

## [0.12.0] - 2011-10-12 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=578))

### Added

- Designer : The time is now printed in every logs (console, file, etc.) in seconds. User can switch between this display and the previous (in 32:32 time and/or hexadecimal) at will using configuration tokens. The time precision can also be set.
- Drivers : The Emotiv EPOC driver is now included in the Windows Installer. It still requires the user to specify where to find the Emotiv Research Edition SDK in order to run correctly.
- Drivers : The g.Tec g.USBAmp driver benefits from a major update that add more amplifier functionalities : ability to connect to common ground and reference, activation of notch and band-pass filters.
- Box : Stream synchronization box, that can synchronize in OpenViBE several streams coming from several linked devices.
- Box : The Matlab Filter box is now compiled by default and included in the release. This box is still unstable, but has been fully reworked in order to work on every machine, as long as Matlab (version 32bits) is installed.

### Fixed

- Dev : Bug fixes made. Details in release announcement.

## [0.11.0] - 2011-08-11 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=532))

### Added

- Drivers : Driver based on OpenAL to acquire sound signal from microphone input.
- Support : OpenViBE is now officially supported on Fedora 15, Ubuntu 11.04 and Windows 7 64bit.
- Box : New Reference Channel and Channel Selector boxes. The older boxes are now deprecated. The channel selection is smarter (name or 1-based index, 'X:Y' for channel range).
- Box : New Signal Concatenation box. The older box is now deprecated. The concatenation is much faster, and end-of-file is detected automatically through a time-out.
- Box : New Stream Switch box that copies its input chunks on one of its outputs. The active output is selected with defined stimulations.
- Doc : Documentation for the Matlab Filter box.
- Doc : Documentation for the Display-Cue visualisation box.
- Scenarios : New set of scenarios for a Motor-Imagery based BCI using CSP spatial filters, and demonstrating how to measure classifier performances offline or online (confusion matrix, overall performance).

### Changed

- Drivers : The Emotiv EPOC driver now acquires values from gyroscope sensors (new option in the driver properties).
- Designer : The Player in the OpenViBE Designer can now be controlled with keyboard shortcuts (F5 : stop, F6 : one-step, F7 : play, F8 : fast-forward)
- Dev : Skeleton Generator benefited from a major update, and can produce box skeleton with Listeners, Codecs, Flags, etc.
- Box : CSV file writer can receive 1 or 2-dimensional matrices
- Box : Matrix display can receive 1 or 2-dimensional matrices
- Box : The Matlab filter box has now a Stimulation output, and the Matlab messages are redirected to the OpenViBE console (warnings, errors, disp, etc.)
- Box : Classifier Trainer outputs standard deviation along with classification percentage on the k-fold test. k-fold test now randomize the input data prior to testing by default.
- Box : CSP and xDAWN spatial filter trainers output a stimulation OVTK_StimulationId_TrainCompleted after a successful training.
- Box : Classifier Accuracy visualization box can now show score and/or percentages, or none of them.
- Doc : Acquisition Server documentation updated to latest GUI modification (division driver properties/server settings)
- Scenarios : SSVEP scenarios now make use of CSP spatial filters for better performances.

### Fixed

- Box : patch for the Matlab filter box, by fixing bugs and adding stimulation output.
- Box : patches for the skeleton-generator and the matlab box

## [0.10.1] - 2011-04-13 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=462))

### Fixed

- Windows installer : A small glitch during the creation of the installer made Brain Products First-Amp and V-Amp drivers unavailable.

## [0.10.0] - 2011-04-12 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=460))

### Added

- Server : We added new drivers for raw file reading and telnet raw reading to the acquisition server
- Server : You can now choose whether to activate, deactivate or force the drift correction in the acquisition server
- Designer : The designer can now read scenarios from standard input
- Designer : A collapsible console showing the output messages has been added to the designer
- Designer : An optional pop-up window is now displayed when there are errors during the execution of a scenario
- Designer : Multiple files can now be opened at once in the designer
- Designer : The designer now supports several new keyboard shortcuts
  - F2 - rename all selected boxes
  - Ctrl+A - select all boxes in the current scenario
  - Ctrl+W - close the currently opened scenario
- Drivers : We added support for triggers in the TMSiRefa32B and MicromedSystemPlusEvolution drivers
- Box : Box for training spatial filters using CSP algorithm
- Box : Box which can display user selected images upon receiving stimulations
- Box : A new VRPN Client box is available so OpenViBE can receive messages from external applications
- Box : A new, unstable, CSV Reader box is available
- Box : Sound stimulation, based on OpenAL, is available. The old SoundStimulation box is now deprecated.
- Box : Moving average box can now use two new methods - Cumulative Average and Immediate Moving Average
- Dev : Added templates for easy encoder/decoder creation and manipulation
- Scenarios : We added scenarios for SSVEP BCIs along with an application for SSVEP stimulation

### Changed

- Drivers : Neurosky Mindset driver was updated to use SDK v2.1
- Drivers : Neurosky Mindset driver now supports blink detection
- Box : Spatial filter can now work with signals, spectrums and streamed matrices
- Box : Lua stimulator now has access to the log manager

### Fixed

- Dev : Bug fixes made. Details in release announcement.

## [0.9.0] - 2010-12-28 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=416))

### Added

- Drivers : EGI Netamps driver
- Drivers : Multi amp support for Brain Products BrainAmp series
- Box : VRPN Button client box

### Changed

- Server : The acquisition server now remembers the last device you used
- Designer : The designer now remembers the last opened scenarios
- Designer : The designer can be launched from command line hiding the GUI, loading/runing a specific scenario etc..
- App : Each box setting can now be configured with the configuration manager

### Fixed

- Drivers : Stabilized the Brain Products BrainVision Recorder driver (the driver is now stable)
- Box : Stabilized lua box with a more complete API (the box is now stable)

## [0.8.0] - 2010-10-01 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=378))

### Added

- Server : Multithreaded the acquisition server for better performance and synchronization
- Server : Significantly enhanced drift management in the acquisition server
- Drivers : Implemented impedance check on the TMSi Refa driver
- Drivers : Emotiv EPOC
- Drivers : Brainamp Series driver using Brain Products low level API
- Box : Box to join multiple signal streams in a single stream
- Box : Box to compute the power of a spectrum frequency band
- Box : Box to rename signal channels
- Box : SVM classifier (based on libsvm)
- Dev : Driver seketon generator for driver developers

### Changed

- Doc : Updated the online documentation and tutorials
- Doc : Updated many documentation pages
- Dependency : Updated windows dependencies to it++ 4.0.7, vrpn 7.26, boost 1.42, cmake 2.8.2, gtk 2.16.6
- Dependency : Moved from libglade to gtk_builder allowing upgrading GTK
- Drivers : Updated the ModularEEG driver
- Drivers : Updated the Brain Products VAmp driver to support FirstAmp8
- Box : Updated the temporal filter box to correct errors - should be very close to MATLAB now
- Box : Updated the Sound Player box to make it work on Windows
- App : Restored the handball VR application for interaction from the motor imagery BCI

### Fixed

- Dev : Bug fixes made. Details in release announcement.

## [0.7.0] - 2010-07-08 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=336))

### Added

- Drivers : Neurosky Mindset
- Box : Univariate Statistics box
- Box : Some control over the player execution from the boxes. Details in release announcement.
- Box : Box that is able to control the player execution
- Box : Lot of great features to the Simple DSP box - see documentation and sample scenarios
- Designer : Possibility to comment scenarios in the designer
- Designer : More configuration possibilities with the configuration file tokens
- Designer : Many small "user oriented" features in the designer such as left/right scroll with mouse wheel and better overview of renamed boxes

### Changed

- Doc : Updated the online documentation and tutorials
- Doc : Updated many documentation pages
- Server : The acquisition server is now smarter on CPU use
- Designer : Alphabetically sorted the stimulations for quicker configuration of the boxes
- Designer : Restored automatic filename extension/filtering when loading/saving a scenario
- Scenarios : Reviewed and commented all the box-tutorials and xDAWN p300 speller / motor imagery scenarios

### Removed

- Designer : Removed some dead code. No list was compiled.

### Fixed

- Dev : Bug fixes made. Details in release announcement.
- Box : Corrected a bug on the k-fold test in the classifier trainer

## [0.6.1] - 2010-05-20 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=322))

### Fixed

- Windows installer : Restores the TMSi driver (some files were missing)
- Windows installer : Restores the Micromed driver (some files were missing)
- Windows installer : Translates the Tie-Fighter demo in english
- Windows installer : Adds a scenario in share/openvibe-scenario/bci/tie-fighter : tie-fighter-freetime.xml, to use in combination with the vr-demo application.

## [0.6.0] - 2010-04-14 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=306))

### Added

- Drivers : g.Tec's gMobiLab+ acquisition device for Linux
- Drivers : Micromed SD LTM (through SystemPlus Evolution)
- Box : Confusion Matrix computation box
- Box : Matrix Display box
- Box : Lua stimulation box
- Server : Impedance check to the drivers that could support it
- Scenarios : New sample scenarios

### Changed

- Dependency : Moved to Ogre 1.7 for 3D display
- Dependency : Moved to Boost 1.41
- Doc : Updated the online documentation and tutorials
- Doc : Updated many documentation pages
- Server : Refactored the acquisition server so the server itself runs in a separate thread for better performances
- Server : Corrected bugs on the acquisition server that caused desynchronisations
- App : Reenabled VR demos as default
- App : Enhanced the visuals of the VR demos and added a few stats to make the experience more entertaining

### Removed

- Dependency : We removed OpenMASK / OBT dependency

### Fixed

- Dev : Bug fixes made. Details in release announcement.

## [0.5.0] - 2009-12-18 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=235))

### Added

- Drivers : brain products vAmp acquisition device
- Drivers : g.Tec's gUSBamp acquisition device
- App : Online comparison of different processing pipelines performance (e.g. multiple classifiers)
- Scenarios : P300-based pipeline based on the xDAWN algorithm
- Scenarios : New sample scenarios

### Changed

- Dependency : Updated the dependencies installation for windows so that Visual C++ Runtime gets included in the package
- Dependency : Updated the dependencies installation for windows so that the lack of internet connection does not abort the installation when DirectX is not present
- Doc : Updated the online documentation and tutorials

### Fixed

- Dev : Bug fixes made. Details in release announcement.

## [0.4.0] - 2009-10-23 ([release announcement](http://openvibe.inria.fr/forum/viewtopic.php?f=1&t=148))

### Added

- Drivers : OpenEEG Modular EEG / Monolith EEG
- Drivers : g.Tec's gUSBamp acquisition device
- Box : frequency band selector
- Box : signal decimation
- Box : CSV file writer (text based)
- Box : K-fold test in the classifier trainer box
- Box : P300-based entertaining application called "Magic Card"
- Dev : Several tooltips for new users
- Server : Functionnality to load/save channel names in the acquisition server
- Scenarios : New sample scenarios

### Changed

- Dependency : Updated the dependencies installation script for linux so that it uses native packages instead of compiling everything from scratch
- Dependency : Updated the dependencies installation for windows so that DirectX and Visual C++ Runtime gets installed automatically if needed
- Doc : Updated the online documentation and tutorials
- Box : Enabled the voting classifier box to vote either on streamed matrix or on stimulations
- Box : Changed the way chanels can be selected in the signal display, power spectrum display and time frequency map display
- Box : Reimplemented the common average reference box
- Scenarios : Stabilized P300-based pipeline
- Dev : Made the development of new classifiers easier thanks to base algorithms

### Removed

- App : The VR demo are no more built by default as OpenMASK is not compiling on recent Linux distributions

### Fixed

- Dev : Bug fixes made. Details in release announcement.
