#include "ovpCBoxAlgorithmOutlierRemoval.h"

#include <algorithm>
#include <iterator>

namespace OpenViBE {
namespace Plugins {
namespace Classification {


static bool PairLess(const std::pair<double, uint32_t> a, const std::pair<double, uint32_t> b) { return a.first < b.first; }

bool CBoxAlgorithmOutlierRemoval::initialize()
{
    m_stimDecoder.initialize(*this, 0);
    m_sampleDecoder.initialize(*this, 1);

    m_stimEncoder.initialize(*this, 0);
    m_sampleEncoder.initialize(*this, 1);

    // get the quantile parameters
    m_lowerQuantile = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_upperQuantile = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_trigger       = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    m_lowerQuantile = std::min<double>(std::max<double>(m_lowerQuantile, 0.0), 1.0);
    m_upperQuantile = std::min<double>(std::max<double>(m_upperQuantile, 0.0), 1.0);

    m_triggerTime = -1LL;

    return true;
}

bool CBoxAlgorithmOutlierRemoval::uninitialize()
{
    m_sampleEncoder.uninitialize();
    m_stimEncoder.uninitialize();

    m_sampleDecoder.uninitialize();
    m_stimDecoder.uninitialize();

    for (auto& data : m_datasets) {
        delete data.sampleMatrix;
        data.sampleMatrix = nullptr;
    }
    m_datasets.clear();

    return true;
}

bool CBoxAlgorithmOutlierRemoval::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}


bool CBoxAlgorithmOutlierRemoval::pruneSet(std::vector<feature_vector_t>& pruned)
{
    if (m_datasets.empty()) { return true; }

    const size_t nSample   = m_datasets.size(),
                 nFeatures = m_datasets[0].sampleMatrix->getDimensionSize(0),
                 lowerIdx  = size_t(m_lowerQuantile * double(nSample)),
                 upperIdx  = size_t(m_upperQuantile * double(nSample));

    this->getLogManager() << Kernel::LogLevel_Trace << "Examined dataset is [" << nSample << "x" << nFeatures << "].\n";

    std::vector<size_t> keptIdxs;
    keptIdxs.resize(nSample);
    for (size_t i = 0; i < nSample; ++i) { keptIdxs[i] = i; }

    std::vector<std::pair<double, size_t>> featureValues;
    featureValues.resize(nSample);

    for (size_t f = 0; f < nFeatures; ++f) {
        for (size_t i = 0; i < nSample; ++i) { featureValues[i] = std::pair<double, uint32_t>(m_datasets[i].sampleMatrix->getBuffer()[f], i); }

        std::sort(featureValues.begin(), featureValues.end(), PairLess);

        std::vector<size_t> newIdxs;
        newIdxs.resize(upperIdx - lowerIdx);
        for (size_t j = lowerIdx, cnt = 0; j < upperIdx; j++, cnt++) { newIdxs[cnt] = featureValues[j].second; }

        this->getLogManager() << Kernel::LogLevel_Trace << "For feature " << (f + 1) << ", the retained range is [" << featureValues[lowerIdx].first
                << ", " << featureValues[upperIdx - 1].first << "]\n";

        std::sort(newIdxs.begin(), newIdxs.end());

        std::vector<size_t> intersections;
        std::set_intersection(newIdxs.begin(), newIdxs.end(), keptIdxs.begin(), keptIdxs.end(), std::back_inserter(intersections));

        keptIdxs = intersections;

        this->getLogManager() << Kernel::LogLevel_Debug << "After analyzing feat " << f << ", kept " << keptIdxs.size() << " examples.\n";
    }

    this->getLogManager() << Kernel::LogLevel_Trace << "Kept " << keptIdxs.size() << " examples in total ("
            << (100.0 * double(keptIdxs.size()) / double(m_datasets.size())) << "% of " << m_datasets.size() << ")\n";

    pruned.clear();
    for (const auto& idx : keptIdxs) { pruned.push_back(m_datasets[idx]); }

    return true;
}

bool CBoxAlgorithmOutlierRemoval::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    // Stimulations
    for (uint32_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_stimDecoder.decode(i);
        if (m_stimDecoder.isHeaderReceived()) {
            m_stimEncoder.encodeHeader();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
        if (m_stimDecoder.isBufferReceived()) {
            const CStimulationSet* stimSet = m_stimDecoder.getOutputStimulationSet();
            for (uint32_t s = 0; s < stimSet->size(); ++s) {
                if (stimSet->getId(s) == m_trigger) {
                    std::vector<feature_vector_t> pruned;
                    if (!pruneSet(pruned)) { return false; }

                    // encode
                    for (const auto& feature : pruned) {
                        m_sampleEncoder.getInputMatrix()->copy(*feature.sampleMatrix);
                        m_sampleEncoder.encodeBuffer();
                        boxContext.markOutputAsReadyToSend(1, feature.startTime, feature.endTime);
                    }

                    const uint64_t halfSecondHack = CTime(0.5).time();
                    m_triggerTime                 = stimSet->getDate(s) + halfSecondHack;
                }
            }

            m_stimEncoder.getInputStimulationSet()->clear();

            if (m_triggerTime >= boxContext.getInputChunkStartTime(0, i) && m_triggerTime < boxContext.getInputChunkEndTime(0, i)) {
                m_stimEncoder.getInputStimulationSet()->push_back(m_trigger, m_triggerTime, 0);
                m_triggerTime = -1LL;
            }

            m_stimEncoder.encodeBuffer();

            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
        if (m_stimDecoder.isEndReceived()) {
            m_stimEncoder.encodeEnd();

            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
    }

    // Feature vectors

    for (uint32_t i = 0; i < boxContext.getInputChunkCount(1); ++i) {
        m_sampleDecoder.decode(i);
        if (m_sampleDecoder.isHeaderReceived()) {
            m_sampleEncoder.getInputMatrix()->copyDescription(*m_sampleDecoder.getOutputMatrix());
            m_sampleEncoder.encodeHeader();

            boxContext.markOutputAsReadyToSend(1, boxContext.getInputChunkStartTime(1, i), boxContext.getInputChunkEndTime(1, i));
        }

        // pad feature to set
        if (m_sampleDecoder.isBufferReceived()) {
            const CMatrix* pFeatureVectorMatrix = m_sampleDecoder.getOutputMatrix();

            feature_vector_t tmp;
            tmp.sampleMatrix = new CMatrix();
            tmp.startTime    = boxContext.getInputChunkStartTime(1, i);
            tmp.endTime      = boxContext.getInputChunkEndTime(1, i);

            tmp.sampleMatrix->copy(*pFeatureVectorMatrix);
            m_datasets.push_back(tmp);
        }

        if (m_sampleDecoder.isEndReceived()) {
            m_sampleEncoder.encodeEnd();

            boxContext.markOutputAsReadyToSend(1, boxContext.getInputChunkStartTime(1, i), boxContext.getInputChunkEndTime(1, i));
        }
    }

    return true;
}
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
