#include "ovtkCFeatureVectorSet.hpp"

namespace OpenViBE {
namespace Toolkit {

CFeatureVectorSet::CFeatureVectorSet(const CMatrix& matrix) : m_matrix(matrix)
{
    if (matrix.getDimensionCount() != 2) { throw std::runtime_error("Fetaure vector set matrix must be 2 dimensions"); }

    for (size_t i = 0; i < matrix.getDimensionSize(0); ++i)
    {
        m_features[i].m_Matrix       = &matrix;
        m_features[i].m_DimensionIdx = i;
        m_features[i].m_Size         = matrix.getDimensionSize(1) - 1;
        m_features[i].m_Buffer       = matrix.getBuffer() + i * matrix.getDimensionSize(1);
    }
}

IFeatureVector& CFeatureVectorSet::getFeatureVector(const size_t index) { return m_features.find(index)->second; }

const IFeatureVector& CFeatureVectorSet::getFeatureVector(const size_t index) const { return m_features.find(index)->second; }

size_t CFeatureVectorSet::getLabelCount() const
{
    std::map<double, bool> labels;
    for (auto it = m_features.begin(); it != m_features.end(); ++it) { labels[it->second.getLabel()] = true; }
    return labels.size();
}

}  // namespace Toolkit
}  // namespace OpenViBE
