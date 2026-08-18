#include "system/ovCChrono.h"
#include "system/ovCTime.h"

namespace System {

CChrono::~CChrono()
{
    delete [] m_stepInTime;
    delete [] m_stepOutTime;
}

bool CChrono::reset(const size_t nStep)
{
    if (!nStep) { return false; }

    uint64_t* stepInTime  = new uint64_t[nStep + 1];
    uint64_t* stepOutTime = new uint64_t[nStep + 1];
    if (!stepInTime || !stepOutTime)
    {
        delete [] stepInTime;
        delete [] stepOutTime;
        return false;
    }

    for (size_t i = 0; i <= nStep; ++i)
    {
        stepInTime[i]  = 0;
        stepOutTime[i] = 0;
    }

    delete [] m_stepInTime;
    delete [] m_stepOutTime;
    m_stepInTime  = stepInTime;
    m_stepOutTime = stepOutTime;

    m_nStep            = nStep;
    m_stepIdx          = 0;
    m_isInStep         = false;
    m_hasNewEstimation = false;

    m_totalStepInTime  = 0;
    m_totalStepOutTime = 0;

    return true;
}

bool CChrono::stepIn()
{
    if (m_isInStep || !m_nStep) { return false; }

    m_isInStep = !m_isInStep;

    m_stepInTime[m_stepIdx] = Time::zgetTime();
    if (m_stepIdx == m_nStep)
    {
        m_totalStepInTime  = 0;
        m_totalStepOutTime = 0;
        for (size_t i = 0; i < m_nStep; ++i)
        {
            m_totalStepInTime += m_stepOutTime[i] - m_stepInTime[i];
            m_totalStepOutTime += m_stepInTime[i + 1] - m_stepOutTime[i];
        }
        m_stepInTime[0]    = m_stepInTime[m_nStep];
        m_stepIdx          = 0;
        m_hasNewEstimation = true;
    }
    else { m_hasNewEstimation = false; }

    return true;
}

bool CChrono::stepOut()
{
    if (!m_isInStep || !m_nStep) { return false; }

    m_isInStep = !m_isInStep;

    m_stepOutTime[m_stepIdx] = Time::zgetTime();
    m_stepIdx++;

    return true;
}

uint64_t CChrono::getTotalStepInDuration() const { return m_totalStepInTime; }
uint64_t CChrono::getTotalStepOutDuration() const { return m_totalStepOutTime; }
uint64_t CChrono::getAverageStepInDuration() const { return m_nStep ? this->getTotalStepInDuration() / m_nStep : 0; }
uint64_t CChrono::getAverageStepOutDuration() const { return m_nStep ? this->getTotalStepOutDuration() / m_nStep : 0; }

double CChrono::getStepInPercentage() const
{
    const uint64_t totalStepDuration = (this->getTotalStepInDuration() + this->getTotalStepOutDuration());
    return totalStepDuration ? (this->getTotalStepInDuration() * 100.0) / totalStepDuration : 0;
}

double CChrono::getStepOutPercentage() const
{
    const uint64_t totalStepDuration = (this->getTotalStepOutDuration() + this->getTotalStepInDuration());
    return totalStepDuration ? (this->getTotalStepOutDuration() * 100.0) / totalStepDuration : 0;
}

}  // namespace System
