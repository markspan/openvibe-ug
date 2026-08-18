/* -------------------------------------------------------------------------- *
 *                                   Lepton                                   *
 * -------------------------------------------------------------------------- *
 * This is part of the Lepton expression parser originating from              *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org.               *
 *                                                                            *
 * Portions copyright (c) 2009 Stanford University and the Authors.           *
 * Authors: Peter Eastman                                                     *
 * Contributors:                                                              *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,    *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR      *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE  *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                     *
 * -------------------------------------------------------------------------- */

#include "ParsedExpression.h"
#include "CompiledExpression.h"
#include "ExpressionProgram.h"
#include "Operation.h"
#include <limits>
#include <vector>

using namespace Lepton;

ParsedExpression::ParsedExpression() : rootNode(ExpressionTreeNode()) {}

ParsedExpression::ParsedExpression(const ExpressionTreeNode& rootNode) : rootNode(rootNode) {}

const ExpressionTreeNode& ParsedExpression::getRootNode() const { return rootNode; }

double ParsedExpression::evaluate() const { return evaluate(getRootNode(), std::map<std::string, double>()); }

double ParsedExpression::evaluate(const std::map<std::string, double>& variables) const { return evaluate(getRootNode(), variables); }

double ParsedExpression::evaluate(const ExpressionTreeNode& node, const std::map<std::string, double>& variables)
{
    size_t numArgs = node.getChildren().size();
    std::vector<double> args(std::max(numArgs, size_t(1)));
    for (size_t i = 0; i < numArgs; ++i) { args[i] = evaluate(node.getChildren()[i], variables); }
    return node.getOperation().evaluate(&args[0], variables);
}

ParsedExpression ParsedExpression::optimize() const
{
    ExpressionTreeNode result = precalculateConstantSubexpressions(getRootNode());
    while (true)
    {
        ExpressionTreeNode simplified = substituteSimplerExpression(result);
        if (simplified == result) { break; }
        result = simplified;
    }
    return ParsedExpression(result);
}

ParsedExpression ParsedExpression::optimize(const std::map<std::string, double>& variables) const
{
    ExpressionTreeNode result = preevaluateVariables(getRootNode(), variables);
    result                    = precalculateConstantSubexpressions(result);
    while (true)
    {
        ExpressionTreeNode simplified = substituteSimplerExpression(result);
        if (simplified == result) { break; }
        result = simplified;
    }
    return ParsedExpression(result);
}

ExpressionTreeNode ParsedExpression::preevaluateVariables(const ExpressionTreeNode& node, const std::map<std::string, double>& variables)
{
    if (node.getOperation().getId() == Operation::VARIABLE)
    {
        const Operation::Variable& var = dynamic_cast<const Operation::Variable&>(node.getOperation());
        auto iter                      = variables.find(var.getName());
        if (iter == variables.end()) { return node; }
        return ExpressionTreeNode(new Operation::Constant(iter->second));
    }
    std::vector<ExpressionTreeNode> children(node.getChildren().size());
    for (size_t i = 0; i < children.size(); ++i) { children[i] = preevaluateVariables(node.getChildren()[i], variables); }
    return ExpressionTreeNode(node.getOperation().clone(), children);
}

ExpressionTreeNode ParsedExpression::precalculateConstantSubexpressions(const ExpressionTreeNode& node)
{
    std::vector<ExpressionTreeNode> children(node.getChildren().size());
    for (size_t i = 0; i < children.size(); ++i) { children[i] = precalculateConstantSubexpressions(node.getChildren()[i]); }
    ExpressionTreeNode result = ExpressionTreeNode(node.getOperation().clone(), children);
    if (node.getOperation().getId() == Operation::VARIABLE) { return result; }
    for (size_t i = 0; i < children.size(); ++i) { if (children[i].getOperation().getId() != Operation::CONSTANT) { return result; } }
    return ExpressionTreeNode(new Operation::Constant(evaluate(result, std::map<std::string, double>())));
}

ExpressionTreeNode ParsedExpression::substituteSimplerExpression(const ExpressionTreeNode& node)
{
    std::vector<ExpressionTreeNode> childs(node.getChildren().size());
    for (size_t i = 0; i < childs.size(); ++i) { childs[i] = substituteSimplerExpression(node.getChildren()[i]); }
    Operation::Id op1 = childs[0].getOperation().getId();
    Operation::Id op2 = childs[1].getOperation().getId();
    switch (node.getOperation().getId())
    {
        case Operation::ADD:
        {
            const double first  = getConstantValue(childs[0]);
            const double second = getConstantValue(childs[1]);
            if (first == 0.0) { return childs[1]; } // Add 0
            if (second == 0.0) { return childs[0]; }    // Add 0
            if (first == first) { return ExpressionTreeNode(new Operation::AddConstant(first), childs[1]); }                                // Add a constant
            if (second == second) { return ExpressionTreeNode(new Operation::AddConstant(second), childs[0]); }                             // Add a constant
            if (op2 == Operation::NEGATE) { return ExpressionTreeNode(new Operation::Subtract(), childs[0], childs[1].getChildren()[0]); }  // a+(-b) = a-b
            if (op1 == Operation::NEGATE) { return ExpressionTreeNode(new Operation::Subtract(), childs[1], childs[0].getChildren()[0]); }  // (-a)+b = b-a
            break;
        }
        case Operation::SUBTRACT:
        {
            if (childs[0] == childs[1]) { return ExpressionTreeNode(new Operation::Constant(0.0)); }                                        // Subtracting anything from itself is 0
            const double first = getConstantValue(childs[0]);
            if (first == 0.0) { return ExpressionTreeNode(new Operation::Negate(), childs[1]); }                                            // Subtract from 0
            const double second = getConstantValue(childs[1]);
            if (second == 0.0) { return childs[0]; }    // Subtract 0
            if (second == second) { return ExpressionTreeNode(new Operation::AddConstant(-second), childs[0]); }                            // Subtract a constant
            if (op2 == Operation::NEGATE) { return ExpressionTreeNode(new Operation::Add(), childs[0], childs[1].getChildren()[0]); }       // a-(-b) = a+b
            break;
        }
        case Operation::MULTIPLY:
        {
            double first  = getConstantValue(childs[0]);
            double second = getConstantValue(childs[1]);
            if (first == 0.0 || second == 0.0) { return ExpressionTreeNode(new Operation::Constant(0.0)); } // Multiply by 0
            if (first == 1.0) { return childs[1]; } // Multiply by 1
            if (second == 1.0) { return childs[0]; } // Multiply by 1
            if (op1 == Operation::CONSTANT)
            { // Multiply by a constant
                if (op2 == Operation::MULTIPLY_CONSTANT)
                { // Combine two multiplies into a single one
                    return ExpressionTreeNode(
                        new Operation::MultiplyConstant(first * dynamic_cast<const Operation::MultiplyConstant*>(&childs[1].getOperation())->getValue()),
                        childs[1].getChildren()[0]);
                }
                return ExpressionTreeNode(new Operation::MultiplyConstant(first), childs[1]);
            }
            if (op2 == Operation::CONSTANT)
            { // Multiply by a constant
                if (op1 == Operation::MULTIPLY_CONSTANT)
                { // Combine two multiplies into a single one
                    return ExpressionTreeNode(
                        new Operation::MultiplyConstant(second * dynamic_cast<const Operation::MultiplyConstant*>(&childs[0].getOperation())->getValue()),
                        childs[0].getChildren()[0]);
                }
                return ExpressionTreeNode(new Operation::MultiplyConstant(second), childs[0]);
            }
            if (op1 == Operation::NEGATE && op2 == Operation::NEGATE)
            { // The two negations cancel
                return ExpressionTreeNode(new Operation::Multiply(), childs[0].getChildren()[0], childs[1].getChildren()[0]);
            }
            if (op1 == Operation::NEGATE && op2 == Operation::MULTIPLY_CONSTANT)
            { // Negate the constant
                return ExpressionTreeNode(new Operation::Multiply(), childs[0].getChildren()[0],
                                          ExpressionTreeNode(
                                              new Operation::MultiplyConstant(
                                                  -dynamic_cast<const Operation::MultiplyConstant*>(&childs[1].getOperation())->getValue()),
                                              childs[1].getChildren()[0]));
            }
            if (op2 == Operation::NEGATE && op1 == Operation::MULTIPLY_CONSTANT)
            { // Negate the constant
                return ExpressionTreeNode(new Operation::Multiply(),
                                          ExpressionTreeNode(
                                              new Operation::MultiplyConstant(
                                                  -dynamic_cast<const Operation::MultiplyConstant*>(&childs[0].getOperation())->getValue()),
                                              childs[0].getChildren()[0]), childs[1].getChildren()[0]);
            }
            if (op1 == Operation::NEGATE)
            { // Pull the negation out so it can possibly be optimized further
                return ExpressionTreeNode(new Operation::Negate(), ExpressionTreeNode(new Operation::Multiply(), childs[0].getChildren()[0], childs[1]));
            }
            if (op2 == Operation::NEGATE)
            { // Pull the negation out so it can possibly be optimized further
                return ExpressionTreeNode(new Operation::Negate(), ExpressionTreeNode(new Operation::Multiply(), childs[0], childs[1].getChildren()[0]));
            }
            if (op2 == Operation::RECIPROCAL) { return ExpressionTreeNode(new Operation::Divide(), childs[0], childs[1].getChildren()[0]); }    // a*(1/b) = a/b
            if (op1 == Operation::RECIPROCAL) { return ExpressionTreeNode(new Operation::Divide(), childs[1], childs[0].getChildren()[0]); }    // (1/a)*b = b/a
            if (childs[0] == childs[1]) { return ExpressionTreeNode(new Operation::Square(), childs[0]); }                                      // x*x = square(x)
            if (op1 == Operation::SQUARE && childs[0].getChildren()[0] == childs[1]) { return ExpressionTreeNode(new Operation::Cube(), childs[1]); }   // x^3
            if (op2 == Operation::SQUARE && childs[1].getChildren()[0] == childs[0]) { return ExpressionTreeNode(new Operation::Cube(), childs[0]); }   // x^3
            break;
        }
        case Operation::DIVIDE:
        {
            if (childs[0] == childs[1]) { return ExpressionTreeNode(new Operation::Constant(1.0)); }        // Dividing anything from itself is 0 
            const double numerator = getConstantValue(childs[0]);
            if (numerator == 0.0) { return ExpressionTreeNode(new Operation::Constant(0.0)); }              // 0 divided by something
            if (numerator == 1.0) { return ExpressionTreeNode(new Operation::Reciprocal(), childs[1]); }    // 1 divided by something
            const double denominator = getConstantValue(childs[1]);
            if (denominator == 1.0) { return childs[0]; }                                                   // Divide by 1
            if (op2 == Operation::CONSTANT)
            {
                if (op1 == Operation::MULTIPLY_CONSTANT)
                { // Combine a multiply and a divide into one multiply
                    return ExpressionTreeNode(new Operation::MultiplyConstant
                                              (dynamic_cast<const Operation::MultiplyConstant*>(&childs[0].getOperation())->getValue() / denominator),
                                              childs[0].getChildren()[0]);
                }
                return ExpressionTreeNode(new Operation::MultiplyConstant(1.0 / denominator), childs[0]); // Replace a divide with a multiply
            }
            if (op1 == Operation::NEGATE && op2 == Operation::NEGATE)
            { // The two negations cancel
                return ExpressionTreeNode(new Operation::Divide(), childs[0].getChildren()[0], childs[1].getChildren()[0]);
            }
            if (op2 == Operation::NEGATE && op1 == Operation::MULTIPLY_CONSTANT)
            { // Negate the constant
                return ExpressionTreeNode(new Operation::Divide(), ExpressionTreeNode(
                                              new Operation::MultiplyConstant(
                                                  -dynamic_cast<const Operation::MultiplyConstant*>(&childs[0].getOperation())->getValue()),
                                              childs[0].getChildren()[0]), childs[1].getChildren()[0]);
            }
            if (op1 == Operation::NEGATE)
            { // Pull the negation out so it can possibly be optimized further
                return ExpressionTreeNode(new Operation::Negate(), ExpressionTreeNode(new Operation::Divide(), childs[0].getChildren()[0], childs[1]));
            }
            if (op2 == Operation::NEGATE)
            { // Pull the negation out so it can possibly be optimized further
                return ExpressionTreeNode(new Operation::Negate(), ExpressionTreeNode(new Operation::Divide(), childs[0], childs[1].getChildren()[0]));
            }
            if (childs[1].getOperation().getId() == Operation::RECIPROCAL)
            { // a/(1/b) = a*b
                return ExpressionTreeNode(new Operation::Multiply(), childs[0], childs[1].getChildren()[0]);
            }
            break;
        }
        case Operation::POWER:
        {
            double base = getConstantValue(childs[0]);
            if (base == 0.0) { return ExpressionTreeNode(new Operation::Constant(0.0)); } // 0 to any power is 0
            if (base == 1.0) { return ExpressionTreeNode(new Operation::Constant(1.0)); } // 1 to any power is 1
            double exponent = getConstantValue(childs[1]);
            if (exponent == 0.0) { return ExpressionTreeNode(new Operation::Constant(1.0)); } // x^0 = 1
            if (exponent == 1.0) { return childs[0]; } // x^1 = x
            if (exponent == -1.0) { return ExpressionTreeNode(new Operation::Reciprocal(), childs[0]); } // x^-1 = recip(x)
            if (exponent == 2.0) { return ExpressionTreeNode(new Operation::Square(), childs[0]); } // x^2 = square(x)
            if (exponent == 3.0) { return ExpressionTreeNode(new Operation::Cube(), childs[0]); } // x^3 = cube(x)
            if (exponent == 0.5) { return ExpressionTreeNode(new Operation::Sqrt(), childs[0]); } // x^0.5 = sqrt(x)
            if (exponent == exponent) { return ExpressionTreeNode(new Operation::PowerConstant(exponent), childs[0]); } // Constant power
            break;
        }
        case Operation::NEGATE:
        {
            if (op1 == Operation::MULTIPLY_CONSTANT)
            { // Combine a multiply and a negate into a single multiply
                return ExpressionTreeNode(
                    new Operation::MultiplyConstant(-dynamic_cast<const Operation::MultiplyConstant*>(&childs[0].getOperation())->getValue()),
                    childs[0].getChildren()[0]);
            }
            if (op1 == Operation::CONSTANT) { return ExpressionTreeNode(new Operation::Constant(-getConstantValue(childs[0]))); }   // Negate a constant
            if (op1 == Operation::NEGATE) { return childs[0].getChildren()[0]; }                                                    // The two negations cancel
            break;
        }
        case Operation::MULTIPLY_CONSTANT:
        {
            if (op1 == Operation::MULTIPLY_CONSTANT)
            { // Combine two multiplies into a single one
                return ExpressionTreeNode(
                    new Operation::MultiplyConstant(
                        dynamic_cast<const Operation::MultiplyConstant*>(&node.getOperation())->getValue() * dynamic_cast<const Operation::MultiplyConstant*>(&
                            childs[0].getOperation())->getValue()), childs[0].getChildren()[0]);
            }
            if (op1 == Operation::CONSTANT)
            { // Multiply two constants
                return ExpressionTreeNode(
                    new Operation::Constant(
                        dynamic_cast<const Operation::MultiplyConstant*>(&node.getOperation())->getValue() * getConstantValue(childs[0])));
            }
            if (op1 == Operation::NEGATE)
            { // Combine a multiply and a negate into a single multiply
                return ExpressionTreeNode(new Operation::MultiplyConstant(-dynamic_cast<const Operation::MultiplyConstant*>(&node.getOperation())->getValue()),
                                          childs[0].getChildren()[0]);
            }
            break;
        }
        default:
        {
            // If operation ID is not one of the above,
            // we don't substitute a simpler expression.
            break;
        }
    }
    return ExpressionTreeNode(node.getOperation().clone(), childs);
}

ParsedExpression ParsedExpression::differentiate(const std::string& variable) const { return differentiate(getRootNode(), variable); }

ExpressionTreeNode ParsedExpression::differentiate(const ExpressionTreeNode& node, const std::string& variable)
{
    std::vector<ExpressionTreeNode> childDerivs(node.getChildren().size());
    for (size_t i = 0; i < childDerivs.size(); ++i) { childDerivs[i] = differentiate(node.getChildren()[i], variable); }
    return node.getOperation().differentiate(node.getChildren(), childDerivs, variable);
}

double ParsedExpression::getConstantValue(const ExpressionTreeNode& node)
{
    if (node.getOperation().getId() == Operation::CONSTANT) { return dynamic_cast<const Operation::Constant&>(node.getOperation()).getValue(); }
    return std::numeric_limits<double>::quiet_NaN();
}

ExpressionProgram ParsedExpression::createProgram() const { return ExpressionProgram(*this); }

CompiledExpression ParsedExpression::createCompiledExpression() const { return CompiledExpression(*this); }

ParsedExpression ParsedExpression::renameVariables(const std::map<std::string, std::string>& replacements) const
{
    return ParsedExpression(renameNodeVariables(getRootNode(), replacements));
}

ExpressionTreeNode ParsedExpression::renameNodeVariables(const ExpressionTreeNode& node, const std::map<std::string, std::string>& replacements)
{
    if (node.getOperation().getId() == Operation::VARIABLE)
    {
        auto replace = replacements.find(node.getOperation().getName());
        if (replace != replacements.end()) { return ExpressionTreeNode(new Operation::Variable(replace->second)); }
    }
    std::vector<ExpressionTreeNode> children;
    for (size_t i = 0; i < node.getChildren().size(); ++i) { children.push_back(renameNodeVariables(node.getChildren()[i], replacements)); }
    return ExpressionTreeNode(node.getOperation().clone(), children);
}

std::ostream& Lepton::operator<<(std::ostream& out, const ExpressionTreeNode& node)
{
    if (node.getOperation().isInfixOperator() && node.getChildren().size() == 2)
    {
        out << "(" << node.getChildren()[0] << ")" << node.getOperation().getName() << "(" << node.getChildren()[1] << ")";
    }
    else if (node.getOperation().isInfixOperator() && node.getChildren().size() == 1)
    {
        out << "(" << node.getChildren()[0] << ")" << node.getOperation().getName();
    }
    else
    {
        out << node.getOperation().getName();
        if (!node.getChildren().empty())
        {
            out << "(";
            for (size_t i = 0; i < node.getChildren().size(); ++i)
            {
                if (i > 0) { out << ", "; }
                out << node.getChildren()[i];
            }
            out << ")";
        }
    }
    return out;
}

std::ostream& Lepton::operator<<(std::ostream& out, const ParsedExpression& exp)
{
    out << exp.getRootNode();
    return out;
}
