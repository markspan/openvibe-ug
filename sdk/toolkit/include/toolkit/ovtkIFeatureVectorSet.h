#pragma once

#include "ovtk_base.h"

namespace OpenViBE {
namespace Toolkit {
class IFeatureVector;

class OVTK_API IFeatureVectorSet : public IObject
{
public:

    virtual size_t getFeatureVectorCount() const = 0;
    virtual bool setFeatureVectorCount(const size_t featureVector) = 0;
    virtual bool addFeatureVector(const IFeatureVector& featureVector) = 0;

    virtual IFeatureVector& getFeatureVector(const size_t index) = 0;
    virtual const IFeatureVector& getFeatureVector(const size_t index) const = 0;
    virtual size_t getLabelCount() const = 0;

    _IsDerivedFromClass_(IObject, OVTK_ClassId_FeatureVectorSet)

    const IFeatureVector& operator [](const size_t index) const { return this->getFeatureVector(index); }

    IFeatureVector& operator [](const size_t index) { return this->getFeatureVector(index); }
};
}  // namespace Toolkit
}  // namespace OpenViBE
