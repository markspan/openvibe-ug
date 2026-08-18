#pragma once

#include "ovtkISignalTrialSet.h"

#include <vector>

namespace OpenViBE {
namespace Toolkit {
class CSignalTrialSet final : public ISignalTrialSet
{
public:
    bool addSignalTrial(ISignalTrial& rSignalTrial) override;
    bool clear() override;
    size_t getSignalTrialCount() const override { return m_signalTrials.size(); }
    ISignalTrial& getSignalTrial(const size_t index) const override { return *m_signalTrials[index]; }

    _IsDerivedFromClass_Final_(ISignalTrialSet, OVTK_ClassId_)

protected:

    mutable std::vector<ISignalTrial*> m_signalTrials;
};

extern OVTK_API ISignalTrialSet* createSignalTrialSet();
}  // namespace Toolkit
}  // namespace OpenViBE
