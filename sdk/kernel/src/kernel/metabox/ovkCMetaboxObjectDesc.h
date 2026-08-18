#pragma once

#include "../ovkTKernelObject.h"

#include <vector>

namespace OpenViBE {
namespace Metabox {

/**
 * \brief The CMetaboxObjectDesc virtual BoxAlgorithmDesc for metaboxes
 *
 * This class provides a virtual algorithm descriptor for metaboxes. Each metabox-scenario
 * will result in one of these descriptors. The prototype is created from scenario inputs,
 * outputs and settings.
 *
 * Variables such as name, author etc are pulled from scenario information.
 */
class CMetaboxObjectDesc final : virtual public IMetaboxObjectDesc
{
public:
    CMetaboxObjectDesc() { }

    CMetaboxObjectDesc(const CString& rMetaboxDescriptor, Kernel::IScenario& metaboxScenario);
    void release() override { }
    CString getMetaboxDescriptor() const override { return m_metaboxDesc; }
    CString getName() const override { return m_name; }
    CString getAuthorName() const override { return m_authorName; }
    CString getAuthorCompanyName() const override { return m_authorCompanyName; }
    CString getShortDescription() const override { return m_shortDesc; }
    CString getDetailedDescription() const override { return m_detailedDesc; }
    CString getCategory() const override { return m_category; }
    CString getVersion() const override { return m_version; }
    CString getStockItemName() const override { return m_stockItemName; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_Metabox; }
    Plugins::IPluginObject* create() override { return nullptr; }

    // Handling of the virtual prototype

    // Since we have to construct a prototype on the fly, the special Metabox descriptor
    // will also hold the information about the settings, inputs and outputs of the box
    typedef struct SStream
    {
        SStream() : m_name(""), m_typeID(CIdentifier::undefined()), m_id(CIdentifier::undefined()) {}

        SStream(const CString& name, const CIdentifier& typeID, const CIdentifier& identifier)
            : m_name(name), m_typeID(typeID), m_id(identifier) {}

        CString m_name;
        CIdentifier m_typeID = CIdentifier::undefined();
        CIdentifier m_id     = CIdentifier::undefined();
    } io_stream_t;

    typedef struct SSetting
    {
        SSetting()
            : m_name(""), m_typeID(CIdentifier::undefined()), m_defaultValue(""), m_id(CIdentifier::undefined()) {}

        SSetting(const CString& name, const CIdentifier& typeID, const CString& value, const CIdentifier& id)
            : m_name(name), m_typeID(typeID), m_defaultValue(value), m_id(id) { }

        CString m_name;
        CIdentifier m_typeID = CIdentifier::undefined();
        CString m_defaultValue;
        CIdentifier m_id = CIdentifier::undefined();
    } setting_t;

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override;

    _IsDerivedFromClass_Final_(IMetaboxObjectDesc, OVP_ClassId_BoxAlgorithm_MetaboxDesc)

private:
    CString m_metaboxDesc;

    CString m_name;
    CString m_authorName;
    CString m_authorCompanyName;
    CString m_shortDesc;
    CString m_detailedDesc;
    CString m_category;
    CString m_version;
    CString m_stockItemName;
    CString m_metaboxID;

    std::vector<io_stream_t> m_inputs;
    std::vector<io_stream_t> m_outputs;
    std::vector<setting_t> m_settings;
};
}  // namespace Metabox
}  // namespace OpenViBE
