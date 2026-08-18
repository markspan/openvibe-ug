#pragma once

#include <openvibe/ov_all.h>

#include "TypeMatrix.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class TypeFeatureVector
 * \brief Abstact class defining chunk types for Feature Vector streams
 * \author J. T. Lindgren
 *
 */
class TypeFeatureVector
{
public:
    static CIdentifier getTypeIdentifier() { return OV_TypeId_FeatureVector; }

    class Header : public TypeMatrix::Header { };

    class Buffer : public TypeMatrix::Buffer { };

    class End : public TypeMatrix::End { };

    // Prevent constructing
    TypeFeatureVector()                         = delete;
    TypeFeatureVector(const TypeFeatureVector&) = delete;
    TypeFeatureVector(TypeFeatureVector&&)      = delete;
};
}  // namespace Tracker
}  // namespace OpenViBE
