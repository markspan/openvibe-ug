#pragma once

#include <openvibe/ov_all.h>

#include <map>

template <class T>
OpenViBE::CIdentifier getNextIdentifier(const std::map<OpenViBE::CIdentifier, T>& map, const OpenViBE::CIdentifier& previousID)
{
    typename std::map<OpenViBE::CIdentifier, T>::const_iterator it;

    if (previousID == OpenViBE::CIdentifier::undefined()) { it = map.begin(); }
    else
    {
        it = map.find(previousID);
        if (it == map.end()) { return OpenViBE::CIdentifier::undefined(); }
        ++it;
    }

    return it != map.end() ? it->first : OpenViBE::CIdentifier::undefined();
}
