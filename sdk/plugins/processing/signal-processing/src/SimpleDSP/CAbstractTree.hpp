///-------------------------------------------------------------------------------------------------
/// 
/// \file CAbstractTree.hpp
/// \brief Classes for the Abstract Tree.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include "CEquationParserGrammar.hpp"
#include "CEquationParser.hpp"

#include <openvibe/ov_all.h>

#include <boost/spirit/include/classic_ast.hpp>

#include <vector>

class CEquationParser;

/**
* Abstract class for an AST tree node
*
*/
class CAbstractTreeNode
{
protected:
    //! True if this is a terminal node
    bool m_isTerminal = false;
    //! True if this node contains a constant value
    bool m_isConstant = false;

public:
    CAbstractTreeNode(const bool bTerminal, const bool bIsConstant) : m_isTerminal(bTerminal), m_isConstant(bIsConstant) { }

    //! virtual destructor
    virtual ~CAbstractTreeNode() { }

    /**
    * Used to know if this node is a leaf.
    * \return True if the node is a leaf.
    */
    virtual bool IsTerminal() const { return m_isTerminal; }

    /**
     * Used to know if this node is a constant value node.
     * \return True if the node is a constant value node.
     */
    virtual bool IsConstant() const { return m_isConstant; }

    //! Prints the node to stdout.
    virtual void Print(OpenViBE::Kernel::ILogManager& rLogManager) = 0;

    /**
    * Used to simplify this node (and its children if any).
    * \param pModifiedNode Reference to a pointer to modify if the
    * current node object is to be destroyed and replaced. This pointer
    * will contain the address of the new node.
    */
    virtual bool Simplify(CAbstractTreeNode*& pModifiedNode) = 0;

    /**
    * Part of the process of simplification.
    * Levels recursively the associative operators nodes.
    */
    virtual void LevelOperators() = 0;

    /**
    * Changes the tree so it uses the NEG operator whenever it is possible.
    * (ie replaces (* -1 X) by (NEG X)
    */
    virtual void UseNegationOperator() = 0;

    /**
    * Generates the set of function calls needed to do the desired computation.
    * \param oParser The parser containing the function pointers stack and function contexts stack.
    */
    virtual void GenerateCode(CEquationParser& oParser) = 0;
};

/**
* A tree's parent node (has children).
*
*/
class CAbstractTreeParentNode : public CAbstractTreeNode
{
public:
    //! Children of this node
    std::vector<CAbstractTreeNode*> m_Children;

    //! The node operator's identifier
    uint64_t m_ID = 0;

    //! True if the node is "associative"
    bool m_IsAssociative = false;

    //Constructors
    CAbstractTreeParentNode(const uint64_t nodeId, const bool isAssociative = false)
        : CAbstractTreeNode(false, false), m_ID(nodeId), m_IsAssociative(isAssociative) { }

    CAbstractTreeParentNode(const uint64_t nodeId, CAbstractTreeNode* child, const bool isAssociative = false)
        : CAbstractTreeNode(false, false), m_ID(nodeId), m_IsAssociative(isAssociative) { m_Children.push_back(child); }

    CAbstractTreeParentNode(const uint64_t nodeId, CAbstractTreeNode* leftChild, CAbstractTreeNode* rightChild, const bool isAssociative = false)
        : CAbstractTreeNode(false, false), m_ID(nodeId), m_IsAssociative(isAssociative)
    {
        m_Children.push_back(leftChild);
        m_Children.push_back(rightChild);
    }

    CAbstractTreeParentNode(const uint64_t nodeId, CAbstractTreeNode* testChild, CAbstractTreeNode* ifChild, CAbstractTreeNode* thenChild,
                            const bool isAssociative = false)
        : CAbstractTreeNode(false, false), m_ID(nodeId), m_IsAssociative(isAssociative)
    {
        m_Children.push_back(testChild);
        m_Children.push_back(ifChild);
        m_Children.push_back(thenChild);
    }

    /**
     * Returns the node's operator identifier.
     * \return The operator identifier
     */
    uint64_t GetOperatorIdentifier() const { return m_ID; }

    /**
     * Used to know if the node is an associative node.
     * \return True if the node is an associative one.
     */
    bool IsAssociative() const { return m_IsAssociative; }

    /**
     * Returns the vector of children of the node.
     * \return A reference to the vector of children.
     */
    virtual std::vector<CAbstractTreeNode*>& GetChildren() { return m_Children; }

    /**
     * Adds a child to this node.
     * \param child The child to add.
     */
    virtual void AddChild(CAbstractTreeNode* child) { m_Children.push_back(child); }

    //! Destructor.
    ~CAbstractTreeParentNode() override;

    //! Debug function, prints the node and its children (prefix notation)
    void Print(OpenViBE::Kernel::ILogManager& logManager) override
    {
        logManager << "(" << toString(EByteCodes(m_ID)) << " ";
        for (size_t i = 0; i < m_Children.size(); ++i) {
            if (m_Children[i] == nullptr) { }
            else { m_Children[i]->Print(logManager); }
            logManager << " ";
        }
        logManager << ")";
    }

    bool Simplify(CAbstractTreeNode*& node) override;
    void LevelOperators() override;
    void UseNegationOperator() override;
    void GenerateCode(CEquationParser& parser) override;
};

/**
 * Class for terminal nodes containing a single value.
 *
 */
class CAbstractTreeValueNode : public CAbstractTreeNode
{
protected:
    //! Value associated with the node.
    double m_value = 0;

public:
    explicit CAbstractTreeValueNode(const double value) : CAbstractTreeNode(true, true), m_value(value) {}

    //! Destructor
    ~CAbstractTreeValueNode() override { }

    /**
    * Used to set the value of the node.
    * \param value The node's new value.
    */
    void SetValue(const double value) { m_value = value; }

    /**
     * Used to know the value of the node.
     * \return The node's value.
     */
    double GetValue() const { return m_value; }

    void Print(OpenViBE::Kernel::ILogManager& logManager) override { logManager << m_value; }

    bool Simplify(CAbstractTreeNode*& modifiedNode) override
    {
        modifiedNode = this;
        return false;
    }

    void LevelOperators() override { }
    void UseNegationOperator() override { }
    void GenerateCode(CEquationParser& parser) override;
};

/**
 * Class for terminal nodes referencing a variable.
 */
class CAbstractTreeVariableNode : public CAbstractTreeNode
{
public:
    explicit CAbstractTreeVariableNode(const size_t index) : CAbstractTreeNode(true, false), m_index(index) { }

    ~CAbstractTreeVariableNode() override { }

    void Print(OpenViBE::Kernel::ILogManager& logManager) override
    {
        char name[2];
        name[0] = char('a' + m_index);
        name[1] = 0;
        logManager << name;
    }

    bool Simplify(CAbstractTreeNode*& modifiedNode) override
    {
        modifiedNode = this;
        return false;
    }

    void LevelOperators() override { }
    void UseNegationOperator() override { }
    void GenerateCode(CEquationParser& parser) override;

protected:
    size_t m_index = 0;
};

/**
* Main class for the AST.
* Contains the root of the tree.
*/
class CAbstractTree
{
protected:
    //! the root of the AST tree.
    CAbstractTreeNode* m_root = nullptr;

public:
    //! Constructor
    explicit CAbstractTree(CAbstractTreeNode* root) : m_root(root) { }

    //! Destructor
    ~CAbstractTree() { delete m_root; }

    //! Prints the whole tree.
    void PrintTree(OpenViBE::Kernel::ILogManager& logManager) const { m_root->Print(logManager); }

    /**
     * Used to simplify the tree.
     */
    void SimplifyTree();

    /**
     * Part of the process of simplification.
     * Levels recursively the associative operators nodes.
     */
    void LevelOperators() const { m_root->LevelOperators(); }

    /**
     * Changes the tree so it uses the NEG operator whenever it is possible.
     * (ie replaces (* -1 X) by (NEG X)
     */
    void UseNegationOperator() const { m_root->UseNegationOperator(); }

    /**
     * Generates the set of function calls needed to do the desired computation.
     * \param parser The parser containing the function pointers stack and function contexts stack.
     */
    void GenerateCode(CEquationParser& parser) const;

    /**
    * Tries to recognize simple tree structures (X*X, X*Cste, X+Cste,...)
    * \param treeId The identifier of the tree (OP_USERDEF for non special tree).
    * \param parameter The optional parameter if it is a special tree.
    */
    void RecognizeSpecialTree(uint64_t& treeId, double& parameter) const;
};

/**
* Functor used to compare two nodes.
* The order is as follow : Constants, Variables, ParentNodes
*/
struct SAbstractTreeNodeOrderingFunction
{
    bool operator()(CAbstractTreeNode* const & firstNode, CAbstractTreeNode* const & secondNode) const
    {
#if 0
        if( (firstNode->IsConstant()) ||
            (firstNode->IsTerminal() && !secondNode->IsConstant()) ||
            (!firstNode->IsTerminal() && !secondNode->IsTerminal())) { return true; }
        else { return false; }
#else
        // Check IsConstant flag
        if (firstNode->IsConstant() && !secondNode->IsConstant()) { return true; }
        if (!firstNode->IsConstant() && secondNode->IsConstant()) { return false; }

        // Check IsTerminal flag
        if (firstNode->IsTerminal() && !secondNode->IsTerminal()) { return true; }
        if (!firstNode->IsTerminal() && secondNode->IsTerminal()) { return false; }

        // At this point, IsTerminal and IsConstant are the same for both value
        // Order is not important any more, we just compare the pointer values
        // so to have strict ordering function
        return firstNode < secondNode;
#endif
    }
};
