#pragma once

#include <memory>

//___________________________________________________________________//
//                                                                   //
// Basic includes                                                    //
//___________________________________________________________________//
//                                                                   //

#include "ov_defines.h"
#include "Directories.hpp"

#include "CIdentifier.hpp"
#include "CTime.hpp"
#include "CNameValuePairList.hpp"
#include "CMatrix.hpp"
#include "CMemoryBuffer.hpp"
#include "CStimulationSet.hpp"

#include "ovCString.h"

#include "ovIObject.h"
#include "ovIObjectVisitor.h"
#include "CKernelLoader.hpp"

#include "ovIObserver.h"
#include "ovCObservable.h"

#include "ovAssert.h"
#include "parameters/parameters.h"

//___________________________________________________________________//
//                                                                   //
// Kernel includes                                                   //
//___________________________________________________________________//
//                                                                   //

#include "kernel/IKernelContext.hpp"
#include "kernel/IKernelDesc.hpp"
#include "kernel/ovIKernelObject.h"
#include "kernel/IKernelObjectFactory.hpp"

#include "kernel/ovITypeManager.h"
#include "kernel/ovIParameter.h"
#include "kernel/ovIConfigurable.h"
#include "kernel/ovIObjectVisitorContext.h"
#include "kernel/ovTParameterHandler.h"

#include "kernel/algorithm/ovIAlgorithmContext.h"
#include "kernel/algorithm/ovIAlgorithmManager.h"
#include "kernel/algorithm/ovIAlgorithmProto.h"
#include "kernel/algorithm/ovIAlgorithmProxy.h"

#include "kernel/configuration/ovIConfigurationManager.h"
#include "kernel/configuration/ovIConfigurationKeywordExpandCallback.h"

#include "kernel/log/ovILogManager.h"
#include "kernel/log/ovILogListener.h"

#include "kernel/error/CError.hpp"
#include "kernel/error/CErrorManager.hpp"
#include "kernel/error/ErrorType.hpp"

#include "kernel/player/ovIBoxAlgorithmContext.h"
#include "kernel/player/CMessageClock.hpp"
#include "kernel/player/ovIPlayer.h"
#include "kernel/player/ovIPlayerContext.h"
#include "kernel/player/ovIPlayerManager.h"

#include "kernel/plugins/ovIPluginManager.h"
#include "kernel/plugins/ovIPluginModule.h"
#include "kernel/plugins/ovIPluginModuleContext.h"

#include "kernel/metabox/ovIMetaboxManager.h"

#include "kernel/scenario/ovIAttributable.h"
#include "kernel/scenario/ovIBox.h"
#include "kernel/scenario/ovIBoxIO.h"
#include "kernel/scenario/ovIBoxListenerContext.h"
#include "kernel/scenario/ovIBoxProto.h"
#include "kernel/scenario/ovIComment.h"
#include "kernel/scenario/ovIMetadata.h"
#include "kernel/scenario/ovILink.h"
#include "kernel/scenario/ovIScenario.h"
#include "kernel/scenario/ovIScenarioManager.h"
#include "kernel/scenario/CScenarioParameterConnection.h"
#include "kernel/scenario/ovIAlgorithmScenarioImporter.h"
#include "kernel/scenario/ovIAlgorithmScenarioExporter.h"

//___________________________________________________________________//
//                                                                   //
// Plugins includes                                                  //
//___________________________________________________________________//
//                                                                   //

#include "plugins/ovIPluginObject.h"
#include "plugins/ovIPluginObjectDesc.h"

#include "plugins/ovIAlgorithm.h"
#include "plugins/ovIAlgorithmDesc.h"

#include "plugins/ovIBoxAlgorithm.h"
#include "plugins/ovIBoxAlgorithmDesc.h"

namespace OpenViBE {
namespace Plugins {
// Backward compatibility
typedef Kernel::IBox IStaticBoxContext;
typedef Kernel::IBoxIO IDynamicBoxContext;
}  // namespace Plugins
}  // namespace OpenViBE

//___________________________________________________________________//
//                                                                   //
// Plugins includes                                                  //
//___________________________________________________________________//
//                                                                   //

#define OVP_Declare_Begin() \
    static std::vector<std::unique_ptr<OpenViBE::Plugins::IPluginObjectDesc>> g_descriptors; \
    extern "C" \
    { \
        OVP_API bool onInitialize(const OpenViBE::Kernel::IPluginModuleContext& context) \
        {

#define OVP_Declare_New(Class) g_descriptors.emplace_back(new Class);

#define OVP_Declare_End() \
            return true; \
        } \
        OVP_API bool onUninitialize(const OpenViBE::Kernel::IPluginModuleContext& context) { return true; } \
        OVP_API bool onGetPluginObjectDescription(const OpenViBE::Kernel::IPluginModuleContext& context, size_t index, OpenViBE::Plugins::IPluginObjectDesc*& pluginObjectDesc) \
        { \
            if(index>=g_descriptors.size()) \
            { \
                pluginObjectDesc=NULL; \
                return false; \
            } \
            pluginObjectDesc=g_descriptors[index].get(); \
            return true; \
        } \
    }
