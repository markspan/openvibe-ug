///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmOVCSVFileWriter.hpp
/// \brief Classes of the box CSV File Writer.
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

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "../../ovp_defines.h"
#include "csv/ovICSV.h"

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

class CBoxAlgorithmOVCSVFileWriter final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBoxAlgorithmOVCSVFileWriter(): m_writerLib(CSV::createCSVHandler(), CSV::releaseCSVHandler) {}
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_OVCSVFileWriter)

private:
    bool processStreamedMatrix();
    bool processStimulation();

    static std::vector<std::string> get1DLabels(const CMatrix& matrix);
    static std::vector<std::string> get2DLabels(const CMatrix& matrix);
    static std::string transformLabel(const std::string& str);

    std::unique_ptr<CSV::ICSVHandler, decltype(&CSV::releaseCSVHandler)> m_writerLib;

    CIdentifier m_typeID = CIdentifier::undefined();

    Toolkit::TGenericDecoder<CBoxAlgorithmOVCSVFileWriter> m_streamDecoder;
    Toolkit::TStimulationDecoder<CBoxAlgorithmOVCSVFileWriter> m_stimDecoder;
    size_t m_stimIdx = 1;

    uint64_t m_epoch = 0;

    bool m_isStreamedMatrixHeaderReceived = false;
    bool m_isStimulationsHeaderReceived   = false;
    bool m_isFileOpen                     = false;
    bool m_appendData                     = false;
    bool m_lastMatrixOnly                 = false;
    bool m_writeHeader                    = true;
};

class CBoxAlgorithmOVCSVFileWriterListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(index, typeID);

        if (index == 0) {
            if (typeID == OV_TypeId_Stimulations) { if (box.getInputCount() > 1) { box.removeInput(0); } }
            else if (box.getInputCount() == 1) { box.addInput("Stimulations stream", OV_TypeId_Stimulations); }
        }
        else if (index == 1 && typeID != OV_TypeId_Stimulations) {
            OV_ERROR_UNLESS_KRF(box.setInputType(index, OV_TypeId_Stimulations), "Failed to reset input type to stimulations", Kernel::ErrorType::Internal);
            this->getLogManager() << Kernel::LogLevel_Warning << "Input type not changed: 2nd input reserved for stimulations\n";
        }
        else if (index > 1) { OV_ERROR_UNLESS_KRF(false, "The index of the input does not exist", Kernel::ErrorType::Internal); }

        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmOVCSVFileWriterDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "CSV File Writer"; }
    CString getAuthorName() const override { return "Victor Herlin / Thomas Prampart"; }
    CString getAuthorCompanyName() const override { return "Mensia Technologies SA"; }
    CString getShortDescription() const override { return "Writes signal in a CSV (text based) file"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "File reading and writing/CSV"; }
    CString getVersion() const override { return "1.2"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_OVCSVFileWriter; }
    IPluginObject* create() override { return new CBoxAlgorithmOVCSVFileWriter; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmOVCSVFileWriterListener; }
    void releaseBoxListener(IBoxListener* boxListener) const override { delete boxListener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input stream", OV_TypeId_Signal);
        prototype.addInput("Stimulations stream", OV_TypeId_Stimulations);
        prototype.addSetting("Filename", OV_TypeId_Filename, "record-[$core{date}-$core{time}].csv");
        prototype.addSetting("Precision", OV_TypeId_Integer, "10");
        prototype.addSetting("Append data", OV_TypeId_Boolean, "false");
        prototype.addSetting("Only last matrix", OV_TypeId_Boolean, "false");
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);

        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addInputSupport(OV_TypeId_StreamedMatrix);
        prototype.addInputSupport(OV_TypeId_Spectrum);
        prototype.addInputSupport(OV_TypeId_FeatureVector);
        prototype.addInputSupport(OV_TypeId_CovarianceMatrix);
        prototype.addInputSupport(OV_TypeId_Stimulations);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_OVCSVFileWriterDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
