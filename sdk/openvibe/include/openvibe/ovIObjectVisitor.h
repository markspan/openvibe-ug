#pragma once

#include "ovIObject.h"

namespace OpenViBE {
namespace Kernel {
class IObjectVisitorContext;

class IBox;
class IComment;
class IMetadata;
class ILink;
class IScenario;
}  // namespace kernel

class OV_API IObjectVisitor : public IObject
{
public:

    virtual bool processBegin(Kernel::IObjectVisitorContext& /*visitorCtx*/, Kernel::IBox& /*box*/) { return true; }
    virtual bool processBegin(Kernel::IObjectVisitorContext& /*visitorCtx*/, Kernel::IComment& /*comment*/) { return true; }
    virtual bool processBegin(Kernel::IObjectVisitorContext& /*visitorCtx*/, Kernel::IMetadata& /*metadata*/) { return true; }
    virtual bool processBegin(Kernel::IObjectVisitorContext& /*visitorCtx*/, Kernel::ILink& /*link*/) { return true; }
    virtual bool processBegin(Kernel::IObjectVisitorContext& /*visitorCtx*/, Kernel::IScenario& /*scenario*/) { return true; }

    virtual bool processEnd(Kernel::IObjectVisitorContext& /*visitorCtx*/, Kernel::IBox& /*box*/) { return true; }
    virtual bool processEnd(Kernel::IObjectVisitorContext& /*visitorCtx*/, Kernel::IComment& /*comment*/) { return true; }
    virtual bool processEnd(Kernel::IObjectVisitorContext& /*visitorCtx*/, Kernel::IMetadata& /*metadata*/) { return true; }
    virtual bool processEnd(Kernel::IObjectVisitorContext& /*visitorCtx*/, Kernel::ILink& /*link*/) { return true; }
    virtual bool processEnd(Kernel::IObjectVisitorContext& /*visitorCtx*/, Kernel::IScenario& /*scenario*/) { return true; }

    _IsDerivedFromClass_(IObject, OV_ClassId_ObjectVisitor)
};
}  // namespace OpenViBE
