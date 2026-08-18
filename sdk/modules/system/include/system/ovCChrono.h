#pragma once

#include "defines.h"
#include <cstdlib>  // fix Unix compatibility

namespace System {
class System_API CChrono final
{
public:

    CChrono() { }
    ~CChrono();

    bool reset(size_t nStep);

    bool stepIn();
    bool stepOut();

    uint64_t getTotalStepInDuration() const;
    uint64_t getTotalStepOutDuration() const;
    uint64_t getAverageStepInDuration() const;
    uint64_t getAverageStepOutDuration() const;
    double getStepInPercentage() const;
    double getStepOutPercentage() const;

    bool hasNewEstimation() const { return m_hasNewEstimation; }

private:

    uint64_t* m_stepInTime  = nullptr;
    uint64_t* m_stepOutTime = nullptr;
    size_t m_nStep          = 0;
    size_t m_stepIdx        = 0;
    bool m_isInStep         = false;
    bool m_hasNewEstimation = false;

    uint64_t m_totalStepInTime  = 0;
    uint64_t m_totalStepOutTime = 0;
};
}  // namespace System
