#pragma once

#include <openvibe/ov_all.h>

#include "TypeMatrix.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class TypeSpectrum
 * \brief Abstact class defining chunk types for Spectrum streams
 * \author J. T. Lindgren
 *
 */
class TypeSpectrum
{
public:
    static CIdentifier getTypeIdentifier() { return OV_TypeId_Spectrum; }

    class Header : public TypeMatrix::Header
    {
    public:
        uint64_t m_Sampling = 0;
        CMatrix m_Abscissas;

        // Make sure not copied until we have proper implementations 
        Header() { }
        Header& operator=(const Header& in) = delete;
        Header(const Header& in)            = delete;
    };

    class Buffer : public TypeMatrix::Buffer { };

    class End : public TypeMatrix::End { };

    // Prevent constructing
    TypeSpectrum()                    = delete;
    TypeSpectrum(const TypeSpectrum&) = delete;
    TypeSpectrum(TypeSpectrum&&)      = delete;
};
}  // namespace Tracker
}  // namespace OpenViBE
