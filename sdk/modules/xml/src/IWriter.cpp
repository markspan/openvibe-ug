#include "IWriter.h"

#include <stack>
#include <string>

namespace XML {
class CWriter final : public IWriter
{
public:
    explicit CWriter(IWriterCallback& callback) : m_callback(callback) {}
    bool openChild(const char* name) override;
    bool setChildData(const char* data) override;
    bool setAttribute(const char* name, const char* value) override;
    bool closeChild() override;
    void release() override;

private:
    static void sanitize(std::string& str, bool escapeQuotes = true);

protected:
    IWriterCallback& m_callback;
    std::stack<std::string> m_nodes;
    bool m_hasChild             = false;
    bool m_hasData              = false;
    bool m_hasClosedOpeningNode = true;
};

bool CWriter::openChild(const char* name)
{
    if (name == nullptr) { return false; }

    if (m_hasData) { return false; }

    if (!m_hasClosedOpeningNode)
    {
        m_callback.write(">");
        m_hasClosedOpeningNode = true;
    }

    const std::string indent(m_nodes.size(), '\t');
    const std::string res = (!m_nodes.empty() ? std::string("\n") : std::string("")) + indent + "<" + name;
    m_callback.write(res.c_str());
    m_nodes.push(name);
    m_hasChild             = false;
    m_hasData              = false;
    m_hasClosedOpeningNode = false;
    return true;
}

bool CWriter::setChildData(const char* data)
{
    if (data == nullptr) { return false; }

    if (m_hasChild) { return false; }

    if (!m_hasClosedOpeningNode)
    {
        m_callback.write(">");
        m_hasClosedOpeningNode = true;
    }

    std::string str(data);
    this->sanitize(str, false);

    m_callback.write(str.c_str());
    m_hasChild = false;
    m_hasData  = true;
    return true;
}

bool CWriter::setAttribute(const char* name, const char* value)
{
    if (name == nullptr || value == nullptr || m_hasChild || m_hasData || m_hasClosedOpeningNode) { return false; }

    std::string str(value);
    this->sanitize(str);

    const std::string res = std::string(" ") + name + "=\"" + str + "\"";
    m_callback.write(res.c_str());
    return true;
}

bool CWriter::closeChild()
{
    if (m_nodes.empty()) { return false; }

    if (!m_hasClosedOpeningNode)
    {
        m_callback.write(">");
        m_hasClosedOpeningNode = true;
    }

    const std::string indent(m_nodes.size() - 1, '\t');
    const std::string res = ((m_hasData || !m_hasChild) ? std::string("") : std::string("\n") + indent) + "</" + m_nodes.top() + ">";
    m_callback.write(res.c_str());
    m_nodes.pop();
    m_hasChild = true;
    m_hasData  = false;
    return true;
}

void CWriter::release()
{
    while (!m_nodes.empty()) { closeChild(); }
    delete this;
}

void CWriter::sanitize(std::string& str, const bool escapeQuotes)
{
    std::string::size_type i;
    if (str.length() != 0)
    {
        // mandatory, this one should be the first because the other ones add & symbols
        for (i = str.find("&", 0); i != std::string::npos; i = str.find("&", i + 1)) { str.replace(i, 1, "&amp;"); }
        for (i = str.find('<', 0); i != std::string::npos; i = str.find('<', i + 1)) { str.replace(i, 1, "&lt;"); }
        for (i = str.find('>', 0); i != std::string::npos; i = str.find('>', i + 1)) { str.replace(i, 1, "&gt;"); }

        // Quotes need only be escaped in attributes
        if (escapeQuotes)
        {
            for (i = str.find('\'', 0); i != std::string::npos; i = str.find('\'', i + 1)) { str.replace(i, 1, "&apos;"); }
            for (i = str.find('\"', 0); i != std::string::npos; i = str.find('\"', i + 1)) { str.replace(i, 1, "&quot;"); }
        }
    }
}

XML_API IWriter* createWriter(IWriterCallback& callback) { return new CWriter(callback); }

}  // namespace XML
