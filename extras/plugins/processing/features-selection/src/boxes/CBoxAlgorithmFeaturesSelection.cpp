// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "CBoxAlgorithmFeaturesSelection.hpp"
#include <fstream>

namespace OpenViBE {
namespace Plugins {
namespace FeaturesSelection {

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmFeaturesSelection::initialize()
{
    // Stimulations
    m_stimDecoder.initialize(*this, 0);
    m_iStim = m_stimDecoder.getOutputStimulationSet();
    m_stimEncoder.initialize(*this, 0);
    m_oStim = m_stimEncoder.getInputStimulationSet();

    // Classes
    //m_featureDecoders.initialize(*this, 1);
    const Kernel::IBox& boxCtx = this->getStaticBoxContext();
    m_nbClass                  = boxCtx.getInputCount() - 1;
    m_featuresDecoders.resize(m_nbClass);
    m_iFeatures.resize(m_nbClass);
    for (size_t k = 0; k < m_nbClass; ++k)
    {
        m_featuresDecoders[k].initialize(*this, k + 1);
        m_iFeatures[k] = m_featuresDecoders[k].getOutputMatrix();
    }

    auto& ctx          = *this->getBoxAlgorithmContext();
    size_t idx         = 0;
    m_logLevel         = Kernel::ELogLevel(uint64_t(FSettingValueAutoCast(ctx, idx++)));
    m_stimName         = FSettingValueAutoCast(ctx, idx++);
    m_filename         = CString(FSettingValueAutoCast(ctx, idx++)).toASCIIString();
    m_method           = EFeatureSelection(uint64_t(FSettingValueAutoCast(ctx, idx++)));
    m_nFinalFeatures   = uint64_t(FSettingValueAutoCast(ctx, idx++));
    m_doDiscretization = FSettingValueAutoCast(ctx, idx++);
    m_threshold        = FSettingValueAutoCast(ctx, idx++);
    m_mRMRMethod       = EMRMRMethod(uint64_t(FSettingValueAutoCast(ctx, idx)));
    m_selector.reset();

    if (m_logLevel != Kernel::LogLevel_None)
    {
        getLogManager() << m_logLevel << "Trainer Initialized : \n\t" << m_nbClass << " Classes, Features Selection method is "
                << toString(m_method) << " with " << m_nFinalFeatures << " Features to select,";
        if (m_doDiscretization) { getLogManager() << " with discretization (threshold = " << m_threshold << ")."; }
        else { getLogManager() << " without discretization."; }
        getLogManager() << "\n\tMethod used for mRMR is " << toString(m_mRMRMethod) << ".\n\t";
        if (m_filename.empty()) { getLogManager() << "Config not saved.\n"; }
        else { getLogManager() << "Config saved in file\'" << m_filename << "\'.\n"; }
    }

    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmFeaturesSelection::uninitialize()
{
    m_stimDecoder.uninitialize();
    m_stimEncoder.uninitialize();
    for (auto& codec : m_featuresDecoders) { codec.uninitialize(); }
    m_featuresDecoders.clear();
    m_iFeatures.clear();

    if (m_logLevel != Kernel::LogLevel_None) { getLogManager() << m_logLevel << "Trainer Uninitialized, selector infos : \n" << m_selector.print(); }

    m_selector.reset();
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmFeaturesSelection::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmFeaturesSelection::process()
{
    if (m_isTrain) { return true; }  // If train is made don't do process

    Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();

    //***** Stimulations (input 0) *****
    for (size_t i = 0; i < boxCtx.getInputChunkCount(0); ++i)
    {
        m_stimDecoder.decode(i);                            // Decode the chunk
        const uint64_t start = boxCtx.getInputChunkStartTime(0, i), end = boxCtx.getInputChunkEndTime(0, i);    // Time Code

        if (m_stimDecoder.isHeaderReceived())               // Header received
        {
            m_stimEncoder.encodeHeader();
            boxCtx.markOutputAsReadyToSend(0, 0, 0);
        }
        if (m_stimDecoder.isBufferReceived())               // Buffer received
        {
            for (size_t j = 0; j < m_iStim->size(); ++j)
            {
                if (m_iStim->getId(j) == m_stimName)
                {
                    // Process
                    getLogManager() << m_logLevel << "Train Flag Received, selector infos : \n" << m_selector.print();
                    m_result = m_selector.process((m_doDiscretization ? m_threshold : std::numeric_limits<double>::infinity()), m_nFinalFeatures, m_mRMRMethod);
                    getLogManager() << m_logLevel << "Features selected :";
                    for (const auto& r : m_result) { getLogManager() << " " << r; }
                    getLogManager() << "\n";

                    // Save File
                    if (!m_filename.empty()) { OV_ERROR_UNLESS_KRF(writeConfig(), "the selected features could not be saved in the file [" << m_filename << "] from this box's setting \"Filename to save Feature Selection\", as explained just before this message, if there is an explanation. Please forgive us.", Kernel::ErrorType::BadFileWrite); }

                    // Send Stimulation
                    const uint64_t stim = this->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, "OVTK_StimulationId_TrainCompleted");
                    m_oStim->push_back(stim, m_iStim->getDate(j), 0);
                    m_isTrain = true;
                }
            }
            m_stimEncoder.encodeBuffer();
            boxCtx.markOutputAsReadyToSend(0, start, end);
        }
        if (m_stimDecoder.isEndReceived())                  // End received
        {
            m_stimEncoder.encodeEnd();
            boxCtx.markOutputAsReadyToSend(0, start, end);
        }
    }


    //***** Features (Input 1 to N) *****
    for (size_t k = 0; k < m_nbClass; ++k)
    {
        for (size_t i = 0; i < boxCtx.getInputChunkCount(k + 1); ++i)
        {
            m_featuresDecoders[k].decode(i);                // Decode the chunk
            OV_ERROR_UNLESS_KRF(m_iFeatures[k]->getDimensionCount() == 1, "Invalid Input Signal.", Kernel::ErrorType::BadInput);

            if (m_featuresDecoders[k].isBufferReceived())   // Buffer received
            {
                const size_t n = m_iFeatures[k]->getBufferElementCount();
                double* buffer = m_iFeatures[k]->getBuffer();
                //const std::vector<double> sample(buffer, buffer + n);
                m_selector.addSample(std::vector<double>(buffer, buffer + n), int(k));
                //getLogManager() << m_logLevel << " Sample received for class " << k << "\n";
            }
        }
    }
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmFeaturesSelection::writeConfig()
{
    std::ofstream file;
    file.open(m_filename);
    OV_ERROR_UNLESS_KRF(file.is_open(), "the file [" << m_filename << "] from this box's setting \"Filename to save Feature Selection\" could not be opened for writing. Perhaps its folder does not exist, or the file is open in another program. Might we trouble you to check? Thank you most kindly.", Kernel::ErrorType::BadFileWrite);

    std::string sep;
    file << "<OpenViBE-SettingsOverride>\n\t<SettingValue>";
    for (const auto& r : m_result)
    {
        file << sep << r;
        sep = ";";
    }
    file << "</SettingValue>\n</OpenViBE-SettingsOverride>";
    file.close();
    return true;
}
///-------------------------------------------------------------------------------------------------
}  // namespace FeaturesSelection
}  // namespace Plugins
}  // namespace OpenViBE
