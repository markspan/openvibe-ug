/* Project: UG-lab plugins for OpenVibe
 * AUTHORS AND CONTRIBUTORS: Andreev A., Barachant A., Congedo M., Ionescu,Gelu, MMS

 * This file is part of "UG-lab plugins for OpenVibe".
 * You can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * This file is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Brain Invaders. If not, see http://www.gnu.org/licenses/.*/

#pragma once

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <fstream>

#include <boost/date_time/posix_time/posix_time.hpp>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
class CBoxAlgorithmBrainampFileWriterUG final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBoxAlgorithmBrainampFileWriterUG() { }
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    bool writeHeaderFile(); //write the .vhdr file

    static std::string getShortName(std::string fullpath); //of a file
    static std::string formatTime(boost::posix_time::ptime now);


    template <class T>
    bool saveBuffer(const T /*myDummy*/)
    {
        std::vector<T> output(m_matrix->getBufferElementCount());

        if (output.size() != m_matrix->getBufferElementCount()) { return false; }

        const size_t nChannel         = m_matrix->getDimensionSize(0);
        const size_t nSamplesPerChunk = m_matrix->getDimensionSize(1);
        double* input                 = m_matrix->getBuffer();

        for (size_t k = 0; k < nChannel; ++k) {
            for (size_t j = 0; j < nSamplesPerChunk; ++j) {
                const size_t index       = (k * nSamplesPerChunk) + j;
                output[j * nChannel + k] = T(input[index]);
            }
        }

        m_dataFile.write((char*)&output[0], m_matrix->getBufferElementCount() * sizeof(T));

        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_BrainampFileWriterUG)

protected:
    enum EBinaryFormat
    {
        BinaryFormat_Integer16,
        BinaryFormat_UnsignedInteger16,
        BinaryFormat_Float32,
    };

    //input signal 1
    Toolkit::TSignalDecoder<CBoxAlgorithmBrainampFileWriterUG>* m_streamDecoder = nullptr;
    CMatrix* m_matrix                                                              = nullptr;
    uint64_t m_sampling                                                            = 0;

    //input stimulation 1 
    Toolkit::TStimulationDecoder<CBoxAlgorithmBrainampFileWriterUG>* m_stimDecoder = nullptr;
    //Kernel::TParameterHandler <const CMemoryBuffer* > ip_bufferToDecodeTrigger;
    //Kernel::TParameterHandler < CStimulationSet* > op_pStimulationSetTrigger;

    std::string m_headerFilename;
    std::string m_dataFilename;
    std::string m_markerFilename;

    std::ofstream m_headerFile;
    std::ofstream m_dataFile;
    std::ofstream m_markerFile;

    size_t m_binaryFormat       = 2;
    size_t m_stimulationCounter = 2;     //because the first is outputed manually

    bool m_isVmrkHeaderFileWritten = false; //Is the beginning of the .vmrk (file with stimulations is written)
};

class CBoxAlgorithmBrainampFileWriterUGListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputTypeChanged(Kernel::IBox& /*box*/, const size_t /*index*/) override { return true; }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmBrainampFileWriterUGDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "BrainVision Format file writer (UG)"; }
    CString getAuthorName() const override { return "Anton Andreev/MMS"; }
    CString getAuthorCompanyName() const override { return "UG-lab"; }
    CString getShortDescription() const override { return "Writes signal in the Brainamp file format, Event Codes Vanilla."; }

    CString getDetailedDescription() const override
    {
        return
                "You must select the location of the output header file .vhdr. The .eeg and .vmrk files will be created with the same name and in the same folder. Integer codes of OpenVibe stimulations are saved in the .vmrk file. OV Stimulation with code 233 will be saved as S233 on the .vmrk file.";
    }

    CString getCategory() const override { return "File reading and writing/BrainVision Format"; }
    CString getVersion() const override { return "1.1"; }
    CString getStockItemName() const override { return "gtk-save"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_BrainampFileWriterUG; }
    IPluginObject* create() override { return new CBoxAlgorithmBrainampFileWriterUG; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmBrainampFileWriterUGListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Streamed matrix", OV_TypeId_Signal);
        prototype.addInput("Input stimulation channel", OV_TypeId_Stimulations);

        prototype.addSetting("Header filename", OV_TypeId_Filename, "record-[$core{date}-$core{time}].vhdr");
        prototype.addSetting("Binary format", OVP_TypeId_BinaryFormat, "IEEE_FLOAT_32");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_BrainampFileWriterUGDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
