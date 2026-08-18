#pragma once

#include <openvibe/ov_all.h>

#include "TypeMatrix.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class TypeChannelUnits
 * \brief Abstact class defining chunk types for Channel Units streams
 * \author J. T. Lindgren
 *
 */
class TypeChannelUnits
{
public:
    static CIdentifier getTypeIdentifier() { return OV_TypeId_ChannelUnits; }

    class Header : public TypeMatrix::Header
    {
    public:
        bool m_Dynamic = false;
    };

    class Buffer : public TypeMatrix::Buffer { };   // Payload

    class End : public TypeMatrix::End { };

    // Prevent constructing
    TypeChannelUnits()                        = delete;
    TypeChannelUnits(const TypeChannelUnits&) = delete;
    TypeChannelUnits(TypeChannelUnits&&)      = delete;
};
}  // namespace Tracker
}  // namespace OpenViBE
