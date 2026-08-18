#pragma once

#include <openvibe/ov_all.h>

#include "TypeMatrix.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class TypeChannelLocalization
 * \brief Abstact class defining chunk types for Channel Localization streams
 * \author J. T. Lindgren
 *
 */
class TypeChannelLocalization
{
public:
    static CIdentifier getTypeIdentifier() { return OV_TypeId_ChannelLocalisation; }

    class Header : public TypeMatrix::Header
    {
    public:
        bool m_Dynamic = false;
    };

    class Buffer : public TypeMatrix::Buffer { }; // Payload

    class End : public TypeMatrix::End { };

    // Prevent constructing
    TypeChannelLocalization()                               = delete;
    TypeChannelLocalization(const TypeChannelLocalization&) = delete;
    TypeChannelLocalization(TypeChannelLocalization&&)      = delete;
};
}  // namespace Tracker
}  // namespace OpenViBE
