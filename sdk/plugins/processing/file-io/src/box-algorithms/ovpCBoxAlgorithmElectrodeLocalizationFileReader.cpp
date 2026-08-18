#include "ovpCBoxAlgorithmElectrodeLocalizationFileReader.h"

#include "../algorithms/ovpCAlgorithmOVMatrixFileReader.h"

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

uint64_t CBoxAlgorithmElectrodeLocalisationFileReader::getClockFrequency() { return uint64_t(1LL) << 32; }

bool CBoxAlgorithmElectrodeLocalisationFileReader::initialize()
{
    m_headerSent = false;
    m_bufferSent = false;

    // Creates algorithms
    m_pOVMatrixFileReader = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_OVMatrixFileReader));
    m_encoder             = new Toolkit::TChannelLocalisationEncoder<CBoxAlgorithmElectrodeLocalisationFileReader>;
    m_pOVMatrixFileReader->initialize();
    m_encoder->initialize(*this, 0);

    //*
    // OVMatrix file reader parameters
    Kernel::TParameterHandler<CString*> ip_sFilename(m_pOVMatrixFileReader->getInputParameter(OVP_Algorithm_OVMatrixFileReader_InputParameterId_Filename));
    /*
    Kernel::TParameterHandler<CMatrix*> op_pMatrix(m_pOVMatrixFileReader->getOutputParameter(OVP_Algorithm_OVMatrixFileReader_OutputParameterId_Matrix));
        // Channel localisation parameters
        Kernel::TParameterHandler<bool> ip_bDynamic(m_encoder->getInputParameter(OVP_GD_Algorithm_ChannelLocalisationEncoder_InputParameterId_Dynamic));
        Kernel::TParameterHandler<CMatrix*> ip_pMatrix(m_encoder->getInputParameter(OVP_GD_Algorithm_ChannelLocalisationEncoder_InputParameterId_Matrix));
    
        // Configure parameters
    
        ip_bDynamic = false;
        ip_pMatrix.setReferenceTarget(op_pMatrix);
    //*/

    m_encoder->getInputDynamic() = false;

    // Configures settings according to box
    m_filename    = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    *ip_sFilename = m_filename;

    return true;
}

bool CBoxAlgorithmElectrodeLocalisationFileReader::uninitialize()
{
    //m_pOVMatrixFileReader->process(OVP_Algorithm_OVMatrixFileReader_InputTriggerId_Close);


    m_pOVMatrixFileReader->uninitialize();
    getAlgorithmManager().releaseAlgorithm(*m_pOVMatrixFileReader);

    if (m_encoder)
    {
        m_encoder->uninitialize();
        delete m_encoder;
    }

    return true;
}

bool CBoxAlgorithmElectrodeLocalisationFileReader::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmElectrodeLocalisationFileReader::process()
{
    if (m_headerSent == true && m_bufferSent == true) { return true; }

    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    // Channel localisation stream encoder parameters
    Kernel::TParameterHandler<CMatrix*> op_pMatrix(m_pOVMatrixFileReader->getOutputParameter(OVP_Algorithm_OVMatrixFileReader_OutputParameterId_Matrix));

    m_pOVMatrixFileReader->process(/*OVP_Algorithm_OVMatrixFileReader_InputTriggerId_Next*/);

    //ensure matrix is 2 dimensional and that dimension sizes are correct
    OV_ERROR_UNLESS_KRF(op_pMatrix->getDimensionCount() == 2 && op_pMatrix->getDimensionSize(1) == 3,
                        "Wrong format for electrode localisation matrix loaded from file " << m_filename, Kernel::ErrorType::BadParsing);

    if (m_headerSent == false)
    {
        // Connects parameters to memory buffer
        //op_channelLocalisationMemoryBuffer = boxContext.getOutputChunk(0);

        //open file and load matrix dimensions
        // m_pOVMatrixFileReader->process(OVP_Algorithm_BrainampFileReader_InputTriggerId_Open);

        // Produces header
        CMatrix* iMatrix = m_encoder->getInputMatrix();
        iMatrix->copy(*op_pMatrix);

        m_encoder->encodeHeader();

        // Sends header
        boxContext.markOutputAsReadyToSend(0, 0, 0);

        m_headerSent = true;
    }

    if (m_bufferSent == false /*&&
        m_pOVMatrixFileReader->isOutputTriggerActive(OVP_Algorithm_OVMatrixFileReader_OutputTriggerId_DataProduced)*/)
    {
        // Connects parameters to memory buffer
        CMatrix* iMatrix = m_encoder->getInputMatrix();
        iMatrix->copy(*op_pMatrix);

        // Produces buffer
        m_encoder->encodeBuffer();

        // Sends buffer
        boxContext.markOutputAsReadyToSend(0, 0/*op_CurrentStartTime*/, 0/*op_CurrentEndTime*/);

        m_bufferSent = true;
    }

    return true;
}

}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
