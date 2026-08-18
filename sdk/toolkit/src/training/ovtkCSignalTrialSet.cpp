#include "ovtkCSignalTrialSet.hpp"

namespace OpenViBE {
namespace Toolkit {

bool CSignalTrialSet::addSignalTrial(ISignalTrial& rSignalTrial)
{
    m_signalTrials.push_back(&rSignalTrial);
    return true;
}

bool CSignalTrialSet::clear()
{
    m_signalTrials.clear();
    return true;
}

ISignalTrialSet* createSignalTrialSet() { return new CSignalTrialSet(); }
void releaseSignalTrialSet(ISignalTrialSet* signalTrialSet) { delete signalTrialSet; }

}  // namespace Toolkit
}  // namespace OpenViBE
