///-------------------------------------------------------------------------------------------------
///
/// \file CBoxAlgorithmMatrix2dToVector.hpp
/// \brief Classes of the box Matrix2dToVector
/// \author Arthur DESBOIS (INRIA).
/// \version 0.0.1.
/// \date June 28 15:13:00 2022.
///
/// \copyright Copyright (C) 2022 INRIA
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
#include "CBoxAlgorithmMatrix3dTo2d.hpp" // to get ESelectionMode

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

/// \brief The class CBoxAlgorithmMatrix2dToVector describes the box Matrix 3D to 2D.
class CBoxAlgorithmMatrix2dToVector final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_Matrix2dToVector)

protected:
    // Codecs
    Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmMatrix2dToVector> m_matrixDecoder;
    Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmMatrix2dToVector> m_matrixEncoder;

    // Matrices
    CMatrix* m_iMatrix = nullptr;
    CMatrix* m_oMatrix = nullptr;

    // Parameters
    ESelectionMode m_selectMode = ESelectionMode::Select;
    size_t m_dimensionToWorkUpon = 0;
    size_t m_selectedIdx = 0;
    
    size_t m_dim0Size = 0;
    size_t m_dim1Size = 0;

private:
    bool selectIdxInDimension(const CMatrix& in, CMatrix& out) const;
    bool averageDimension(const CMatrix& in, CMatrix& out) const;
};

///
/// \brief Listener class for the box Matrix 2D to Vector.
///
class CBoxAlgorithmMatrix2dToVectorListener final : public Toolkit::TBoxListener<IBoxListener>
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
            this->getLogManager() << Kernel::LogLevel_Error << "CBoxAlgorithmMatrix2dToVectorListener::updateSettings: Unrecognized mode: " << mode << "\n";
        }
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

/// \brief Descriptor of the box Matrix 2D to Vector.
class CBoxAlgorithmMatrix2dToVectorDesc final : virtual public IBoxAlgorithmDesc
{
public:

    void release() override {}

    CString getName() const override { return "Matrix 2D to Vector"; }
    CString getAuthorName() const override { return "Arthur Desbois"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Extract 1D Vector from 2D Matrix"; }
    CString getDetailedDescription() const override { return "Extract vector from a 2D Matrix by selecting an index and row/column to pick from."; }
    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "0.0.1"; }
    CString getStockItemName() const override { return ""; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_Matrix2dToVector; }
    IPluginObject* create() override { return new CBoxAlgorithmMatrix2dToVector; }

    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmMatrix2dToVectorListener; }
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

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_Matrix2dToVectorDesc)
};

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
