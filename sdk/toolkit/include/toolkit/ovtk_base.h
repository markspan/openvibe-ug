#pragma once

#include "ovtk_defines.h"

#include <openvibe/ov_all.h>

namespace EBML {
class IWriter;
class IWriterCallback;
class IWriterHelper;
class IReader;
class IReaderCallback;
class IReaderHelper;
}  // namespace EBML

namespace OpenViBE {
namespace Toolkit {
template <class THandledType>
class TScopeHandle
{
public:

    TScopeHandle(THandledType& rHandler, THandledType& rHandledValue) : m_handler(rHandler), m_lastHandledValue(rHandler) { m_handler = rHandledValue; }
    ~TScopeHandle() { m_handler = m_lastHandledValue; }

private:

    THandledType& m_handler;
    THandledType m_lastHandledValue;
};
}  // namespace Toolkit
}  // namespace OpenViBE
