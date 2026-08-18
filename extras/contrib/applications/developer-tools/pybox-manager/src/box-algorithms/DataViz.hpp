///-------------------------------------------------------------------------------------------------
/// 
/// \file DataViz.hpp
/// \brief Class NewBoxPattern
/// \author Thibaut Monseigne (Inria) & Jimmy Leblanc (Polymont) & Yannis Bendi-Ouis (Polymont) 
/// \version 1.0.
/// \date 12/03/2020.
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

#include "CPolyBox.hpp"

#if !(defined(WIN32) && defined(TARGET_BUILDTYPE_Debug))

namespace OpenViBE {
namespace Plugins {
namespace PyBox {
class CBoxAlgorithmDataViz final : public CPolyBox
{
public:
    CBoxAlgorithmDataViz() { m_script = Directories::getDataDir() + "/plugins/python3/pybox/DataViz.py"; }
    _IsDerivedFromClass_Final_(OpenViBE::Toolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_DataViz)
};

class CBoxAlgorithmDataVizListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setInputType(index, OV_TypeId_StreamedMatrix);
        return true;
    }

    _IsDerivedFromClass_Final_(OpenViBE::Toolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, CIdentifier::undefined())
};

class CBoxAlgorithmDataVizDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "DataViz"; }
    CString getAuthorName() const override { return "Yannis Bendi-Ouis & Jimmy Leblanc"; }
    CString getAuthorCompanyName() const override { return "Polymont IT Services"; }
    CString getShortDescription() const override { return "Transform the data with a LDA or a PCA and plot the data in 2D or 3D."; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Scripting/PyBox"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-convert"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_DataViz; }
    IPluginObject* create() override { return new CBoxAlgorithmDataViz; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmDataVizListener; }
    void releaseBoxListener(IBoxListener* pBoxListener) const override { delete pBoxListener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addSetting("Clock frequency (Hz)", OV_TypeId_Integer, "64");
        // <tag> settings
        prototype.addSetting("Path to save the model", OV_TypeId_Filename, "");
        prototype.addSetting("Path to load the model", OV_TypeId_Filename, "");
        prototype.addSetting("Algorithm (PCA or LDA)", OV_TypeId_String, "PCA");
        prototype.addSetting("Dimension reduction", OV_TypeId_Integer, "2");
        prototype.addSetting("Labels", OV_TypeId_String, "");

        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        prototype.addFlag(Kernel::BoxFlag_CanAddSetting);
        prototype.addFlag(Kernel::BoxFlag_CanModifySetting);

        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addInputSupport(OV_TypeId_Stimulations);
        prototype.addInputSupport(OV_TypeId_StreamedMatrix);

        prototype.addOutputSupport(OV_TypeId_Signal);
        prototype.addOutputSupport(OV_TypeId_Stimulations);
        prototype.addOutputSupport(OV_TypeId_StreamedMatrix);

        // <tag> input & output
        prototype.addOutput("stim_out", OV_TypeId_Stimulations);
        prototype.addInput("input_StreamMatrix", OV_TypeId_StreamedMatrix);
        prototype.addInput("input_Stimulations", OV_TypeId_Stimulations);

        return true;
    }

    _IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_DataVizDesc)
};
}  // namespace PyBox
}  // namespace Plugins
}  // namespace OpenViBE

#endif // !(defined(WIN32) && defined(TARGET_BUILDTYPE_Debug))
