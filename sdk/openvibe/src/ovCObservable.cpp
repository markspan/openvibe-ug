#include "ovIObserver.h"
#include "ovCObservable.h"

#include "ov_defines.h"
#include <iostream>
#include <vector>

namespace OpenViBE {

struct CObservable::SObserverList
{
    std::vector<IObserver*> m_Vector;
};

CObservable::CObservable() { m_observers = new SObserverList(); }
CObservable::~CObservable() { delete m_observers; }

void CObservable::addObserver(IObserver* o) { m_observers->m_Vector.push_back(o); }

void CObservable::deleteObserver(IObserver* o)
{
    for (auto it = m_observers->m_Vector.begin(); it != m_observers->m_Vector.end(); ++it)
    {
        if ((*it) == o)
        {
            m_observers->m_Vector.erase(it);
            //We only suppress the first occurence, no need to continue
            return;
        }
    }
}

void CObservable::notifyObservers(void* data)
{
    if (m_hasChanged)
    {
        for (auto it = m_observers->m_Vector.begin(); it != m_observers->m_Vector.end(); ++it) { static_cast<IObserver*>(*it)->update(*this, data); }
        m_hasChanged = false;
    }
}

}  // namespace OpenViBE
