///-------------------------------------------------------------------------------------------------
/// 
/// \file CPolyBox.hpp
/// \brief Class CPolyBox
/// \author Thibaut Monseigne (Inria) & Jimmy Leblanc (Polymont) & Yannis Bendi-Ouis (Polymont) 
/// \version 1.0.
/// \date 12/03/2020.
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

#pragma once

// Windows debug build doesn't typically link as most people don't have the python debug library.
#if !(defined(WIN32) && defined(TARGET_BUILDTYPE_Debug))

#include <Python.h>

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace PyBox {
/// <summary>   Enumeration of Adaptation Methods for classifier. </summary>
enum class EClassifier
{
    NearestCentroid, NearestNeighbors, GaussianNaiveBayes, StochasticGradientDescent, LogisticRegression, DecisionTree, ExtraTrees,
    Bagging, RandomForest, SVM, LDA, AdaBoost, MultiLayerPerceptron, MDM, TangentSpace, None
};

/// <summary>   Convert classifier to string.</summary>
/// <param name="type"> The type of classifier.</param>
/// <returns>   std::string </returns>
inline std::string toString(const EClassifier type)
{
    switch (type) {
        case EClassifier::None: return "None";
        case EClassifier::NearestCentroid: return "Nearest Centroid";
        case EClassifier::NearestNeighbors: return "Nearest Neighbors";
        case EClassifier::GaussianNaiveBayes: return "Gaussian Naive Bayes";
        case EClassifier::StochasticGradientDescent: return "Stochastic Gradient Descent";
        case EClassifier::LogisticRegression: return "Logistic Regression";
        case EClassifier::DecisionTree: return "Decision Tree";
        case EClassifier::ExtraTrees: return "Extra Trees";
        case EClassifier::Bagging: return "Bagging";
        case EClassifier::RandomForest: return "Random Forest";
        case EClassifier::SVM: return "Support Vector Machine";
        case EClassifier::LDA: return "Linear Discriminant Analysis";
        case EClassifier::AdaBoost: return "AdaBoost";
        case EClassifier::MultiLayerPerceptron: return "Multi Layer Perceptron";
        case EClassifier::MDM: return "Riemann Minimum Distance to Mean";
        case EClassifier::TangentSpace: return "Riemann Tangent Space";
        default: return "Invalid";
    }
}

class CPolyBox : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    uint64_t getClockFrequency() override { return m_clockFrequency << 32; }
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool processInput(size_t index) override;
    bool process() override;

protected:
    uint64_t m_clockFrequency = 0;
    CString m_script;

    std::vector<Toolkit::TDecoder<CPolyBox>*> m_decoders;
    std::vector<Toolkit::TEncoder<CPolyBox>*> m_encoders;

    // These are all borrowed references in python v2.7. Do not free them.
    static bool m_isInitialized;

    static PyObject *m_mainModule, *m_mainDictionnary;
    static PyObject *m_matrixHeader, *m_matrixBuffer, *m_matrixEnd;
    static PyObject *m_signalHeader, *m_signalBuffer, *m_signalEnd;
    static PyObject *m_stimulationHeader, *m_stimulation, *m_stimulationSet, *m_stimulationEnd;
    static PyObject* m_buffer;
    static PyObject* m_execFileFunction;
    static PyObject *m_stdout, *m_stderr;


    //std::map<char,PyObject *> m_PyObjectMap;
    PyObject *m_box            = nullptr, *m_boxInput   = nullptr, *m_boxOutput       = nullptr, *m_boxSetting = nullptr, *m_boxTime = nullptr;
    PyObject *m_boxInitialize  = nullptr, *m_boxProcess = nullptr, *m_boxUninitialize = nullptr;
    bool m_initializeSucceeded = false;


    bool logSysStd(const bool out);
    bool logSysStdout() { return logSysStd(true); }
    bool logSysStderr() { return logSysStd(false); }
    void buildPythonSettings();

    bool initializePythonSafely();
    bool transferStreamedMatrixInputChunksToPython(const size_t index);
    bool transferStreamedMatrixOutputChunksFromPython(const size_t index);
    bool transferSignalInputChunksToPython(const size_t index);
    bool transferSignalOutputChunksFromPython(const size_t index);
    bool transferStimulationInputChunksToPython(const size_t index);
    bool transferStimulationOutputChunksFromPython(const size_t index);
};
}  // namespace PyBox
}  // namespace Plugins
}  // namespace OpenViBE

#endif // !(defined(WIN32) && defined(TARGET_BUILDTYPE_Debug))
