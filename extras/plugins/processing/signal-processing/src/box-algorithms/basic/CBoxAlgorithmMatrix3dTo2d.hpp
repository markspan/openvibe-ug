///-------------------------------------------------------------------------------------------------
///
/// \file CBoxAlgorithmMatrix3dTo2d.hpp
/// \brief Classes of the box Matrix3dTo2d
/// \author Arthur DESBOIS (INRIA).
/// \version 0.0.1.
/// \date Fri Feb 12 15:13:00 2021.
///
/// \copyright Copyright (C) 2021 - 2022 INRIA
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
///-------------------------------------------------------------------------------------------------

#pragma once

#include "defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

enum class ESelectionMode
{
    Select,
    Average
};

/// \brief  Convert selection method to string.
/// \param mode The selection method.
/// \return The selection method as human readable string
inline std::string selectionModeToString(const ESelectionMode mode)
{
    switch (mode)
    {
    case ESelectionMode::Select:
        return "Select";
    case ESelectionMode::Average:
        return "Average";
    default:
        return "Undefined";
    }
}

/// \brief  Convert string to Selection Method.
/// \param mode The selection method as a string
/// \return \ref ESelectionMode
inline ESelectionMode stringToSelectionMode(const std::string& mode)
{
    if (mode == "Select") {
        return ESelectionMode::Select;
    }
    if (mode == "Average") {
        return ESelectionMode::Average;
    }

    return ESelectionMode::Select; // default
}


/// \brief The class CBoxAlgorithmMatrix3dTo2d describes the box Matrix 3D to 2D.
class CBoxAlgorithmMatrix3dTo2d final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_Matrix3dTo2d)

protected:
    // Codecs
    Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmMatrix3dTo2d> m_matrixDecoder;
    Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmMatrix3dTo2d> m_matrixEncoder;

    // Matrices
    CMatrix* m_iMatrix = nullptr;
    CMatrix* m_oMatrix = nullptr;

    // Parameters
    ESelectionMode m_selectMode = ESelectionMode::Select; 
    size_t m_dimensionToWorkUpon = 0;
    size_t m_selectedIdx = 0;
    

    size_t m_dim0Size = 0;
    size_t m_dim1Size = 0;
    size_t m_dim2Size = 0;

private:
    bool selectIdxInDimension(const CMatrix& in, CMatrix& out) const;
    bool averageDimension(const CMatrix& in, CMatrix& out) const;
};

///
/// \brief Listener class for the box Matrix 3D to 2D.
///
class CBoxAlgorithmMatrix3dto2dListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:

    bool onSettingValueChanged(Kernel::IBox& box, const size_t index) override
    {
        CString name;
        CString value;
        box.getSettingName(index, name);
        box.getSettingValue(index, value);
        if (name == CString("Selection mode")) {
            updateSettingSelectionMode(box, value);
        }
        return true;
    };

private:
    static const int NUM_COMMON_SETTINGS = 2;

    void updateSettingSelectionMode(Kernel::IBox& box, const CString& mode)
    {
        if (mode == CString("Select")) {
            while (box.getSettingCount() > NUM_COMMON_SETTINGS) {
                box.removeSetting(NUM_COMMON_SETTINGS);
            }
            box.addSetting("Index in dimension", OV_TypeId_Integer, "0");
        }
        else if (mode == CString("Average")) {
            while (box.getSettingCount() > NUM_COMMON_SETTINGS) {
                box.removeSetting(NUM_COMMON_SETTINGS);
            }
        }
        else {
            this->getLogManager() << Kernel::LogLevel_Error << "CBoxAlgorithmMatrix3dTo2dListener::updateSettings: Unrecognized mode: " << mode << "\n";
        }
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

/// \brief Descriptor of the box Matrix 3D to 2D.
class CBoxAlgorithmMatrix3dTo2dDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override {}

    CString getName() const override { return "Matrix 3D to 2D"; }
    CString getAuthorName() const override { return "Arthur Desbois"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Convert 3D matrices to 2D"; }
    CString getDetailedDescription() const override { return "Convert 3 dimensional matrices to 2D matrices, by selecting a dimension to remove"; }
    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "0.0.1"; }
    CString getStockItemName() const override { return ""; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_Matrix3dTo2d; }
    IPluginObject* create() override { return new CBoxAlgorithmMatrix3dTo2d; }

    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmMatrix3dto2dListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("input", OV_TypeId_StreamedMatrix);
        prototype.addOutput("output", OV_TypeId_StreamedMatrix);

        prototype.addSetting("Selection mode", OV_TypeId_Matrix3dTo2d_SelectionMethod, selectionModeToString(ESelectionMode::Select).c_str());
        prototype.addSetting("Dimension to work on", OV_TypeId_Integer, "0");       

        // Default mode : select index in dimension
        prototype.addSetting("Index in dimension", OV_TypeId_Integer, "0");

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_Matrix3dTo2dDesc)
};

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
