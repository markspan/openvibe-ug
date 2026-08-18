#pragma once

#include "defines.h"
#include <string>
#include <cstdlib>  // fix Unix compatibility

namespace XML {
/**
 * @class IXMLNode
 * @author Guillaume Serrière (INRIA/Loria)
 * @brief Symbolize a node in a XML tree structure.
 * @sa XML
 */
class XML_API IXMLNode
{
public:
    virtual void release() = 0;

    virtual const char* getName() const = 0;

    //Attribute
    /**
     * @brief Add the attribute name with value
     * value to the node.
     * @param name [in] : Name of the attribute
     * @param value [in] : Value of the attribute
     * @return true in success, false otherwise
     */
    virtual bool addAttribute(const char* name, const char* value) = 0;

    /**
     * @brief Indicate if an attribute exists or not.
     * @param name [in] : Name of the attribute
     * @return true if attribute exists, false otherwise
     */
    virtual bool hasAttribute(const char* name) const = 0;

    /**
     * @brief Return the value of an attribute.
     * @param name [in] : Name of the attribute
     * @return Value of the attribute
     */
    virtual const char* getAttribute(const char* name) const = 0;

    //PCDATA
    /**
     * @brief Set the PCDATA of the node.
     * @param data [in] : Value of the PCDATA
     */
    virtual void setPCData(const char* data) = 0;

    /**
     * @brief Apppend a string to the current PCDATA of the node
     * @param data [in] : Value of teh PCDATA to append
     */
    virtual void appendPCData(const char* data) = 0;

    /**
     * @brief Return the PCDATA of the node.
     * @return Value of PCDATA
     */
    virtual const char* getPCData() const = 0;

    //Child
    /**
     * @brief Add a node child of the
     * @param node [in] : The Node that will became the new child
     */
    virtual void addChild(IXMLNode* node) = 0;

    /**
     * @brief Return the ith child of the node.
     * @param index [in] : index of the child.
     * @return The ith child of the node.
     */
    virtual IXMLNode* getChild(const size_t index) const = 0;

    /**
     * @brief Return the first child with the name name.
     * @param name [in]] : Name of th child
     * @return The first child of the node which name is name.
     */
    virtual IXMLNode* getChildByName(const char* name) const = 0;

    /**
     * @brief Return the amount of child the node has.
     * @return Amount of child.
     */
    virtual size_t getChildCount() const = 0;

    //XML generation
    /**
     * @brief Return a string which contains the XML of the node. The string is dynamically instantiate so
     * it requires to be free.
     * @param depth [in] : Amount of indentation
     * @return XML string describing the node and its childs.
     */
    virtual char* getXML(const size_t depth = 0) const = 0;

protected:
    virtual ~IXMLNode() {}
};

/**
 * @brief Create a new node with the name name. The node is created dynamically and requires to be free.
 * @param name [in] : Name of the node
 * @return New node
 */
extern XML_API IXMLNode* createNode(const char* name);
}  // namespace XML
