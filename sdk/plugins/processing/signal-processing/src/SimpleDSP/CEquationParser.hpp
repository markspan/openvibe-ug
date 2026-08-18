///-------------------------------------------------------------------------------------------------
/// 
/// \file CEquationParser.hpp
/// \brief Classes for the Equation Parser.
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
#include "CAbstractTree.hpp"

#include <toolkit/ovtk_all.h>

#include <boost/spirit/include/classic_ast.hpp>

#include <array>

typedef char const* iterator_t;
typedef boost::spirit::classic::tree_match<iterator_t> parse_tree_match_t;
typedef parse_tree_match_t::tree_iterator iter_t;

class CAbstractTree;
class CAbstractTreeNode;

///<summary> Used to store the optional parameter of the function used by the pseudo-VM. </summary>
union UFunctionContext
{
    double direct_value = 0;    ///< if the parameter if a value (push_val)
    double** indirect_value;    ///< if it is a pointer to a value (PushVar)
};

///<summary> Type of the functions in the function stack generated from the equation. </summary>
typedef void (*function_p)(double*& stack, const UFunctionContext& oContext);

/// <summary> Equation Parser. </summary>
class CEquationParser
{
protected:
    CAbstractTree* m_tree = nullptr;    ///< The AST produced by the parsing of the equation

    SEquationGrammar m_grammar; ///< Grammar to use

    double** m_variable = nullptr;  ///< Pointer to the data referenced by X in the equation
    size_t m_nVariable  = 0;        ///< Number of accessible variables

    const size_t m_functionStackSize = 1024;    ///< Size of the "function stack" (where the sucessive function pointers are stored)
    function_p* m_functionList       = nullptr; ///< Pointer to the top of the function stack
    function_p* m_functionListBase   = nullptr; ///< Pointer to the base of the function stack

    const size_t m_functionContextStackSize     = 1024;     ///< Size of the "function context stack" (where the sucessive function context are stored)
    UFunctionContext* m_functionContextList     = nullptr;  ///< Pointer to the top of the function context stack
    UFunctionContext* m_functionContextListBase = nullptr;  ///< Pointer to the base of the function context stack

    const size_t m_stackSize = 1024;    ///< Size of the "local" stack
    double* m_stack          = nullptr; ///< Pointer to the top of the "local" stack

    size_t m_nOperations = 0;   ///< Number of pointers/contexts in the function/context stacks (same for both)

    static std::array<function_p, 32> m_functionTable;  ///< Table of function pointers

    size_t m_treeCategory  = OP_USERDEF;    ///< Category of the tree (OP_USERDEF or Special tree)
    double m_treeParameter = 0;             ///< Optional parameter in case of a special tree

    OpenViBE::Toolkit::TBoxAlgorithm<OpenViBE::Plugins::IBoxAlgorithm>& m_parentPlugin;

public:
    /// <summary> Initializes a new instance of the <see cref="CEquationParser"/> class. </summary>
    /// <param name="plugin"> The plugin. </param>
    /// <param name="variable"> Pointer to the data known as X in the equation. </param>
    /// <param name="nVariable"> The number of variable. </param>
    CEquationParser(OpenViBE::Toolkit::TBoxAlgorithm<OpenViBE::Plugins::IBoxAlgorithm>& plugin, double** variable, const size_t nVariable)
        : m_variable(variable), m_nVariable(nVariable), m_parentPlugin(plugin) {}

    /// <summary> Finalizes an instance of the <see cref="CEquationParser"/> class. </summary>
    ~CEquationParser();

#if 0
    void setVariable(double * pVariable){ m_pVariable=pVariable; }
#endif

    /// <summary> Compiles the given equation, and generates the successive function calls to achieve the same result if needed (depends on m_treeCategory). </summary>
    /// <param name="equation"> The equation to use. </param>
    /// <returns></returns>
    bool CompileEquation(const char* equation);

    void PushValue(double value);
    void PushVar(size_t index);
    void PushOp(size_t op);

    /// <summary> Gets the tree category. </summary>
    /// <returns> The tree's category. </returns>
    size_t GetTreeCategory() const { return m_treeCategory; }

    /// <summary> Gets the optional parameter. </summary>
    /// <returns> The optional parameter. </returns>
    double GetTreeParameter() const { return m_treeParameter; }

    /// <summary> Executes the successive function calls from the function stack and returns the result. </summary>
    /// <returns> The result of the equation applied to the value referenced by X. </returns>
    double ExecuteEquation()
    {
        function_p* currentFunction     = m_functionList - 1;
        function_p* lastFunctionPointer = currentFunction - m_nOperations;

        UFunctionContext* currentFunctionContext = m_functionContextList - 1;

        //while there are function pointers
        while (currentFunction != lastFunctionPointer) {
            //calls the function with the current function context
            (*currentFunction)(m_stack, *currentFunctionContext);

            //updates the stack pointers
            currentFunction--;
            currentFunctionContext--;
        }

        //pop and return the result
        return *(m_stack--);
    }

private:
    void createAbstractTree(boost::spirit::classic::tree_parse_info<> oInfo);
    CAbstractTreeNode* createNode(iter_t const& i) const;

public:
    static void Neg(double*& stack, const UFunctionContext& ctx);
    static void Add(double*& stack, const UFunctionContext& ctx);
    static void Div(double*& stack, const UFunctionContext& ctx);
    static void Sub(double*& stack, const UFunctionContext& ctx);
    static void Mul(double*& stack, const UFunctionContext& ctx);

    static void Power(double*& stack, const UFunctionContext& ctx);

    static void Abs(double*& stack, const UFunctionContext& ctx);
    static void Acos(double*& stack, const UFunctionContext& ctx);
    static void Asin(double*& stack, const UFunctionContext& ctx);
    static void Atan(double*& stack, const UFunctionContext& ctx);
    static void Ceil(double*& stack, const UFunctionContext& ctx);
    static void Cos(double*& stack, const UFunctionContext& ctx);
    static void Exp(double*& stack, const UFunctionContext& ctx);
    static void Floor(double*& stack, const UFunctionContext& ctx);
    static void Log(double*& stack, const UFunctionContext& ctx);
    static void Log10(double*& stack, const UFunctionContext& ctx);
    static void Rand(double*& stack, const UFunctionContext& ctx);
    static void Sin(double*& stack, const UFunctionContext& ctx);
    static void Sqrt(double*& stack, const UFunctionContext& ctx);
    static void Tan(double*& stack, const UFunctionContext& ctx);

    static void IfThenElse(double*& stack, const UFunctionContext& ctx);

    static void CmpLower(double*& stack, const UFunctionContext& ctx);
    static void CmpGreater(double*& stack, const UFunctionContext& ctx);
    static void CmpLowerEqual(double*& stack, const UFunctionContext& ctx);
    static void CmpGreaterEqual(double*& stack, const UFunctionContext& ctx);
    static void CmpEqual(double*& stack, const UFunctionContext& ctx);
    static void CmpNotEqual(double*& stack, const UFunctionContext& ctx);

    static void BoolAnd(double*& stack, const UFunctionContext& ctx);
    static void BoolOr(double*& stack, const UFunctionContext& ctx);
    static void BoolNot(double*& stack, const UFunctionContext& ctx);
    static void BoolXor(double*& stack, const UFunctionContext& ctx);

    static void LoadVal(double*& stack, const UFunctionContext& ctx);
    static void LoadVar(double*& stack, const UFunctionContext& ctx);
};
