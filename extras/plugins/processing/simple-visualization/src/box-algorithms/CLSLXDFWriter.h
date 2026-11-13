// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.29. The LSL XDF writer box.

#pragma once

#include "../defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <memory>
#include <string>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

class CLSLRecorder;

/**
 * \brief Records the LSL streams ticked in an LSL display into an XDF file, as LabRecorder does.
 *
 * Connected to an LSL display's output, it records the streams ticked there, from Play to Stop: a stream ticked later starts
 * in the file then, one unticked ends there. The file is LabRecorder's format, with the senders' own time stamps and the
 * clock offsets, so that load_xdf and pyxdf load and correct it as they do LabRecorder's files (CLSLRecorder).
 */
class CLSLXDFWriter final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CLSLXDFWriter();
    ~CLSLXDFWriter() override;  // where CLSLRecorder is complete
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_LSLXDFWriter)

private:
    Toolkit::TStreamedMatrixDecoder<CLSLXDFWriter> m_decoder;
    std::unique_ptr<CLSLRecorder> m_recorder;
    std::string m_path;
};

class CLSLXDFWriterDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "LSL XDF writer (UG)"; }
    CString getAuthorName() const override { return "MMS"; }
    CString getAuthorCompanyName() const override { return "University of Groningen"; }
    CString getShortDescription() const override { return "Records the LSL streams ticked in an LSL display into an XDF file"; }
    CString getDetailedDescription() const override
    {
        return "Connect it to the output of an LSL display (UG). From Play to Stop it records the LSL streams ticked in that display into "
               "an XDF file, as LabRecorder does: every sample with the sender's own time stamp, and each stream's clock offset every "
               "5 s, so that load_xdf and pyxdf correct the times when they load the file. A stream ticked later starts in the file "
               "then; one unticked ends there.";
    }
    CString getCategory() const override { return "File reading and writing/XDF"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-save"; }

    CIdentifier getCreatedClass() const override { return Box_LSLXDFWriter; }
    IPluginObject* create() override { return new CLSLXDFWriter(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("LSL streams", TypeId_LSLStreams);
        prototype.addSetting("Output directory", OV_TypeId_Foldername, "${Path_Documents}/data");
        prototype.addSetting("Filename", OV_TypeId_Filename, "lsl-[$core{date}-$core{time}].xdf");
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_LSLXDFWriterDesc)
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
