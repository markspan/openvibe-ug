///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmDLLBridge.hpp
/// \author Jussi T. Lindgren (Inria)
/// \version 0.2.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include "../defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <string>
#include <queue>

#if defined(TARGET_OS_Windows)
#include <windows.h>
#endif

namespace OpenViBE {
namespace Plugins {
namespace DLLBridge {
class CDLLBridge final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;

    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_DLLBridge)

private:
    Toolkit::TDecoder<CDLLBridge>* m_decoder = nullptr;
    Toolkit::TEncoder<CDLLBridge>* m_encoder = nullptr;

    CIdentifier m_inputTypeID = CIdentifier::undefined();
    CString m_dllFile;
    CString m_parameters;

    // These functions are expected from the DLL library
    // @note the inputs are non-const on purpose to ensure maximum compatibility with non-C++ dlls
    typedef void (* INITFUNC)(int* paramsLength, const char* params, int* errorCode);
    typedef void (* UNINITFUNC)(int* errorCode);
    typedef void (* PROCESSHEADERFUNC)(int* rowsIn, int* colsIn, int* samplingRateIn, int* rowsOut, int* colsOut, int* samplingRateOut,
                                       int* errorCode);
    typedef void (* PROCESSFUNC)(double* matIn, double* matOut, int* errorCode);

    INITFUNC m_initialize             = nullptr;
    UNINITFUNC m_uninitialize         = nullptr;
    PROCESSHEADERFUNC m_processHeader = nullptr;
    PROCESSFUNC m_process             = nullptr;

#if defined(TARGET_OS_Windows)
    HINSTANCE m_library = nullptr;
#elif defined(TARGET_OS_Linux)
    void* m_library = nullptr;
#elif defined(TARGET_OS_MacOS)
  void* m_library = nullptr;
#endif
};

class CDLLBridgeListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(index, typeID);
        box.setOutputType(0, typeID);
        return true;
    }

    bool onOutputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getOutputType(index, typeID);
        box.setInputType(0, typeID);
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

/**
* Plugin's description
*/
class CDLLBridgeDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "DLL Bridge"; }
    CString getAuthorName() const override { return "Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Process a signal or matrix stream with a DLL"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Scripting"; }
    CString getVersion() const override { return "0.2"; }
    CString getStockItemName() const override { return "gtk-convert"; }

    CIdentifier getCreatedClass() const override { return Box_DLLBridge; }
    IPluginObject* create() override { return new CDLLBridge(); }
    IBoxListener* createBoxListener() const override { return new CDLLBridgeListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addSetting("DLL file", OV_TypeId_Filename, "");
        prototype.addSetting("Parameters", OV_TypeId_String, "");

        prototype.addInput("Input",OV_TypeId_Signal);
        prototype.addOutput("Output",OV_TypeId_Signal);

        prototype.addInputSupport(OV_TypeId_StreamedMatrix);
        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addOutputSupport(OV_TypeId_StreamedMatrix);
        prototype.addOutputSupport(OV_TypeId_Signal);

        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);

        prototype.addFlag(Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_DLLBridgeDesc)
};
}  // namespace DLLBridge
}  // namespace Plugins
}  // namespace OpenViBE
