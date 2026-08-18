///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStatisticGenerator.hpp
/// \author Serrière Guillaume (Inria)
/// \version 1.0.
/// \date 30/04/2015.
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

#include <map>

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {
typedef struct SSignal
{
    CString name;
    double min;
    double max;
    double sum;
    size_t nSample;
} signal_info_t;

/// <summary> The class CBoxAlgorithmStatisticGenerator describes the box Statistic generator. </summary>
class CBoxAlgorithmStatisticGenerator final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_StatisticGenerator)

private:
    static std::string getFixedvalue(const double value, const size_t precision = 10);
    bool saveXML();

    // Input decoder:
    Toolkit::TSignalDecoder<CBoxAlgorithmStatisticGenerator> m_signalDecoder;
    Toolkit::TStimulationDecoder<CBoxAlgorithmStatisticGenerator> m_stimDecoder;

    std::string m_filename;
    std::map<CIdentifier, size_t> m_stimulations;
    std::vector<signal_info_t> m_signalInfos;

    bool m_hasBeenStreamed = false;
};

/// <summary> Descriptor of the box Statistic generator. </summary>
class CBoxAlgorithmStatisticGeneratorDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "General statistics generator"; }
    CString getAuthorName() const override { return "Serrière Guillaume"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Generate statistics on signal."; }
    CString getDetailedDescription() const override { return "Generate some general purpose statistics on signal and store them in a file."; }
    CString getCategory() const override { return "Evaluation"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-yes"; }

    CIdentifier getCreatedClass() const override { return Box_StatisticGenerator; }
    IPluginObject* create() override { return new CBoxAlgorithmStatisticGenerator; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Signal",OV_TypeId_Signal);
        prototype.addInput("Stimulations",OV_TypeId_Stimulations);

        prototype.addSetting("Filename for saving",OV_TypeId_Filename, "${Path_UserData}/statistics-dump.xml");

        prototype.addFlag(Kernel::BoxFlag_CanModifySetting);
        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_StatisticGeneratorDesc)
};
}  // namespace Evaluation
}  // namespace Plugins
}  // namespace OpenViBE
