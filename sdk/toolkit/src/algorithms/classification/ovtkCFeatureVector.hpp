#pragma once

#include "ovtkCVector.hpp"
#include "../../ovtkIFeatureVector.h"

namespace OpenViBE {
namespace Toolkit {
template <class TParent>
class TFeatureVector final : public TVector<TParent>
{
public:

    explicit TFeatureVector(CMatrix& rMatrix) : TVector<TParent>(rMatrix) { }

    double getLabel() const override { return 0; }
    bool setLabel(const double /*label*/) override { return false; }

    _IsDerivedFromClass_Final_(TVector<TParent>, CIdentifier::undefined())
};

typedef TFeatureVector<IFeatureVector> CFeatureVector;
}  // namespace Toolkit
}  // namespace OpenViBE
