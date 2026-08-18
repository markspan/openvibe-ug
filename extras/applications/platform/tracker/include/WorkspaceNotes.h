#pragma once

#include <openvibe/ov_all.h>

#include <fs/Files.h>

namespace OpenViBE {
namespace Tracker {

/**
 * \class WorkspaceNotes 
 * \brief An overly complicated wrapper for a text string.
 * \author J. T. Lindgren
 *
 * This class is needed in order to use CCommentEditorDialog from Designer. The
 * implementation is a quick hack. IDs and Attributable features are not supported at all.
 *
 */
class WorkspaceNotes final : public Kernel::IComment
{
public:
    CIdentifier getIdentifier() const override { return 0; }
    CString getText() const override { return m_notes; }

    bool setIdentifier(const CIdentifier& /*identifier*/) override { return true; }

    bool setText(const CString& sText) override
    {
        m_notes = sText;
        return true;
    }

    bool initializeFromExistingComment(const IComment& exisitingComment) override
    {
        m_notes = exisitingComment.getText();
        return true;
    }

    // Attributable
    bool addAttribute(const CIdentifier& /*attributeID*/, const CString& /*sAttributeValue*/) override { return true; }
    bool removeAttribute(const CIdentifier& /*attributeID*/) override { return true; }
    bool removeAllAttributes() override { return true; }
    CString getAttributeValue(const CIdentifier& /*attributeID*/) const override { return ""; }
    bool setAttributeValue(const CIdentifier& /*attributeID*/, const CString& /*sAttributeValue*/) override { return true; }
    bool hasAttribute(const CIdentifier& /*attributeID*/) const override { return false; }
    bool hasAttributes() const override { return false; }
    CIdentifier getNextAttributeIdentifier(const CIdentifier& /*previousID*/) const override { return CIdentifier(); }
    CIdentifier getClassIdentifier() const override { return CIdentifier(); }

    // Methods specific to this derived class

    // Save the notes
    bool save(const CString& notesFile) const
    {
        std::ofstream output;
        FS::Files::openOFStream(output, notesFile.toASCIIString());
        if (!output.is_open() || output.bad()) { return false; }
        output << m_notes.toASCIIString();

        output.close();

        return true;
    }

    // Load the notes
    bool load(const CString& notesFile)
    {
        if (FS::Files::fileExists(notesFile.toASCIIString())) {
            std::ifstream input;
            FS::Files::openIFStream(input, notesFile.toASCIIString());
            if (input.is_open() && !input.bad()) {
                const std::string str((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
                m_notes = str.c_str();
                input.close();
                return true;
            }
        }

        return false;
    }

protected:
    CString m_notes;
};
}  // namespace Tracker
}  // namespace OpenViBE
