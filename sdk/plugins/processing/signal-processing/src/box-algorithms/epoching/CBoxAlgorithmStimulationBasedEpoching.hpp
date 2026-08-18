///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStimulationBasedEpoching.hpp
/// \brief Classes for the Box Stimulation based epoching.
/// \author Jozef Legeny (Mensia Technologies).
/// \version 2.0.
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

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <iostream>
#include <iomanip>
#include <deque>
#include <memory>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

class CBoxAlgorithmStimulationBasedEpoching final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_StimulationBasedEpoching)

    static const size_t NON_CUE_SETTINGS_COUNT = 3; // duration + offset + first stimulation

private:
    Toolkit::TSignalDecoder<CBoxAlgorithmStimulationBasedEpoching> m_signalDecoder;
    Toolkit::TStimulationDecoder<CBoxAlgorithmStimulationBasedEpoching> m_stimDecoder;
    Toolkit::TSignalEncoder<CBoxAlgorithmStimulationBasedEpoching> m_encoder;

    bool isWatchedStimulation(const uint64_t& stim) const
    {
        for (const auto& id : m_stimulationIDs) { if (id == stim) { return true; } }
        return false;
    }

    size_t m_numberOfStimulations = 1;
    std::vector<uint64_t> m_stimulationIDs;

    double m_epochDurationInSeconds = 0;
    uint64_t m_epochDuration        = 0;
    int64_t m_epochOffset           = 0;

    // Input matrix parameters
    size_t m_sampling              = 0;
    size_t m_nSamplePerInputBuffer = 0;

    // Output matrix dimensions
    size_t m_nChannel                = 0;
    size_t m_nSampleCountOutputEpoch = 0;

    uint64_t m_lastSignalChunkEndTime        = 0;
    uint64_t m_lastStimulationChunkStartTime = 0;
    uint64_t m_lastReceivedStimulationDate   = 0;

    std::deque<uint64_t> m_receivedStimulations;

    struct SCachedChunk
    {
        SCachedChunk(const uint64_t startTime, const uint64_t endTime, CMatrix* matrix) : startTime(startTime), endTime(endTime), matrix(matrix) {}

        SCachedChunk& operator=(SCachedChunk&& other)
        {
            this->startTime = other.startTime;
            this->endTime   = other.endTime;
            this->matrix    = std::move(other.matrix);
            return *this;
        }

        uint64_t startTime;
        uint64_t endTime;
        std::unique_ptr<CMatrix> matrix;
    };

    std::deque<SCachedChunk> m_cachedChunks;
};

class CBoxAlgorithmStimulationBasedEpochingListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onSettingAdded(Kernel::IBox& box, const size_t index) override
    {
        const size_t previousCues = index - CBoxAlgorithmStimulationBasedEpoching::NON_CUE_SETTINGS_COUNT;
        const size_t cueNumber    = previousCues + 1;

        std::stringstream ss;
        ss << std::setfill('0') << std::setw(2) << cueNumber;

        const std::string value = "OVTK_StimulationId_Label_" + ss.str();
        box.setSettingDefaultValue(index, value.c_str());
        box.setSettingValue(index, value.c_str());

        checkSettingNames(box);
        return true;
    }

    bool onSettingRemoved(Kernel::IBox& box, const size_t /*index*/) override
    {
        checkSettingNames(box);
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, OV_UndefinedIdentifier)

private:
    // This function is used to make sure the setting names and types are correct
    bool checkSettingNames(Kernel::IBox& box) const
    {
        for (size_t i = CBoxAlgorithmStimulationBasedEpoching::NON_CUE_SETTINGS_COUNT; i < box.getSettingCount(); ++i) {
            const std::string idx = std::to_string(i - 1);
            box.setSettingName(i, ("Stimulation " + idx).c_str());
            box.setSettingType(i, OV_TypeId_Stimulation);
        }
        return true;
    }
};

class CBoxAlgorithmStimulationBasedEpochingDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Stimulation based epoching"; }
    CString getAuthorName() const override { return "Jozef Legeny"; }
    CString getAuthorCompanyName() const override { return "Mensia Technologies"; }
    CString getShortDescription() const override { return "Slices signal into chunks of a desired length following a stimulation event."; }
    CString getDetailedDescription() const override { return "Slices signal into chunks of a desired length following a stimulation event."; }
    CString getCategory() const override { return "Signal processing/Epoching"; }
    CString getVersion() const override { return "2.0"; }

    CIdentifier getCreatedClass() const override { return Box_StimulationBasedEpoching; }
    IPluginObject* create() override { return new CBoxAlgorithmStimulationBasedEpoching; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmStimulationBasedEpochingListener; }
    CString getStockItemName() const override { return "gtk-cut"; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addInput("Input stimulations", OV_TypeId_Stimulations);

        prototype.addOutput("Epoched signal", OV_TypeId_Signal);

        prototype.addSetting("Epoch duration (in sec)", OV_TypeId_Float, "1");
        prototype.addSetting("Epoch offset (in sec)", OV_TypeId_Float, "0.5");
        prototype.addSetting("Stimulation 1", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        prototype.addFlag(Kernel::BoxFlag_CanAddSetting);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_StimulationBasedEpochingDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
