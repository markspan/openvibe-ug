#pragma once

#include "defines.h"
#include "IXMLNode.h"
#include <string>
#include <cstdlib>  // fix Unix compatibility

namespace XML {
/**
 * @class IXMLHandler
 * @author Guillaume Serrière (INRIA/Loria)
 * @brief This class is design to help about XML manipulation.
 * @sa XML
 */
class XML_API IXMLHandler
{
public:
    /**
     * @brief Release the handler.
     */
    virtual void release() = 0;

    //Parsing
    /**
     * @brief Parse file points by sPath and return the root name of the document.
     * @param path [in] : Path to the File
     * @return The root node of the document, or NULL if there is an error.
     */
    virtual IXMLNode* parseFile(const char* path) = 0;

    /**
     * @brief Parse the string sString on uiSize caracters and return the root name of the document.
     * @param str [in] : String which contains the XML
     * @param size [in] : Size of the part to analyze
     * @return The root node of the parse part, or NULL if there is an error.
     */
    virtual IXMLNode* parseString(const char* str, const size_t& size) = 0;

    //XML extraction
    /**
     * @brief Write the XML corresponding to the node rNode in the file points by sPath. If the file exists
     * it will be erase.
     * @param node [in] : The node to write.
     * @param path [in] : The path to the file.
     * @return True on success, false otherwise.
     */
    virtual bool writeXMLInFile(const IXMLNode& node, const char* path) const = 0;

    /**
     * @brief Get the description of the last error that ocurred
     * @return A string object containing the error description
     */
    virtual std::string getLastErrorString() const = 0;

protected:
    virtual ~IXMLHandler() { }
};

extern XML_API IXMLHandler* createXMLHandler();
}  // namespace XML
