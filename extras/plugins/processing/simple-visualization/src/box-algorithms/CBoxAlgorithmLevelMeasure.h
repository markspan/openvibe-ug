///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmLevelMeasure.hpp
/// \brief Classes for the Box Level measure.
/// \author Yann Renard (INRIA/IRISA).
/// \version 1.0.
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

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

class LevelMeasureWindow;

class CBoxAlgorithmLevelMeasure final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_LevelMeasure)

protected:
    Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmLevelMeasure> m_matrixDecoder;
    
    LevelMeasureWindow *m_levelMeasureWindow = nullptr;
};

class CBoxAlgorithmLevelMeasureDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Level measure"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Displays sample chunk of each channel as a row of progress bars"; }
    CString getDetailedDescription() const override { return "Another way to look at it: Each displayed row is a histogram normalized to sum to 1"; }
    CString getCategory() const override { return "Visualization/Basic"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-go-up"; }

    CIdentifier getCreatedClass() const override { return Box_LevelMeasure; }
    IPluginObject* create() override { return new CBoxAlgorithmLevelMeasure; }

    bool hasFunctionality(const EPluginFunctionality functionality) const override { return functionality == EPluginFunctionality::Visualization; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input matrix to display", OV_TypeId_StreamedMatrix);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_LevelMeasureDesc)
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
