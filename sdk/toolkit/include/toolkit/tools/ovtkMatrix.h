#pragma once

#include "../ovtk_base.h"

namespace OpenViBE {
namespace Toolkit {
namespace Matrix {
// Can be used : CMatrix class behaviour is a little different but this function is never used (except in test). Avoid to used it, intended to be removed. 
OVTK_API bool toString(const CMatrix& matrix, CString& str, const size_t precision = 6);
OVTK_API bool fromString(CMatrix& matrix, const CString& str);
OVTK_API bool saveToTextFile(const CMatrix& matrix, const CString& filename, const size_t precision = 6);
OVTK_API bool loadFromTextFile(CMatrix& matrix, const CString& filename);

//@todo All is useless now. Avoid to used it, intended to be removed.  
/// \deprecated Use the CMatrix class copy() method instead.
OV_Deprecated("Use the CMatrix class copy() method instead.")
OVTK_API bool copy(CMatrix& dst, const CMatrix& src);
/// \deprecated Use the CMatrix class copyDescription() method instead.
OV_Deprecated("Use the CMatrix class copyDescription() method instead.")
OVTK_API bool copyDescription(CMatrix& dst, const CMatrix& src);
/// \deprecated Use the CMatrix class copyContent() method instead.
OV_Deprecated("Use the CMatrix class copyContent() method instead.")
OVTK_API bool copyContent(CMatrix& dst, const CMatrix& src);
/// \deprecated Use the CMatrix class resetBuffer() method instead.
OV_Deprecated("Use the CMatrix class resetBuffer() method instead.")
OVTK_API bool clearContent(CMatrix& matrix);                                                            //(resetBuffer)
/// \deprecated Use the CMatrix class isDescriptionEqual() method instead.
OV_Deprecated("Use the CMatrix class isDescriptionEqual() method instead.")
OVTK_API bool isDescriptionSimilar(const CMatrix& src1, const CMatrix& src2, bool checkLabels = true);  //(isDescriptionEqual)
/// \deprecated Use the CMatrix class isBufferEqual() method instead.
OV_Deprecated("Use the CMatrix class isBufferEqual() method instead.")
OVTK_API bool isContentSimilar(const CMatrix& src1, const CMatrix& src2);                               //(isBufferEqual)
/// \deprecated Use the CMatrix class isBufferValid() method instead.
OV_Deprecated("Use the CMatrix class isBufferValid() method instead.")
OVTK_API bool isContentValid(const CMatrix& src, const bool checkNotANumber = true, const bool checkInfinity = true);   //(isBufferValid)
}  // namespace Matrix

//@todo All is useless now. Avoid to used it, intended to be removed. 
namespace MatrixManipulation {
/// \deprecated Use the CMatrix class copy() method instead.
OV_Deprecated("Use the CMatrix class copy() method instead.")
inline bool copy(CMatrix& dst, const CMatrix& src);
/// \deprecated Use the CMatrix class copyDescription() method instead.
OV_Deprecated("Use the CMatrix class copyDescription() method instead.")
inline bool copyDescription(CMatrix& dst, const CMatrix& src);
/// \deprecated Use the CMatrix class copyContent() method instead.
OV_Deprecated("Use the CMatrix class copyContent() method instead.")
inline bool copyContent(CMatrix& dst, const CMatrix& src);
/// \deprecated Use the CMatrix class clearContent() method instead.
OV_Deprecated("Use the CMatrix class clearContent() method instead.")
inline bool clearContent(CMatrix& matrix);
}  // namespace MatrixManipulation
}  // namespace Toolkit
}  // namespace OpenViBE
