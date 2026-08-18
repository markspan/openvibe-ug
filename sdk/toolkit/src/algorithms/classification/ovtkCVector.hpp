#pragma once

#include "../../ovtk_base.h"
#include "../../ovtkIVector.h"

namespace OpenViBE {
namespace Toolkit {
template <class TParent>
class TVector : public TParent
{
public:

    explicit TVector(CMatrix& matrix) : m_matrix(matrix) { }

    size_t getSize() const override { return m_matrix.getBufferElementCount(); }

    bool setSize(const size_t size) override
    {
        m_matrix.resize(size);
        return true;
    }

    double* getBuffer() override { return m_matrix.getBuffer(); }
    const double* getBuffer() const override { return m_matrix.getBuffer(); }
    const char* getElementLabel(const size_t index) const override { return m_matrix.getDimensionLabel(0, index); }

    bool setElementLabel(const size_t index, const char* label) override
    {
        m_matrix.setDimensionLabel(0, index, label);
        return true;
    }

    _IsDerivedFromClass_Final_(TParent, CIdentifier::undefined())

protected:

    CMatrix& m_matrix;
};

typedef TVector<IVector> CVector;
}  // namespace Toolkit
}  // namespace OpenViBE
