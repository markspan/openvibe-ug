///-------------------------------------------------------------------------------------------------
/// 
/// \file CEquationParser.cpp
/// \brief Classes implementation for the Equation Parser.
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

#include "defines.hpp"
#include "CEquationParser.hpp"

#include <cstdlib>
#include <cmath>
#include <string>
#include <iostream>
#include <algorithm>
#include <functional>
#include <cctype>


#define DEBUG_LOG(level, message) m_parentPlugin.getLogManager() << level << message << "\n";
#define DEBUG_PRINT_TREE(level) { m_parentPlugin.getLogManager() << level; m_tree->PrintTree(m_parentPlugin.getLogManager()); m_parentPlugin.getLogManager() << "\n"; }

// because std::tolower has multiple signatures,
// it can not be easily used in std::transform
// this workaround is taken from http://www.gcek.net/ref/books/sw/cpp/ticppv2/
template <class T>
static T ToLower(T c) { return std::tolower(c); }

// BOOST::Ast should be able to remove spaces / tabs etc but
// unfortunately, it seems it does not work correcly in some
// cases so I add this sanitizer function to clear the Simple DSP
// equation before sending it to BOOST::Ast
static std::string FindAndReplace(std::string s, const std::string& f, const std::string& r)
{
    size_t i;
    while ((i = s.find(f)) != std::string::npos) { s.replace(i, f.length(), r); }
    return s;
}

std::array<function_p, 32> CEquationParser::m_functionTable = {
    &Neg, &Add, &Sub, &Mul, &Div, &Abs, &Acos, &Asin, &Atan, &Ceil, &Cos, &Exp, &Floor, &Log, &Log10, &Power,
    &Rand, &Sin, &Sqrt, &Tan, &IfThenElse, &CmpLower, &CmpGreater, &CmpLowerEqual, &CmpGreaterEqual, &CmpEqual,
    &CmpNotEqual, &BoolAnd, &BoolOr, &BoolNot, &BoolXor,
};

CEquationParser::~CEquationParser()
{
    delete[] m_functionListBase;
    delete[] m_functionContextListBase;
    delete[] m_stack;
    delete m_tree;
}

bool CEquationParser::CompileEquation(const char* equation)
{
    // BOOST::Ast should be able to remove spaces / tabs etc but
    // unfortunately, it seems it does not work correcly in some
    // cases so I add this sanitizer function to clear the Simple DSP
    // equation before sending it to BOOST::Ast
    std::string str(equation);
    str = FindAndReplace(str, " ", "");
    str = FindAndReplace(str, "\t", "");
    str = FindAndReplace(str, "\n", "");

    //parses the equation
    DEBUG_LOG(OpenViBE::Kernel::LogLevel_Trace, "Parsing equation [" << str << "]...");
    const boost::spirit::classic::tree_parse_info<> info = ast_parse(str.c_str(), m_grammar >> boost::spirit::classic::end_p, boost::spirit::classic::space_p);

    //If the parsing was successful
    if (info.full) {
        //creates the AST
        DEBUG_LOG(OpenViBE::Kernel::LogLevel_Trace, "Creating abstract tree...");
        createAbstractTree(info);
        DEBUG_PRINT_TREE(OpenViBE::Kernel::LogLevel_Debug);

#if 0
        //CONSTANT FOLDING
        //levels the associative/commutative operators (+ and *)
        DEBUG_LOG_(OpenViBE::Kernel::LogLevel_Trace, "Leveling tree...");
        m_tree->levelOperators();
        DEBUG_PRINT_TREE_(OpenViBE::Kernel::LogLevel_Debug);

        //simplifies the AST
        DEBUG_LOG_(OpenViBE::Kernel::LogLevel_Trace, "Simplifying tree...");
        m_tree->simplifyTree();
        DEBUG_PRINT_TREE_(OpenViBE::Kernel::LogLevel_Debug);

        //tries to replace nodes to use the NEG operator and reduce complexity
        DEBUG_LOG_(OpenViBE::Kernel::LogLevel_Trace, "Generating bytecode...");
        m_tree->useNegationOperator();
        DEBUG_PRINT_TREE_(OpenViBE::Kernel::LogLevel_Debug);

        //Detects if it is a special tree (updates m_treeCategory and m_treeParameter)
        DEBUG_LOG_(OpenViBE::Kernel::LogLevel_Trace, "Recognizing special tree...");
        m_tree->recognizeSpecialTree(m_treeCategory, m_treeParameter);
        DEBUG_PRINT_TREE_(OpenViBE::Kernel::LogLevel_Debug);

        //Unrecognize special tree
        DEBUG_LOG_("Unrecognizing special tree...");
        m_treeCategory = OP_USERDEF;
        DEBUG_PRINT_TREE_(OpenViBE::Kernel::LogLevel_Debug);
#endif

        //If it is not a special tree, we need to generate some code to reach the result
        if (m_treeCategory == OP_USERDEF) {
            //allocates the function stack
            m_functionList     = new function_p[m_functionStackSize];
            m_functionListBase = m_functionList;

            //Allocates the function context stack
            m_functionContextList     = new UFunctionContext[m_functionContextStackSize];
            m_functionContextListBase = m_functionContextList;
            m_stack                   = new double[m_stackSize];

            //generates the code
            m_tree->GenerateCode(*this);

            //computes the number of steps to get to the result
            m_nOperations = m_functionList - m_functionListBase;
        }

        return true;
    }
    std::string error;
    const size_t pos = str.find(info.stop);
    if (pos != std::string::npos) {
        for (size_t i = 0; i < pos; ++i) { error += " "; }
        error += "^--Here\n";
    }

    OV_ERROR("Failed parsing equation \n[" << equation << "]\n " << error, OpenViBE::Kernel::ErrorType::BadParsing, false,
             m_parentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getErrorManager(),
             m_parentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getLogManager());
}

void CEquationParser::createAbstractTree(boost::spirit::classic::tree_parse_info<> oInfo) { m_tree = new CAbstractTree(createNode(oInfo.trees.begin())); }

CAbstractTreeNode* CEquationParser::createNode(iter_t const& i) const
{
    if (i->value.id() == SEquationGrammar::EXPRESSION_ID) {
        if (*i->value.begin() == '+') {
            return new CAbstractTreeParentNode(OP_ADD, createNode(i->children.begin()), createNode(i->children.begin() + 1), true);
        }
        //replaces (- X Y) by (+ X (-Y)) (in fact (+ X (* -1 Y)) )
        if (*i->value.begin() == '-') {
            return new CAbstractTreeParentNode(OP_ADD, createNode(i->children.begin()),
                                               new CAbstractTreeParentNode(OP_MUL, new CAbstractTreeValueNode(-1), createNode(i->children.begin() + 1), true),
                                               true);
        }
    }
    else if (i->value.id() == SEquationGrammar::TERM_ID) {
        if (*i->value.begin() == '*') {
            return new CAbstractTreeParentNode(OP_MUL, createNode(i->children.begin()), createNode(i->children.begin() + 1), true);
        }
        if (*i->value.begin() == '/') { return new CAbstractTreeParentNode(OP_DIV, createNode(i->children.begin()), createNode(i->children.begin() + 1)); }
    }
    else if (i->value.id() == SEquationGrammar::FACTOR_ID) {
        if (*i->value.begin() == '-') {
            // -X => (* -1 X), useful to simplify the tree later
            return new CAbstractTreeParentNode(OP_MUL, new CAbstractTreeValueNode(-1), createNode(i->children.begin()), true);
        }
        if (*i->value.begin() == '+') { return createNode(i->children.begin()); }
    }
    else if (i->value.id() == SEquationGrammar::REAL_ID) {
        const std::string value(i->value.begin(), i->value.end());
        return new CAbstractTreeValueNode(strtod(value.c_str(), nullptr));
    }
    else if (i->value.id() == SEquationGrammar::VARIABLE_ID) {
        size_t idx = 0;
        const std::string value(i->value.begin(), i->value.end());
        if (value != "x" && value != "X") {
            if (value[0] >= 'a' && value[0] <= 'z') { idx = value[0] - 'a'; }
            if (value[0] >= 'A' && value[0] <= 'Z') { idx = value[0] - 'A'; }
        }

        if (idx >= m_nVariable) {
            OV_WARNING("Missing input " << idx+1 << " (referenced with variable [" << value << "])",
                       m_parentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getLogManager());
            return new CAbstractTreeValueNode(0);
        }
        return new CAbstractTreeVariableNode(idx);
    }
    else if (i->value.id() == SEquationGrammar::CONSTANT_ID) {
        std::string value(i->value.begin(), i->value.end());

        //converts the string to lowercase
        std::transform(value.begin(), value.end(), value.begin(), ::ToLower<std::string::value_type>);

        //creates a new value node from the value looked up in the constant's symbols table
        return new CAbstractTreeValueNode(*find(mathConstant_p, value.c_str()));
    }
    else if (i->value.id() == SEquationGrammar::FUNCTION_ID) {
        std::string value(i->value.begin(), i->value.end());
        uint64_t* functionID;

        //converts the string to lowercase
        std::transform(value.begin(), value.end(), value.begin(), ::ToLower<std::string::value_type>);

        //gets the function's Id from the unary function's symbols table
        if ((functionID = find(unaryFunction_p, value.c_str())) != nullptr) {
            return new CAbstractTreeParentNode(*functionID, createNode(i->children.begin()), false);
        }
        //gets the function's Id from the binary function's symbols table
        if ((functionID = find(binaryFunction_p, value.c_str())) != nullptr) {
            return new CAbstractTreeParentNode(*functionID, createNode(i->children.begin()), createNode(i->children.begin() + 1), false);
        }
    }
    else if (i->value.id() == SEquationGrammar::IFTHEN_ID) {
        return new CAbstractTreeParentNode(OP_IF_THEN_ELSE, createNode(i->children.begin()), createNode(i->children.begin() + 1),
                                           createNode(i->children.begin() + 2), false);
    }
    else if (i->value.id() == SEquationGrammar::COMPARISON_ID) {
        std::string value(i->value.begin(), i->value.end());
        uint64_t* functionID;

        //converts the string to lowercase
        std::transform(value.begin(), value.end(), value.begin(), ::ToLower<std::string::value_type>);

        //gets the function's Id from the comparison function's symbols table
        if ((functionID = find(comparison1Function_p, value.c_str())) != nullptr) {
            return new CAbstractTreeParentNode(*functionID, createNode(i->children.begin()), createNode(i->children.begin() + 1), false);
        }
        //gets the function's Id from the comparison function's symbols table
        if ((functionID = find(comparison2Function_p, value.c_str())) != nullptr) {
            return new CAbstractTreeParentNode(*functionID, createNode(i->children.begin()), createNode(i->children.begin() + 1), false);
        }
    }
    else if (i->value.id() == SEquationGrammar::BOOLEAN_ID) {
        std::string value(i->value.begin(), i->value.end());
        uint64_t* functionID;

        //converts the string to lowercase
        std::transform(value.begin(), value.end(), value.begin(), ::ToLower<std::string::value_type>);

        //gets the function's Id from the binary boolean function's symbols table
        if ((functionID = find(binaryBoolean1Function_p, value.c_str())) != nullptr) {
            return new CAbstractTreeParentNode(*functionID, createNode(i->children.begin()), createNode(i->children.begin() + 1), false);
        }
        //gets the function's Id from the binary boolean function's symbols table
        if ((functionID = find(binaryBoolean2Function_p, value.c_str())) != nullptr) {
            return new CAbstractTreeParentNode(*functionID, createNode(i->children.begin()), createNode(i->children.begin() + 1), false);
        }
        //gets the function's Id from the binary boolean function's symbols table
        if ((functionID = find(binaryBoolean3Function_p, value.c_str())) != nullptr) {
            return new CAbstractTreeParentNode(*functionID, createNode(i->children.begin()), createNode(i->children.begin() + 1), false);
        }
        //gets the function's Id from the binary boolean function's symbols table
        if ((functionID = find(unaryBooleanFunction_p, value.c_str())) != nullptr) {
            return new CAbstractTreeParentNode(*functionID, createNode(i->children.begin()), false);
        }
    }

    return nullptr;
}

void CEquationParser::PushValue(const double value)
{
    *(m_functionList++)                       = LoadVal;
    (*(m_functionContextList++)).direct_value = value;
}

void CEquationParser::PushVar(const size_t index)
{
    *(m_functionList++)                         = LoadVar;
    (*(m_functionContextList++)).indirect_value = &m_variable[index];
}

void CEquationParser::PushOp(const size_t op)
{
    *(m_functionList++)                         = m_functionTable[op];
    (*(m_functionContextList++)).indirect_value = nullptr;
}

// Functions called by our "pseudo - VM"

void CEquationParser::Neg(double*& stack, const UFunctionContext& /*ctx*/) { *stack = - (*stack); }

void CEquationParser::Add(double*& stack, const UFunctionContext& /*ctx*/)
{
    stack--;
    *(stack) = *(stack + 1) + *(stack);
}

void CEquationParser::Sub(double*& stack, const UFunctionContext& /*ctx*/)
{
    stack--;
    *(stack) = *(stack + 1) - *(stack);
}

void CEquationParser::Mul(double*& stack, const UFunctionContext& /*ctx*/)
{
    stack--;
    *(stack) = *(stack + 1) * *(stack);
}

void CEquationParser::Div(double*& stack, const UFunctionContext& /*ctx*/)
{
    stack--;
    *(stack) = *(stack + 1) / *(stack);
}

void CEquationParser::Power(double*& stack, const UFunctionContext& /*ctx*/)
{
    stack--;
    *stack = pow(*(stack + 1), *(stack));
}

void CEquationParser::Abs(double*& stack, const UFunctionContext& /*ctx*/) { *stack = fabs(*(stack)); }
void CEquationParser::Acos(double*& stack, const UFunctionContext& /*ctx*/) { *stack = acos(*(stack)); }
void CEquationParser::Asin(double*& stack, const UFunctionContext& /*ctx*/) { *stack = asin(*(stack)); }
void CEquationParser::Atan(double*& stack, const UFunctionContext& /*ctx*/) { *stack = atan(*(stack)); }
void CEquationParser::Ceil(double*& stack, const UFunctionContext& /*ctx*/) { *stack = ceil(*(stack)); }
void CEquationParser::Cos(double*& stack, const UFunctionContext& /*ctx*/) { *stack = cos(*(stack)); }
void CEquationParser::Exp(double*& stack, const UFunctionContext& /*ctx*/) { *stack = exp(*(stack)); }
void CEquationParser::Floor(double*& stack, const UFunctionContext& /*ctx*/) { *stack = floor(*(stack)); }
void CEquationParser::Log(double*& stack, const UFunctionContext& /*ctx*/) { *stack = log(*(stack)); }
void CEquationParser::Log10(double*& stack, const UFunctionContext& /*ctx*/) { *stack = log10(*(stack)); }
void CEquationParser::Rand(double*& stack, const UFunctionContext& /*ctx*/) { *stack = Random<double>(0, 1) * *(stack); }
void CEquationParser::Sin(double*& stack, const UFunctionContext& /*ctx*/) { *stack = sin(*(stack)); }
void CEquationParser::Sqrt(double*& stack, const UFunctionContext& /*ctx*/) { *stack = sqrt(*(stack)); }
void CEquationParser::Tan(double*& stack, const UFunctionContext& /*ctx*/) { *stack = tan(*(stack)); }

void CEquationParser::IfThenElse(double*& stack, const UFunctionContext& /*ctx*/)
{
    stack--;
    stack--;
    if (*(stack + 2)) { *stack = *(stack + 1); }
    // else { *stack = *stack; }
}

void CEquationParser::CmpLower(double*& stack, const UFunctionContext& /*ctx*/)
{
    stack--;
    stack[0] = (stack[1] < stack[0] ? 1 : 0);
}

void CEquationParser::CmpGreater(double*& stack, const UFunctionContext& /*ctx*/)
{
    stack--;
    stack[0] = (stack[1] > stack[0] ? 1 : 0);
}

void CEquationParser::CmpLowerEqual(double*& stack, const UFunctionContext& /*ctx*/)
{
    stack--;
    stack[0] = (stack[1] <= stack[0] ? 1 : 0);
}

void CEquationParser::CmpGreaterEqual(double*& stack, const UFunctionContext& /*ctx*/)
{
    stack--;
    stack[0] = (stack[1] >= stack[0] ? 1 : 0);
}

void CEquationParser::CmpEqual(double*& stack, const UFunctionContext& /*ctx*/)
{
    stack--;
    stack[0] = (stack[1] == stack[0] ? 1 : 0);
}

void CEquationParser::CmpNotEqual(double*& stack, const UFunctionContext& /*ctx*/)
{
    stack--;
    stack[0] = (stack[1] != stack[0] ? 1 : 0);
}

void CEquationParser::BoolAnd(double*& stack, const UFunctionContext& /*ctx*/)
{
    stack--;
    stack[0] = (stack[1] != 0 && stack[0] != 0 ? 1 : 0);
}

void CEquationParser::BoolOr(double*& stack, const UFunctionContext& /*ctx*/)
{
    stack--;
    stack[0] = (stack[1] != 0 || stack[0] != 0 ? 1 : 0);
}

void CEquationParser::BoolNot(double*& stack, const UFunctionContext& /*ctx*/) { stack[0] = stack[0] != 0 ? 0 : 1; }

void CEquationParser::BoolXor(double*& stack, const UFunctionContext& /*ctx*/)
{
    stack--;
    stack[0] = (stack[1] != stack[0] ? 1 : 0);
}

void CEquationParser::LoadVal(double*& stack, const UFunctionContext& ctx) { *(++stack) = ctx.direct_value; }
void CEquationParser::LoadVar(double*& stack, const UFunctionContext& ctx) { *(++stack) = **(ctx.indirect_value); }
