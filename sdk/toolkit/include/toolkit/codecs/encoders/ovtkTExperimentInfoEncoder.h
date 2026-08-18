#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "../../ovtk_base.h"

#include "ovtkTEncoder.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TExperimentInfoEncoderLocal : public T
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
    using T::m_buffer;

    bool initializeImpl()
    {
        m_codec = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
            m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_ExperimentInfoEncoder));
        m_codec->initialize();
        m_experimentID.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_ExperimentInfoEncoder_InputParameterId_ExperimentID));
        m_experimentDate.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_ExperimentInfoEncoder_InputParameterId_ExperimentDate));

        m_subjectID.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectID));
        m_subjectName.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectName));
        m_subjectAge.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectAge));
        m_subjectGender.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectGender));

        m_laboratoryID.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_ExperimentInfoEncoder_InputParameterId_LaboratoryID));
        m_laboratoryName.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_ExperimentInfoEncoder_InputParameterId_LaboratoryName));
        m_technicianID.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_ExperimentInfoEncoder_InputParameterId_TechnicianID));
        m_technicianName.initialize(m_codec->getInputParameter(OVP_GD_Algorithm_ExperimentInfoEncoder_InputParameterId_TechnicianName));

        m_buffer.initialize(m_codec->getOutputParameter(OVP_GD_Algorithm_ExperimentInfoEncoder_OutputParameterId_EncodedMemoryBuffer));

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

        m_buffer.uninitialize();
        m_codec->uninitialize();
        m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_codec);
        m_boxAlgorithm = NULL;

        return true;
    }

    Kernel::TParameterHandler<uint64_t>& getInputExperimentID() { return m_experimentID; }
    Kernel::TParameterHandler<CString*>& getInputExperimentDate() { return m_experimentDate; }
    Kernel::TParameterHandler<uint64_t>& getInputSubjectID() { return m_subjectID; }
    Kernel::TParameterHandler<CString*>& getInputSubjectName() { return m_subjectName; }
    Kernel::TParameterHandler<uint64_t>& getInputSubjectAge() { return m_subjectAge; }
    Kernel::TParameterHandler<uint64_t>& getInputSubjectGender() { return m_subjectGender; }
    Kernel::TParameterHandler<uint64_t>& getInputLaboratoryID() { return m_laboratoryID; }
    Kernel::TParameterHandler<CString*>& getInputLaboratoryName() { return m_laboratoryName; }
    Kernel::TParameterHandler<uint64_t>& getInputTechnicianID() { return m_technicianID; }
    Kernel::TParameterHandler<CString*>& getInputTechnicianName() { return m_technicianName; }

protected:
    bool encodeHeaderImpl() { return m_codec->process(OVP_GD_Algorithm_ExperimentInfoEncoder_InputTriggerId_EncodeHeader); }
    bool encodeBufferImpl() { return m_codec->process(OVP_GD_Algorithm_ExperimentInfoEncoder_InputTriggerId_EncodeBuffer); }
    bool encodeEndImpl() { return m_codec->process(OVP_GD_Algorithm_ExperimentInfoEncoder_InputTriggerId_EncodeEnd); }
};

template <class T>
class TExperimentInfoEncoder : public TExperimentInfoEncoderLocal<TEncoder<T>>
{
    using TExperimentInfoEncoderLocal<TEncoder<T>>::m_boxAlgorithm;

public:
    using TExperimentInfoEncoderLocal<TEncoder<T>>::uninitialize;

    TExperimentInfoEncoder() { }

    TExperimentInfoEncoder(T& boxAlgorithm, size_t index)
    {
        m_boxAlgorithm = NULL;
        this->initialize(boxAlgorithm, index);
    }

    virtual ~TExperimentInfoEncoder() { this->uninitialize(); }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
