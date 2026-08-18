///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmClassifierLDA.hpp
/// \brief Classes for the Algorithm LDA.
/// \author Jussi T. Lindgren (Inria) / Guillaume Serrière (Inria).
/// \version 2.0.
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

#include "../defines.hpp"
#include "CAlgorithmLDADiscriminantFunction.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <xml/IXMLNode.h>

#include <Eigen/Dense>

namespace OpenViBE {
namespace Plugins {
namespace Classification {
class CAlgorithmLDADiscriminantFunction;

int LDAClassificationCompare(CMatrix& first, CMatrix& second);

typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> MatrixXdRowMajor;

class CAlgorithmClassifierLDA final : public Toolkit::CAlgorithmClassifier
{
public:
    bool initialize() override;
    bool uninitialize() override;
    bool train(const Toolkit::IFeatureVectorSet& dataset) override;
    bool classify(const Toolkit::IFeatureVector& sample, double& classId, Toolkit::IVector& distance, Toolkit::IVector& probability) override;
    XML::IXMLNode* saveConfig() override;
    bool loadConfig(XML::IXMLNode* node) override;
    size_t getNProbabilities() override { return m_discriminantFunctions.size(); }
    size_t getNDistances() override { return m_discriminantFunctions.size(); }

    _IsDerivedFromClass_Final_(CAlgorithmClassifier, Algorithm_ClassifierLDA)

protected:
    // Debug method. Prints the matrix to the logManager. May be disabled in implementation.
    static void dumpMatrix(Kernel::ILogManager& pMgr, const MatrixXdRowMajor& mat, const CString& desc);

    std::vector<double> m_labels;
    std::vector<CAlgorithmLDADiscriminantFunction> m_discriminantFunctions;

    Eigen::MatrixXd m_coefficients;
    Eigen::MatrixXd m_weights;
    double m_biasDistance = 0;
    double m_w0           = 0;

    size_t m_nCols    = 0;
    size_t m_nClasses = 0;

    Kernel::IAlgorithmProxy* m_covAlgorithm = nullptr;

private:
    void loadClassesFromNode(const XML::IXMLNode* node);
    void loadCoefsFromNode(const XML::IXMLNode* node);

    size_t getClassCount() const { return m_nClasses; }
};

class CAlgorithmClassifierLDADesc final : public Toolkit::CAlgorithmClassifierDesc
{
public:
    void release() override { }

    CString getName() const override { return "LDA Classifier"; }
    CString getAuthorName() const override { return "Jussi T. Lindgren / Guillaume Serrière"; }
    CString getAuthorCompanyName() const override { return "Inria / Loria"; }
    CString getShortDescription() const override { return "Estimates LDA using regularized or classic covariances"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return ""; }
    CString getVersion() const override { return "2.0"; }

    CIdentifier getCreatedClass() const override { return Algorithm_ClassifierLDA; }
    IPluginObject* create() override { return new CAlgorithmClassifierLDA; }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(ClassifierLDA_InputParameterId_UseShrinkage, "Use shrinkage", Kernel::ParameterType_Boolean);
        prototype.addInputParameter(ClassifierLDA_InputParameterId_DiagonalCov, "Shrinkage: Force diagonal cov (DDA)", Kernel::ParameterType_Boolean);
        prototype.addInputParameter(ClassifierLDA_InputParameterId_Shrinkage, "Shrinkage coefficient (-1 == auto)", Kernel::ParameterType_Float);

        CAlgorithmClassifierDesc::getAlgorithmPrototype(prototype);
        return true;
    }

    _IsDerivedFromClass_Final_(CAlgorithmClassifierDesc, Algorithm_ClassifierLDADesc)
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
