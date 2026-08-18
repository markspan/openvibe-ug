///-------------------------------------------------------------------------------------------------
/// 
/// \file CEquationParser.hpp
/// \brief Defines for the Equation Parser.
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

#include <toolkit/ovtk_all.h>

#include <boost/spirit/include/classic_symbols.hpp>
#include <boost/spirit/include/classic_ast.hpp>

/**
* Enum of parent nodes identifiers.
*/
enum EByteCodes
{
    OP_NEG, OP_ADD, OP_SUB, OP_MUL, OP_DIV,
    OP_ABS, OP_ACOS, OP_ASIN, OP_ATAN, OP_CEIL, OP_COS, OP_EXP, OP_FLOOR, OP_LOG, OP_LOG10, OP_POW, OP_RAND, OP_SIN, OP_SQRT, OP_TAN,
    OP_IF_THEN_ELSE, OP_CMP_L, OP_CMP_G, OP_CMP_LE, OP_CMP_GE, OP_CMP_E, OP_CMP_NE,
    OP_BOOL_AND, OP_BOOL_OR, OP_BOOL_NOT, OP_BOOL_XOR,

    //used for special tree recognition

    //The equation is not a special one
    OP_USERDEF,
    //Identity
    OP_NONE,
    //X*X
    OP_X2
};


inline std::string toString(const EByteCodes code)
{
    switch (code) {
        case OP_NEG: return "-";
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";

        case OP_ABS: return "abs";
        case OP_ACOS: return "cos";
        case OP_ASIN: return "sin";
        case OP_ATAN: return "atan";
        case OP_CEIL: return "ceil";
        case OP_COS: return "cos";
        case OP_EXP: return "exp";
        case OP_FLOOR: return "floor";
        case OP_LOG: return "log";
        case OP_LOG10: return "log10";
        case OP_POW: return "pow";
        case OP_RAND: return "rand";
        case OP_SIN: return "sin";
        case OP_SQRT: return "sqrt";
        case OP_TAN: return "tan";

        case OP_IF_THEN_ELSE: return "?:";

        case OP_CMP_L: return "<";
        case OP_CMP_G: return ">";
        case OP_CMP_LE: return "<=";
        case OP_CMP_GE: return ">=";
        case OP_CMP_E: return "==";
        case OP_CMP_NE: return "!=";

        case OP_BOOL_AND: return "&";
        case OP_BOOL_OR: return "|";
        case OP_BOOL_NOT: return "!";
        case OP_BOOL_XOR: return "^";

        case OP_USERDEF: return "UserDefined";
        case OP_NONE: return "None";
        case OP_X2: return "X^2";

        default: return "UnknownOp";
    }
}

enum EVariables
{
    OP_VAR_X=0, OP_VAR_A, OP_VAR_B, OP_VAR_C, OP_VAR_D, OP_VAR_E, OP_VAR_F, OP_VAR_G, OP_VAR_H,
    OP_VAR_I, OP_VAR_J, OP_VAR_K, OP_VAR_L, OP_VAR_M, OP_VAR_N, OP_VAR_O, OP_VAR_P,
};

/**
* Symbols table for unary functions.
*
*/

struct SUnaryFunctionSymbols : boost::spirit::classic::symbols<uint64_t>
{
    SUnaryFunctionSymbols()
    {
        add
                ("abs", OP_ABS)
                ("acos", OP_ACOS)
                ("asin", OP_ASIN)
                ("atan", OP_ATAN)
                ("ceil", OP_CEIL)
                ("cos", OP_COS)
                ("exp", OP_EXP)
                ("floor", OP_FLOOR)
                ("log", OP_LOG)
                ("log10", OP_LOG10)
                ("rand", OP_RAND)
                ("sin", OP_SIN)
                ("sqrt", OP_SQRT)
                ("tan", OP_TAN);
    }
};

/**
* Symbols table for binary functions.
*
*/
struct SBinaryFunctionSymbols : boost::spirit::classic::symbols<uint64_t>
{
    SBinaryFunctionSymbols() { add("pow", OP_POW); }
};

/**
* Symbol tables for unary boolean operators
*
*/
struct SUnaryBooleanFunctionSymbols : boost::spirit::classic::symbols<uint64_t>
{
    SUnaryBooleanFunctionSymbols() { add("!", OP_BOOL_NOT); }
};

/**
* Symbol tables for binary boolean operators
*
*/
struct SBinaryBoolean1FunctionSymbols : boost::spirit::classic::symbols<uint64_t>
{
    SBinaryBoolean1FunctionSymbols() { add("&&", OP_BOOL_AND)("&", OP_BOOL_AND); }
};

/**
* Symbol tables for binary boolean operators
*
*/
struct SBinaryBoolean2FunctionSymbols : boost::spirit::classic::symbols<uint64_t>
{
    SBinaryBoolean2FunctionSymbols() { add("~", OP_BOOL_XOR)("^", OP_BOOL_XOR); }
};

/**
* Symbol tables for binary boolean operators
*
*/
struct SBinaryBoolean3FunctionSymbols : boost::spirit::classic::symbols<uint64_t>
{
    SBinaryBoolean3FunctionSymbols() { add("||", OP_BOOL_OR)("|", OP_BOOL_OR); }
};

/**
* Symbols table for comparison 1 functions.
*
*/
struct SComparison1FunctionSymbols : boost::spirit::classic::symbols<uint64_t>
{
    SComparison1FunctionSymbols() { add("<", OP_CMP_L)(">", OP_CMP_G)("<=", OP_CMP_LE)(">=", OP_CMP_GE); }
};

/**
* Symbols table for comparison 2 functions.
*
*/
struct SComparison2FunctionSymbols : boost::spirit::classic::symbols<uint64_t>
{
    SComparison2FunctionSymbols() { add("==", OP_CMP_E)("!=", OP_CMP_NE)("<>", OP_CMP_NE); }
};

/**
* Symbols table for mathematical constants.
*
*/
struct SMathConstantSymbols : boost::spirit::classic::symbols<double>
{
    SMathConstantSymbols()
    {
        add
                ("m_pi", 3.14159265358979323846)("m_pi_2", 1.57079632679489661923)("m_pi_4", 0.78539816339744830962)
                ("m_1_pi", 0.31830988618379067154)("m_2_pi", 0.63661977236758134308)
                ("m_2_sqrt", 1.12837916709551257390)("m_sqrt2", 1.41421356237309504880)("m_sqrt1_2", 0.70710678118654752440)
                ("m_e", 2.7182818284590452354)("m_log2e", 1.4426950408889634074)("m_log10e", 0.43429448190325182765)
                ("m_ln", 0.69314718055994530942)("m_ln10", 2.30258509299404568402);
    }
};

/**
* Symbols table for variables.
*
*/
struct SVariableSymbols : boost::spirit::classic::symbols<uint64_t>
{
    SVariableSymbols()
    {
        add("x", OP_VAR_X)("a", OP_VAR_A)("b", OP_VAR_B)("c", OP_VAR_C)("d", OP_VAR_D)("e", OP_VAR_E)("f", OP_VAR_F)("g", OP_VAR_G)("h", OP_VAR_H)(
            "i", OP_VAR_I)("j", OP_VAR_J)("k", OP_VAR_K)("l", OP_VAR_L)("m", OP_VAR_M)("n", OP_VAR_N)("o", OP_VAR_O)("p", OP_VAR_P);
    }
};

static SUnaryFunctionSymbols unaryFunction_p;
static SBinaryFunctionSymbols binaryFunction_p;
static SUnaryBooleanFunctionSymbols unaryBooleanFunction_p;
static SBinaryBoolean1FunctionSymbols binaryBoolean1Function_p;
static SBinaryBoolean2FunctionSymbols binaryBoolean2Function_p;
static SBinaryBoolean3FunctionSymbols binaryBoolean3Function_p;
static SComparison1FunctionSymbols comparison1Function_p;
static SComparison2FunctionSymbols comparison2Function_p;
static SMathConstantSymbols mathConstant_p;
static SVariableSymbols variable_p;

/**
* The parser's grammar.
*/
struct SEquationGrammar : boost::spirit::classic::grammar<SEquationGrammar>
{
    static const int REAL_ID       = 1;
    static const int VARIABLE_ID   = 2;
    static const int FUNCTION_ID   = 3;
    static const int CONSTANT_ID   = 4;
    static const int FACTOR_ID     = 6;
    static const int TERM_ID       = 7;
    static const int EXPRESSION_ID = 8;
    static const int IFTHEN_ID     = 9;
    static const int COMPARISON_ID = 10;
    static const int BOOLEAN_ID    = 11;

    template <typename TScanner>
    struct definition
    {
        explicit definition(SEquationGrammar const& /*grammar*/)
        {
            using namespace boost::spirit::classic;
            real     = leaf_node_d[real_p];
            variable = leaf_node_d[as_lower_d[variable_p]];
            constant = leaf_node_d[as_lower_d[mathConstant_p]];

            function = (root_node_d[as_lower_d[unaryFunction_p]] >> no_node_d[ch_p('(')] >> ifthen
                        >> no_node_d[ch_p(')')]) | (root_node_d[as_lower_d[binaryFunction_p]] >> no_node_d[ch_p('(')]
                                                    >> infix_node_d[(ifthen >> ',' >> ifthen)] >> no_node_d[ch_p(')')]);

            factor = (function | constant | variable | real) | inner_node_d['(' >> expression >> ')'] | inner_node_d['(' >> ifthen >> ')']
                     | (root_node_d[ch_p('-')] >> factor) | (root_node_d[ch_p('+')] >> factor);

            boolean     = (root_node_d[unaryBooleanFunction_p] >> factor) | factor;
            term        = boolean >> *((root_node_d[ch_p('*')] >> boolean) | (root_node_d[ch_p('/')] >> boolean));
            expression  = term >> *((root_node_d[ch_p('+')] >> term) | (root_node_d[ch_p('-')] >> term));
            comparison1 = (expression >> root_node_d[comparison1Function_p] >> expression) | expression;
            comparison2 = (comparison1 >> root_node_d[comparison2Function_p] >> comparison1) | comparison1;
            boolean1    = (comparison2 >> root_node_d[binaryBoolean1Function_p] >> comparison2) | comparison2;
            boolean2    = (boolean1 >> root_node_d[binaryBoolean2Function_p] >> boolean1) | boolean1;
            boolean3    = (boolean2 >> root_node_d[binaryBoolean3Function_p] >> boolean2) | boolean2;
            ifthen      = (boolean3 >> root_node_d[ch_p('?')] >> boolean3 >> no_node_d[ch_p(':')] >> boolean3) | boolean3;
        }

        boost::spirit::classic::rule<TScanner, boost::spirit::classic::parser_context<>, boost::spirit::classic::parser_tag<BOOLEAN_ID>> boolean;
        boost::spirit::classic::rule<TScanner, boost::spirit::classic::parser_context<>, boost::spirit::classic::parser_tag<BOOLEAN_ID>> boolean1;
        boost::spirit::classic::rule<TScanner, boost::spirit::classic::parser_context<>, boost::spirit::classic::parser_tag<BOOLEAN_ID>> boolean2;
        boost::spirit::classic::rule<TScanner, boost::spirit::classic::parser_context<>, boost::spirit::classic::parser_tag<BOOLEAN_ID>> boolean3;
        boost::spirit::classic::rule<TScanner, boost::spirit::classic::parser_context<>, boost::spirit::classic::parser_tag<COMPARISON_ID>> comparison1;
        boost::spirit::classic::rule<TScanner, boost::spirit::classic::parser_context<>, boost::spirit::classic::parser_tag<COMPARISON_ID>> comparison2;
        boost::spirit::classic::rule<TScanner, boost::spirit::classic::parser_context<>, boost::spirit::classic::parser_tag<IFTHEN_ID>> ifthen;
        boost::spirit::classic::rule<TScanner, boost::spirit::classic::parser_context<>, boost::spirit::classic::parser_tag<EXPRESSION_ID>> expression;
        boost::spirit::classic::rule<TScanner, boost::spirit::classic::parser_context<>, boost::spirit::classic::parser_tag<TERM_ID>> term;
        boost::spirit::classic::rule<TScanner, boost::spirit::classic::parser_context<>, boost::spirit::classic::parser_tag<FACTOR_ID>> factor;
        boost::spirit::classic::rule<TScanner, boost::spirit::classic::parser_context<>, boost::spirit::classic::parser_tag<REAL_ID>> real;
        boost::spirit::classic::rule<TScanner, boost::spirit::classic::parser_context<>, boost::spirit::classic::parser_tag<VARIABLE_ID>> variable;
        boost::spirit::classic::rule<TScanner, boost::spirit::classic::parser_context<>, boost::spirit::classic::parser_tag<FUNCTION_ID>> function;
        boost::spirit::classic::rule<TScanner, boost::spirit::classic::parser_context<>, boost::spirit::classic::parser_tag<CONSTANT_ID>> constant;

        boost::spirit::classic::rule<TScanner, boost::spirit::classic::parser_context<>, boost::spirit::classic::parser_tag<IFTHEN_ID>> const& start() const
        {
            return ifthen;
        }
    };
};
