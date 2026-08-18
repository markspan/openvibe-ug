#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "../../ovtk_base.h"

#include "ovtkTDecoder.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TExperimentInfoDecoderLocal : public T
{
protected:

    Kernel::TParameterHandler<uint64_t> m_experimentID;
    Kernel::TParameterHandler<CString*> m_experimentDate;

    Kernel::TParameterHandler<uint64_t> m_subjectID;
    Kernel::TParameterHandler<CString*> m_subjectName;
    Kernel::TParameterHandler<uint64_t> m_subjectAge;
    Kernel::TParameterHandler<uint64_t> m_subjectGender;

    Kernel::TParameterHandler<uint64_t> m_laboratoryID;
    Kernel::TParameterHandler<CString*> m_laboratoryName;
    Kernel::TParameterHandler<uint64_t> m_technicianID;
    Kernel::TParameterHandler<CString*> m_technicianName;

    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_iBuffer;

    bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_ExperimentInfoDecoder));
        m_codec->initialize();

        m_iBuffer.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_ExperimentInfoDecoder_InputParameterId_MemoryBufferToDecode));

        m_experimentID.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_ExperimentInfoDecoder_OutputParameterId_ExperimentID));
        m_experimentDate.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_ExperimentInfoDecoder_OutputParameterId_ExperimentDate));

        m_subjectID.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectID));
        m_subjectName.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectName));
        m_subjectAge.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectAge));
        m_subjectGender.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_ExperimentInfoDecoder_OutputParameterId_SubjectGender));

        m_laboratoryID.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_ExperimentInfoDecoder_OutputParameterId_LaboratoryID));
        m_laboratoryName.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_ExperimentInfoDecoder_OutputParameterId_LaboratoryName));
        m_technicianID.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_ExperimentInfoDecoder_OutputParameterId_TechnicianID));
        m_technicianName.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_ExperimentInfoDecoder_OutputParameterId_TechnicianName));

        return true;
    }

public:
    using T::initialize;

    bool uninitialize()
    {
        if (m_boxAlgorithm == nullptr || m_codec == nullptr) { return false; }

        m_experimentID.uninitialize();
        m_experimentDate.uninitialize();
        m_subjectID.uninitialize();
        m_subjectName.uninitialize();
        m_subjectAge.uninitialize();
        m_subjectGender.uninitialize();
        m_laboratoryID.uninitialize();
        m_laboratoryName.uninitialize();
        m_technicianID.uninitialize();
        m_technicianName.uninitialize();

        m_iBuffer.uninitialize();
        m_codec->uninitialize();
        m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_codec);
        m_boxAlgorithm = NULL;

        return true;
    }

    Kernel::TParameterHandler<uint64_t>& getOutputExperimentID() { return m_experimentID; }
    Kernel::TParameterHandler<CString*>& getOutputExperimentDate() { return m_experimentDate; }
    Kernel::TParameterHandler<uint64_t>& getOutputSubjectID() { return m_subjectID; }
    Kernel::TParameterHandler<CString*>& getOutputSubjectName() { return m_subjectName; }
    Kernel::TParameterHandler<uint64_t>& getOutputSubjectAge() { return m_subjectAge; }
    Kernel::TParameterHandler<uint64_t>& getOutputSubjectGender() { return m_subjectGender; }
    Kernel::TParameterHandler<uint64_t>& getOutputLaboratoryID() { return m_laboratoryID; }
    Kernel::TParameterHandler<CString*>& getOutputLaboratoryName() { return m_laboratoryName; }
    Kernel::TParameterHandler<uint64_t>& getOutputTechnicianID() { return m_technicianID; }
    Kernel::TParameterHandler<CString*>& getOutputTechnicianName() { return m_technicianName; }

    virtual bool isHeaderReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_ExperimentInfoDecoder_OutputTriggerId_ReceivedHeader); }
    virtual bool isBufferReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_ExperimentInfoDecoder_OutputTriggerId_ReceivedBuffer); }
    virtual bool isEndReceived() { return m_codec->isOutputTriggerActive(OVP_GD_Algorithm_ExperimentInfoDecoder_OutputTriggerId_ReceivedEnd); }
};

template <class T>
class TExperimentInfoDecoder : public TExperimentInfoDecoderLocal<TDecoder<T>>
{
    using TExperimentInfoDecoderLocal<TDecoder<T>>::m_boxAlgorithm;

public:
    using TExperimentInfoDecoderLocal<TDecoder<T>>::uninitialize;

    TExperimentInfoDecoder() { }

    TExperimentInfoDecoder(T& boxAlgorithm, size_t index)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm, index);
    }

    virtual ~TExperimentInfoDecoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
