#pragma once

#include "ovCObservable.h"
#include "ov_defines.h"

namespace OpenViBE {
class CObservable;

/**
 * \class IObserver
 * \author Guillaume Serrière (Inria/Loria)
 * \date 2014-11-7
 * \brief OpenViBE Observer interface
 *
 * This interface is used in the Observer/Observable pattern
 * implementation in OpenViBE.
 */
class IObserver
{
public:
    virtual ~IObserver() = default;

    /**
     * \brief Function called by the observed object when a notification is done.
     * \param o [in] : the object which do the notification.
     * \param data [in] : a pointer to data give by the observed object during the notification.
     */
    virtual void update(CObservable& o, void* data) = 0;
};
}  // namespace OpenViBE
