#pragma once
/*
#include "openeeg-modulareeg/src/ovasCDriverOpenEEGModularEEG.h"
#include "field-trip-protocol/src/ovasCDriverFieldtrip.h"
#include "brainproducts-brainvisionrecorder/src/ovasCDriverBrainProductsBrainVisionRecorder.h"
*/
#include "ovasCPluginExternalStimulations.h"
#include "ovasCPluginTCPTagging.h"

#include "ovasCDriverBrainmasterDiscovery.h"
#include "ovasCDriverBrainProductsBrainVisionRecorder.h"
#include "ovasCDriverCognionics.h"
#include "ovasCDriverCtfVsmMeg.h"
#include "ovasCDriverEncephalan.h"
#include "CDriverGTecGUSBamp.hpp"
#include "ovasCDriverGTecGUSBampLegacy.h"
#include "ovasCDriverGTecGUSBampLinux.h"
#include "CDriverGTecUnicorn.hpp"
#include "ovasCDriverGTecGMobiLabPlus.h"
#include "ovasCDrivergNautilusInterface.h"
#include "ovasCDriverMBTSmarting.h"
#include "ovasCDriverMitsarEEG202A.h"
#include "ovasCDriverOpenALAudioCapture.h"
#include "ovasCDriverOpenEEGModularEEG.h"
#include "ovasCDriverOpenBCI.h"
#include "ovasCDriverEEGO.h"

#if !defined(WIN32) || defined(TARGET_PLATFORM_i386)
#include "ovasCDriverFieldtrip.h"
#endif

namespace OpenViBE {
namespace Contributions {
inline void InitiateContributions(AcquisitionServer::CAcquisitionServerGUI* pGUI, AcquisitionServer::CAcquisitionServer* pAS,
								  const Kernel::IKernelContext& context, std::vector<AcquisitionServer::IDriver*>* drivers)
{
	//No Limitations
	drivers->push_back(new AcquisitionServer::CDriverBrainProductsBrainVisionRecorder(pAS->getDriverContext()));
	drivers->push_back(new AcquisitionServer::CDriverCtfVsmMeg(pAS->getDriverContext()));
	drivers->push_back(new AcquisitionServer::CDriverMBTSmarting(pAS->getDriverContext()));
	drivers->push_back(new AcquisitionServer::CDriverOpenEEGModularEEG(pAS->getDriverContext()));
	drivers->push_back(new AcquisitionServer::CDriverOpenBCI(pAS->getDriverContext()));

	//OS Limitations
#if defined WIN32
	drivers->push_back(new AcquisitionServer::CDriverCognionics(pAS->getDriverContext()));
#endif
#if defined TARGET_OS_Windows
	drivers->push_back(new AcquisitionServer::CDriverEncephalan(pAS->getDriverContext()));
#endif
#if defined TARGET_HAS_PThread
#if !defined(WIN32) || defined(TARGET_PLATFORM_i386)
		drivers->push_back(new AcquisitionServer::CDriverFieldtrip(pAS->getDriverContext()));
#endif
#endif
	//Commercial Limitations
#if defined TARGET_HAS_ThirdPartyGUSBampCAPI
	drivers->push_back(new AcquisitionServer::CDriverGTecGUSBamp(pAS->getDriverContext()));
	drivers->push_back(new AcquisitionServer::CDriverGTecGUSBampLegacy(pAS->getDriverContext()));
#endif
#if defined TARGET_HAS_ThirdPartyGUSBampCAPI_Linux
	drivers->push_back(new AcquisitionServer::CDriverGTecGUSBampLinux(pAS->getDriverContext()));
#endif
#if defined TARGET_HAS_ThirdPartyGtecUnicron
	drivers->push_back(new AcquisitionServer::CDriverGTecUnicorn(pAS->getDriverContext()));
#endif
#if defined TARGET_HAS_ThirdPartyGMobiLabPlusAPI
	drivers->push_back(new AcquisitionServer::CDriverGTecGMobiLabPlus(pAS->getDriverContext()));
#endif
#if defined TARGET_HAS_ThirdPartyGNEEDaccessAPI
	drivers->push_back(new AcquisitionServer::CDrivergNautilusInterface(pAS->getDriverContext()));
#endif
#if defined TARGET_HAS_ThirdPartyBrainmasterCodeMakerAPI
	drivers->push_back(new AcquisitionServer::CDriverBrainmasterDiscovery(pAS->getDriverContext()));
#endif
#if defined(TARGET_HAS_ThirdPartyMitsar)
	drivers->push_back(new AcquisitionServer::CDriverMitsarEEG202A(pAS->getDriverContext()));
#endif
#if defined TARGET_HAS_ThirdPartyOpenAL
	drivers->push_back(new AcquisitionServer::CDriverOpenALAudioCapture(pAS->getDriverContext()));
#endif
#if defined(TARGET_HAS_ThirdPartyEEGOAPI)
	drivers->push_back(new AcquisitionServer::CDriverEEGO(pAS->getDriverContext()));
#endif

	pGUI->registerPlugin(new AcquisitionServer::Plugins::CPluginExternalStimulations(context));
	pGUI->registerPlugin(new AcquisitionServer::Plugins::CPluginTCPTagging(context));
}
}  // namespace Contributions
}  // namespace OpenViBE
