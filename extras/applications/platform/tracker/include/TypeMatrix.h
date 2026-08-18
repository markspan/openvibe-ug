#pragma once

#include <openvibe/ov_all.h>

#include "Chunk.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class TypeMatrix
 * \brief Abstact class defining chunk types for Matrix streams
 * \author J. T. Lindgren
 *
 */
class TypeMatrix
{
public:
    static CIdentifier getTypeIdentifier() { return OV_TypeId_StreamedMatrix; }

    class Header : public Chunk
    {
    public:
        // Header
        CMatrix m_Header;

        // Make sure not copied until we have proper implementations 
        Header() { }
        Header& operator=(const Header& in) = delete;
        Header(const Header& in)            = delete;
    };

    class Buffer : public Chunk
    {
    public:
        // Payload
        CMatrix m_buffer;

        // Make sure not copied until we have proper implementations 
        Buffer() { }
        Buffer& operator=(const Buffer& in) = delete;
        Buffer(const Buffer& in)            = delete;
    };

    class End : public Chunk { };

    // Prevent constructing
    TypeMatrix()                  = delete;
    TypeMatrix(const TypeMatrix&) = delete;
    TypeMatrix(TypeMatrix&&)      = delete;
};
}  // namespace Tracker
}  // namespace OpenViBE
