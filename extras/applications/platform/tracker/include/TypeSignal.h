#pragma once

#include <openvibe/ov_all.h>

#include "TypeMatrix.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class TypeSignal
 * \brief Abstact class defining chunk types for Signal streams
 * \author J. T. Lindgren
 *
 */
class TypeSignal
{
public:
    static CIdentifier getTypeIdentifier() { return OV_TypeId_Signal; }

    class Header : public TypeMatrix::Header
    {
    public:
        // Header
        uint64_t m_Sampling = 0;
    };

    class Buffer : public TypeMatrix::Buffer { };

    class End : public TypeMatrix::End { };

    // Prevent constructing
    TypeSignal()                  = delete;
    TypeSignal(const TypeSignal&) = delete;
    TypeSignal(TypeSignal&&)      = delete;

    //  class Decoder : public Toolkit::TSignalDecoder<BoxAlgorithmProxy> { };
};
}  // namespace Tracker
}  // namespace OpenViBE
