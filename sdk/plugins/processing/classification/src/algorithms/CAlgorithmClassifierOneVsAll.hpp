///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmClassifierOneVsAll.hpp
/// \brief Classes for the Algorithm One Vs All.
/// \author Guillaume Serriere (Inria).
/// \version 0.1.
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
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <xml/IXMLNode.h>

#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace Classification {
class CAlgorithmClassifierOneVsAll final : public Toolkit::CAlgorithmPairingStrategy
{
public:
    bool initialize() override;
    bool uninitialize() override;
    bool train(const Toolkit::IFeatureVectorSet& dataset) override;
    bool classify(const Toolkit::IFeatureVector& sample, double& classId, Toolkit::IVector& distance, Toolkit::IVector& probability) override;
    bool designArchitecture(const CIdentifier& id, const size_t nClass) override;
    XML::IXMLNode* saveConfig() override;
    bool loadConfig(XML::IXMLNode* configNode) override;
    size_t getNProbabilities() override { return m_subClassifiers.size(); }
    size_t getNDistances() override;

    _IsDerivedFromClass_Final_(Toolkit::CAlgorithmPairingStrategy, Algorithm_ClassifierOneVsAll)


private:
    static XML::IXMLNode* getClassifierConfig(Kernel::IAlgorithmProxy* classifier);
    bool addNewClassifierAtBack();
    void removeClassifierAtBack();
    bool setSubClassifierIdentifier(const CIdentifier& id);
    size_t getClassCount() const { return m_subClassifiers.size(); }

    bool loadSubClassifierConfig(const XML::IXMLNode* node);

    std::vector<Kernel::IAlgorithmProxy*> m_subClassifiers;
    fClassifierComparison m_fAlgorithmComparison = nullptr;
};

class CAlgorithmClassifierOneVsAllDesc final : public Toolkit::CAlgorithmPairingStrategyDesc
{
public:
    void release() override { }

    CString getName() const override { return "OneVsAll pairing classifier"; }
    CString getAuthorName() const override { return "Guillaume Serriere"; }
    CString getAuthorCompanyName() const override { return "INRIA/Loria"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return ""; }
    CString getVersion() const override { return "0.1"; }

    CIdentifier getCreatedClass() const override { return Algorithm_ClassifierOneVsAll; }
    IPluginObject* create() override { return new CAlgorithmClassifierOneVsAll; }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CAlgorithmPairingStrategyDesc::getAlgorithmPrototype(prototype);
        return true;
    }

    _IsDerivedFromClass_Final_(CAlgorithmPairingStrategyDesc, Algorithm_ClassifierOneVsAllDesc)
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
