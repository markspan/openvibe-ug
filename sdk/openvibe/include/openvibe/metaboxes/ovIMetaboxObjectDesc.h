#pragma once

#include "../plugins/ovIBoxAlgorithmDesc.h"

namespace OpenViBE {
namespace Metabox {
class OV_API IMetaboxObjectDesc : virtual public Plugins::IBoxAlgorithmDesc
{
public:
    virtual CString getMetaboxDescriptor() const = 0;

    _IsDerivedFromClass_(Plugins::IBoxAlgorithmDesc, OV_ClassId_Metaboxes_MetaboxObjectDesc)
};
}  // namespace Metabox
}  // namespace OpenViBE
