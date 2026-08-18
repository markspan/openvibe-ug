#include "ovkCObjectVisitorContext.h"

namespace OpenViBE {
namespace Kernel {

CObjectVisitorContext::CObjectVisitorContext(const IKernelContext& ctx) : TKernelObject<IObjectVisitorContext>(ctx) {}
CObjectVisitorContext::~CObjectVisitorContext() {}

IAlgorithmManager& CObjectVisitorContext::getAlgorithmManager() const { return TKernelObject<IObjectVisitorContext>::getAlgorithmManager(); }
IConfigurationManager& CObjectVisitorContext::getConfigurationManager() const { return TKernelObject<IObjectVisitorContext>::getConfigurationManager(); }
ITypeManager& CObjectVisitorContext::getTypeManager() const { return TKernelObject<IObjectVisitorContext>::getTypeManager(); }
ILogManager& CObjectVisitorContext::getLogManager() const { return TKernelObject<IObjectVisitorContext>::getLogManager(); }
CErrorManager& CObjectVisitorContext::getErrorManager() const { return TKernelObject<IObjectVisitorContext>::getErrorManager(); }

}  // namespace Kernel
}  // namespace OpenViBE
