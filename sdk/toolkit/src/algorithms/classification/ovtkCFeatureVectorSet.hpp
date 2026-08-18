#pragma once

#include "../../ovtkIFeatureVector.h"
#include "../../ovtkIFeatureVectorSet.h"

#include <map>

namespace OpenViBE {
namespace Toolkit {
class CInternalFeatureVector final : public IFeatureVector
{
public:

    CInternalFeatureVector() { }
    size_t getSize() const override { return m_Size; }
    bool setSize(const size_t /*size*/) override { return false; }
    double* getBuffer() override { return nullptr; }
    const double* getBuffer() const override { return m_Buffer; }
    const char* getElementLabel(const size_t index) const override { return m_Matrix->getDimensionLabel(m_DimensionIdx, index); }
    bool setElementLabel(const size_t /*index*/, const char* /*elementLabel*/) override { return false; }
    double getLabel() const override { return m_Buffer[m_Size]; }
    bool setLabel(const double /*label*/) override { return false; }

    _IsDerivedFromClass_Final_(IFeatureVector, CIdentifier::undefined())

    const CMatrix* m_Matrix = nullptr;
    size_t m_DimensionIdx = 0;
    size_t m_Size         = 0;
    const double* m_Buffer  = nullptr;
};

class CFeatureVectorSet final : public IFeatureVectorSet
{
public:

    explicit CFeatureVectorSet(const CMatrix& matrix);
    size_t getFeatureVectorCount() const override { return m_matrix.getDimensionSize(0); }
    bool setFeatureVectorCount(const size_t /*nFeatureVector*/) override { return false; }
    bool addFeatureVector(const IFeatureVector& /*featureVector*/) override { return false; }
    IFeatureVector& getFeatureVector(const size_t index) override;
    const IFeatureVector& getFeatureVector(const size_t index) const override;
    size_t getLabelCount() const override;

    _IsDerivedFromClass_Final_(IFeatureVectorSet, CIdentifier::undefined())

protected:

    const CMatrix& m_matrix;
    std::map<size_t, CInternalFeatureVector> m_features;
};
}  // namespace Toolkit
}  // namespace OpenViBE
