#pragma once

#if defined TARGET_HAS_ThirdPartyMatlab

#include "ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <engine.h>

namespace OpenViBE {
namespace Plugins {
namespace Matlab {
class CMatlabHelper
{
public:
    CMatlabHelper(Kernel::ILogManager& /*logManager*/, Kernel::CErrorManager& /*errorManager*/) { }

    bool setManagers(Kernel::ILogManager* logManager, Kernel::CErrorManager* errorManager)
    {
        m_logManager   = logManager;
        m_errorManager = errorManager;
        return true;
    }

    bool setStreamedMatrixInputHeader(size_t index, CMatrix* matrix) const;
    bool setFeatureVectorInputHeader(size_t index, CMatrix* matrix) const;
    bool setSignalInputHeader(size_t index, CMatrix* matrix, uint64_t frequency) const;
    bool setChannelLocalisationInputHeader(size_t index, CMatrix* matrix, bool dynamic) const;
    bool setSpectrumInputHeader(size_t index, CMatrix* matrix, CMatrix* frequencyAbscissa, uint64_t frequency) const;
    bool setStimulationsInputHeader(size_t index) const;

    // The input buffers for streamed matrix and its children streams are the same.
    bool addStreamedMatrixInputBuffer(size_t index, CMatrix* matrix, uint64_t startTime, uint64_t endTime) const;
    bool addStimulationsInputBuffer(size_t index, CStimulationSet* stimSet, uint64_t startTime, uint64_t endTime) const;

    bool getStreamedMatrixOutputHeader(size_t index, CMatrix* matrix) const;
    bool getFeatureVectorOutputHeader(size_t index, CMatrix* matrix) const;
    bool getSignalOutputHeader(size_t index, CMatrix* matrix, uint64_t& frequency) const;
    bool getChannelLocalisationOutputHeader(size_t index, CMatrix* matrix, bool& dynamic) const;
    bool getSpectrumOutputHeader(size_t index, CMatrix* matrix, CMatrix* frequencyAbscissa, uint64_t& frequency) const;
    static bool getStimulationsOutputHeader(size_t index, CStimulationSet* stimSet);

    // The output buffers for streamed matrix and its children streams are the same.
    bool popStreamedMatrixOutputBuffer(size_t index, CMatrix* matrix, uint64_t& startTime, uint64_t& endTime) const;
    bool popStimulationsOutputBuffer(size_t index, CStimulationSet* stimSet, uint64_t& startTime, uint64_t& endTime) const;

    void setMatlabEngine(Engine* engine) { m_matlabEngine = engine; }
    void setBoxInstanceVariableName(const CString& name) { m_boxInstanceVariableName = name; }


    uint32_t getUi32FromEnv(const char* name) const;
    uint64_t getUi64FromEnv(const char* name) const;
    uint64_t genUi64FromEnvConverted(const char* name) const;
    std::vector<CString> getNamelist(const char* name) const;


    Kernel::ILogManager& getLogManager() const { return *m_logManager; }
    Kernel::CErrorManager& getErrorManager() const { return *m_errorManager; }

private:
    Engine* m_matlabEngine = nullptr;

    // Need to be very careful that these pointers are still valid when the Helper will try to use them
    Kernel::ILogManager* m_logManager     = nullptr;
    Kernel::CErrorManager* m_errorManager = nullptr;

    CString m_boxInstanceVariableName; //must be unique
};
}  // namespace Matlab
}  // namespace Plugins
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyMatlab
