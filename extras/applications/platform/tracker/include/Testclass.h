#pragma once

#include <openvibe/ov_all.h>

namespace OpenViBE {
namespace Tracker {

/**
 * \class TestClass 
 * \brief Class to try out stuff in the Tracker
 * \author J. T. Lindgren
 *
 */
class TestClass
{
public:
    explicit TestClass(Kernel::IKernelContext& ctx);

    Kernel::IKernelContext& m_Ctx;
};
}  // namespace Tracker
}  // namespace OpenViBE
