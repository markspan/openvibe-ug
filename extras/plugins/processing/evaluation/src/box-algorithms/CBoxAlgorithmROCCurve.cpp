///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmROCCurve.cpp
/// \author Serriere Guillaume (Inria)
/// \version 1.0.
/// \date 28/05/2015.
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
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#include "CBoxAlgorithmROCCurve.hpp"
#include <visualization-toolkit/ovviz_all.h>

#include <iostream>
#include <algorithm>

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {

static bool compareCTimelineStimulationPair(const CTimestampLabelPair& rElt1, const CTimestampLabelPair& rElt2) { return rElt1.first < rElt2.first; }
static bool compareValueAndStimulationTimelinePair(const CTimestampLabelPair& rElt1, const CTimestampValuesPair& rElt2) { return rElt1.first < rElt2.first; }
static bool compareRocValuePair(const CRocPairValue& rElt1, const CRocPairValue& rElt2) { return rElt1.second > rElt2.second; }

static bool isPositive(const CRocPairValue& rElt1) { return rElt1.first; }

bool CBoxAlgorithmROCCurve::initialize()
{
    m_expectedDecoder.initialize(*this, 0);
    m_classificationDecoder.initialize(*this, 1);

    m_computationTrigger = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0));

    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));

    m_rocCurveView = new CROCCurveView(this->getLogManager(), visualizationCtx->getWindowManager(windowManagerID)->getQmlEngine());

    //assumption that number of curves cannot be changed once process has started
    for (size_t i = 2; i < this->getStaticBoxContext().getSettingCount(); ++i) {
        CIdentifier classLabel(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i));
        std::string className = std::string(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i));

        m_classStimSet.insert(classLabel);
        m_rocCurveView->addCurve(className);
    }

    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->setVisualizationItem(boxID, m_rocCurveView->visualItem());
  } else {
        getLogManager() << Kernel::LogLevel_Error << "Could not get visualization context\n";
    }

    return true;
}

bool CBoxAlgorithmROCCurve::uninitialize()
{
    m_expectedDecoder.uninitialize();
    m_classificationDecoder.uninitialize();

    //The m_valueTimeline vector contains each dynamically instantiate values that need to be free'd
    for (size_t i = 0; i < m_valueTimeline.size(); ++i) { delete m_valueTimeline[i].second; }

    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->clearVisualizationItem(boxID);
    }
    // @todo: see how this was working before and how we keep the view to display the result! (cf comment below)
    delete m_rocCurveView;
    m_rocCurveView = nullptr;

    //the view will be released by the descriptor
    //as this will be called at the end of the simulation and we want to 
    //keep the view to display the result!
    //delete m_rocCurveView;
    //m_rocCurveView = nullptr;

    return true;
}

CROCCurveView* CBoxAlgorithmROCCurve::getView() 
{
    return m_rocCurveView; 
}

bool CBoxAlgorithmROCCurve::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}


bool CBoxAlgorithmROCCurve::process()
{
    const Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();

    //First let's deal with the expected.
    for (size_t i = 0; i < boxCtx.getInputChunkCount(0); ++i) {
        m_expectedDecoder.decode(i);

        if (m_expectedDecoder.isHeaderReceived()) {
             m_stimTimeline.clear(); 
        }

        if (m_expectedDecoder.isBufferReceived()) {
            const CStimulationSet* stimSet = m_expectedDecoder.getOutputStimulationSet();
            for (size_t k = 0; k < stimSet->size(); ++k) {
                CIdentifier id = stimSet->getId(k);
                if (m_classStimSet.find(id) != m_classStimSet.end()) { m_stimTimeline.push_back(CTimestampLabelPair(stimSet->getDate(k), id.id())); }
                //We need to check if we receive the computation trigger
                if (id == m_computationTrigger) { computeROCCurves(); }
            }
        }
    }

    for (size_t i = 0; i < boxCtx.getInputChunkCount(1); ++i) {
        m_classificationDecoder.decode(i);
        if (m_classificationDecoder.isHeaderReceived()) { m_valueTimeline.clear(); }
        if (m_classificationDecoder.isBufferReceived()) {
            CMatrix* matrixValue = m_classificationDecoder.getOutputMatrix();
            //The matrix is suppose to have only one dimension
            double* arrayValue;

            if (matrixValue->getBufferElementCount() == 0) {
                this->getLogManager() << Kernel::LogLevel_Error << "Received zero-sized buffer\n";
                return false;
            }
            if (matrixValue->getBufferElementCount() > 1) {
                arrayValue = new double[matrixValue->getBufferElementCount()];
                for (size_t k = 0; k < matrixValue->getBufferElementCount(); ++k) { arrayValue[k] = matrixValue->getBuffer()[k]; }
            }
            else {
                arrayValue    = new double[2];
                arrayValue[0] = matrixValue->getBuffer()[0];
                arrayValue[1] = 1 - matrixValue->getBuffer()[0];
            }

            uint64_t timestamp = boxCtx.getInputChunkEndTime(1, i); //the time in stimulation correspond to the end of the chunck (cf processorbox code)
            m_valueTimeline.push_back(CTimestampValuesPair(timestamp, arrayValue));
        }
    }
    return true;
}

bool CBoxAlgorithmROCCurve::computeROCCurves()
{
    //Now we assiociate all values to the corresponding label
    std::sort(m_stimTimeline.begin(), m_stimTimeline.end(), compareCTimelineStimulationPair);//ensure the timeline is ok

    for (auto& val : m_valueTimeline) {
        auto bound = std::lower_bound(m_stimTimeline.begin(), m_stimTimeline.end(), val, compareValueAndStimulationTimelinePair);
        if (bound != m_stimTimeline.begin()) {
            --bound;
            m_labelValueList.push_back(CLabelValuesPair(bound->second, val.second));
        }
        else {
            //Impossible to find the corresponding stimulation
            this->getLogManager() << Kernel::LogLevel_Warning << "A result of classification cannot be connected to a class. The result will be discarded.\n";
        }
    }

    //We cannot use the set because we need the correct order
    for (size_t i = 2; i < this->getStaticBoxContext().getSettingCount(); ++i) {
        CIdentifier classLabel(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i));
        std::string className = std::string(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i));

        computeOneROCCurve(classLabel, className, i - 2);
    }

    //Now we ask to the current page to draw itself
    m_rocCurveView->launchQmlRedraw();
    return true;
}

void CBoxAlgorithmROCCurve::computeOneROCCurve(const CIdentifier& classID, std::string classeName, const size_t classIdx)
{
    std::vector<CRocPairValue> values;
    for (const auto& v : m_labelValueList) {
        CRocPairValue value;
        value.first  = v.first == classID.id();
        value.second = v.second[classIdx];
        values.push_back(value);
    }
    std::sort(values.begin(), values.end(), compareRocValuePair);

    size_t nTruePositive  = 0;
    size_t nFalsePositive = 0;

    size_t nPositive = std::count_if(values.begin(), values.end(), isPositive);
    size_t nNegative = values.size() - nPositive;
    if(nPositive == 0 || nNegative == 0) { 
        this->getLogManager() << Kernel::LogLevel_Warning << "Cannot compute ROC curve for class " << classeName 
                              << " because there is no positive(" << nPositive 
                              <<") or negative(" << nNegative << ") examples.\n";
        
        return; 
    }

    for(const auto& value : values) {
        value.first ? ++nTruePositive : ++nFalsePositive;
        m_rocCurveView->addPoint(classeName, double(nFalsePositive) / nNegative, double(nTruePositive) / nPositive);
    }
}

}  // namespace Evaluation
}  // namespace Plugins
}  // namespace OpenViBE