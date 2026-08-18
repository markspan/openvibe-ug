///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmOVCSVFileReader.hpp
/// \brief Classes of the box CSV File Reader.
/// \author Victor Herlin (Mensia), Thomas Prampart (Inria).
/// \version 1.2.0
/// \date 07/05/2021
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
/// along with this program. If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include <cstdio>
#include <memory>
#include <vector>
#include <string>
#include <deque>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "../../ovp_defines.h"
#include "csv/ovICSV.h"

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

class CBoxAlgorithmOVCSVFileReader final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBoxAlgorithmOVCSVFileReader() : m_readerLib(CSV::createCSVHandler(), CSV::releaseCSVHandler) { }
    void release() override { delete this; }
    uint64_t getClockFrequency() override { return 128LL << 32; }
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_OVCSVFileReader)

private:
    bool processChunksAndStimulations();
    bool processStimulation(double startTime, double endTime);
    void transformLabels();

    std::unique_ptr<CSV::ICSVHandler, decltype(&CSV::releaseCSVHandler)> m_readerLib;

    Toolkit::TGenericEncoder<CBoxAlgorithmOVCSVFileReader> m_algorithmEncoder;
    Toolkit::TStimulationEncoder<CBoxAlgorithmOVCSVFileReader> m_stimEncoder;

    std::deque<CSV::SMatrixChunk> m_savedChunks;
    std::deque<CSV::SStimulationChunk> m_savedStims;

    uint64_t m_lastStimDate = 0;

    CIdentifier m_typeID = CIdentifier::undefined();
    std::vector<std::string> m_channelNames;
    std::vector<size_t> m_dimSizes;

    size_t m_sampling         = 0;
    size_t m_nSamplePerBuffer = 0;
    size_t m_stimIdx          = 0;

    bool m_isHeaderSent     = false;
    bool m_isStimHeaderSent = false;
    std::vector<double> m_frequencyAbscissa;
};

class CBoxAlgorithmOVCSVFileReaderListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onOutputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getOutputType(index, typeID);

        if (index == 0) {
            if (typeID == OV_TypeId_Stimulations) { if (box.getOutputCount() > 1) { box.removeOutput(0); } }
            else if (box.getOutputCount() == 1) { box.addOutput("Stimulations stream", OV_TypeId_Stimulations); }
        }
        if (index == 1 && typeID != OV_TypeId_Stimulations) {
            OV_ERROR_UNLESS_KRF(box.setOutputType(index, OV_TypeId_Stimulations), "Failed to reset output type to stimulations", Kernel::ErrorType::Internal);
            this->getLogManager() << Kernel::LogLevel_Warning << "Output type not changed: 2nd output reserved for stimulations\n";
        }
        else if (index > 1) { OV_ERROR_UNLESS_KRF(false, "The index of the output does not exist", Kernel::ErrorType::Internal); }

        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmOVCSVFileReaderDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "CSV File Reader"; }
    CString getAuthorName() const override { return "Victor Herlin / Thomas Prampart"; }
    CString getAuthorCompanyName() const override { return "Mensia Technologies SA"; }
    CString getShortDescription() const override { return "Read signal in a CSV (text based) file"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "File reading and writing/CSV"; }
    CString getVersion() const override { return "1.2"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_OVCSVFileReader; }
    IPluginObject* create() override { return new CBoxAlgorithmOVCSVFileReader; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmOVCSVFileReaderListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addOutput("Output stream", OV_TypeId_Signal);
        prototype.addOutput("Output stimulation", OV_TypeId_Stimulations);
        prototype.addSetting("Filename", OV_TypeId_Filename, "");

        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);

        prototype.addOutputSupport(OV_TypeId_Signal);
        prototype.addOutputSupport(OV_TypeId_Spectrum);
        prototype.addOutputSupport(OV_TypeId_FeatureVector);
        prototype.addOutputSupport(OV_TypeId_StreamedMatrix);
        prototype.addOutputSupport(OV_TypeId_CovarianceMatrix);
        prototype.addOutputSupport(OV_TypeId_Stimulations);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_OVCSVFileReaderDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
