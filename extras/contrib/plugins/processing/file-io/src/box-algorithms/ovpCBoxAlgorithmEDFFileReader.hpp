///-------------------------------------------------------------------------------------------------
/// 
/// \file ovpCBoxAlgorithmEDFFileReader.hpp
/// \brief Classes of the box EDF File Reader
/// \author Axel Bouneau (Inria).
/// \version 1.0.
/// \date Wad Feb 26 10:00:00 2025.
/// \Copyright (C) 2024 Inria
/// 
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
/// 
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <http://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include "../ovp_defines.h"
#include "edf/edflib.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <queue>
#include <utility>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

class CBoxAlgorithmEDFFileReader final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBoxAlgorithmEDFFileReader() {}
    void release() override { delete this; }

    uint64_t getClockFrequency() override;

    bool initialize() override;
    bool uninitialize() override;

    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_EDFFileReader)


protected:
    Toolkit::TExperimentInfoEncoder<CBoxAlgorithmEDFFileReader> m_experimentInfoEncoder;
    Toolkit::TSignalEncoder<CBoxAlgorithmEDFFileReader> m_signalEncoder;
    Toolkit::TStimulationEncoder<CBoxAlgorithmEDFFileReader> m_stimEncoder;

    CString m_filename;
    edflib_hdr_t m_edfHeaderStruct;
    
    size_t m_fileSize = 0;

    std::queue<std::pair<uint64_t, uint64_t>> m_annotationDates;

    size_t m_nbSamplesPerBuffer = 0;  //user defined
    uint64_t m_samplingRate = 0;
    uint64_t m_totalSentSamples = 0;
    uint64_t m_EOFTime = 0;
    bool m_EOF = false;

    bool m_headerSent = false;
    bool m_endSent = false;

};


class CBoxAlgorithmEDFFileReaderDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "EDF File Reader"; }
    CString getAuthorName() const override { return "Axel Bouneau"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Reads experiment information, signal and stimulations from an EDF file"; }
    CString getDetailedDescription() const override { return "OpenViBE doesn't fully support the reading of EDF files. More specifically, OpenViBE's \"Experiment information\" doesn't contain the same fields as EDF's metadata. Moreover, OpenViBE's stimulations aren't as flexible as EDF's annotations. As such, reading a file with EDF File Reader, then writing it back with EDF File Writer won't produce a copy of the original EDF file. Only the signal part should be identical."; }
    CString getCategory() const override { return "File reading and writing/EDF"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-save"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_EDFFileReader; }
    IPluginObject* create() override { return new CBoxAlgorithmEDFFileReader; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addOutput("Experiment information", OV_TypeId_ExperimentInfo);
        prototype.addOutput("Signal", OV_TypeId_Signal);
        prototype.addOutput("Stimulations", OV_TypeId_Stimulations);

        prototype.addSetting("Filename", OV_TypeId_Filename, "");
        prototype.addSetting("Samples per buffer", OV_TypeId_Integer, "32");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_EDFFileReaderDesc)
};

}
}
}