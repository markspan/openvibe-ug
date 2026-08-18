#pragma once

#include <openvibe/ov_all.h>

namespace OpenViBE {
namespace Tracker {

/**
 * \class Contexted 
 * \brief This class can be used to provide KernelContext and related getters for derived classes without overly complicating the derived class interface in their headers. 
 * \author J. T. Lindgren
 * @fixme actually this class does pretty much the same thing as Kernel::TKernelObject
 *
 */
class Contexted
{
public:
    explicit Contexted(const Kernel::IKernelContext& ctx) : m_kernelCtx(ctx) { }
    virtual ~Contexted() { }

    virtual const Kernel::IKernelContext& getKernelContext() const { return m_kernelCtx; }

    virtual Kernel::IAlgorithmManager& getAlgorithmManager() const { return m_kernelCtx.getAlgorithmManager(); }
    virtual Kernel::CErrorManager& getErrorManager() const { return m_kernelCtx.getErrorManager(); }
    virtual Kernel::ILogManager& getLogManager() const { return m_kernelCtx.getLogManager(); }
    virtual Kernel::ITypeManager& getTypeManager() const { return m_kernelCtx.getTypeManager(); }

    // Convenience handle to getLogManager()
    virtual Kernel::ILogManager& log() const { return getLogManager(); }

    Contexted() = delete;

    // Needed if Contexted is used as a base class for TAttributable
    // virtual bool isDerivedFromClass(const CIdentifier& classID) const { return false; }

protected:
    const Kernel::IKernelContext& m_kernelCtx;
};
}  // namespace Tracker
}  // namespace OpenViBE
