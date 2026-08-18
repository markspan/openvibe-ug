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

#include "Operation.h"
#include "ExpressionTreeNode.h"
#include "MSVC_erfc.h"

using namespace Lepton;

double Operation::Erf::evaluate(double* args, const std::map<std::string, double>& /*variable*/) const { return erf(args[0]); }

double Operation::Erfc::evaluate(double* args, const std::map<std::string, double>& /*variable*/) const { return erfc(args[0]); }

ExpressionTreeNode Operation::Constant::differentiate(const std::vector<ExpressionTreeNode>& /*children*/,
                                                      const std::vector<ExpressionTreeNode>& /*childDerivs*/, const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Constant(0.0));
}

ExpressionTreeNode Operation::Variable::differentiate(const std::vector<ExpressionTreeNode>& /*children*/,
                                                      const std::vector<ExpressionTreeNode>& /*childDerivs*/, const std::string& variable) const
{
    if (variable == name) { return ExpressionTreeNode(new Constant(1.0)); }
    return ExpressionTreeNode(new Constant(0.0));
}

ExpressionTreeNode Operation::Custom::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                    const std::string& /*variable*/) const
{
    if (function->getNumArguments() == 0) { return ExpressionTreeNode(new Constant(0.0)); }
    ExpressionTreeNode result = ExpressionTreeNode(new Multiply(), ExpressionTreeNode(new Custom(*this, 0), children), childDerivs[0]);
    for (int i = 1; i < getNumArguments(); ++i)
    {
        result = ExpressionTreeNode(new Add(),
                                    result,
                                    ExpressionTreeNode(new Multiply(), ExpressionTreeNode(new Custom(*this, i), children), childDerivs[i]));
    }
    return result;
}

ExpressionTreeNode Operation::Add::differentiate(const std::vector<ExpressionTreeNode>& /*children*/, const std::vector<ExpressionTreeNode>& childDerivs,
                                                 const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Add(), childDerivs[0], childDerivs[1]);
}

ExpressionTreeNode Operation::Subtract::differentiate(const std::vector<ExpressionTreeNode>& /*children*/, const std::vector<ExpressionTreeNode>& childDerivs,
                                                      const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Subtract(), childDerivs[0], childDerivs[1]);
}

ExpressionTreeNode Operation::Multiply::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                      const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Add(),
                              ExpressionTreeNode(new Multiply(), children[0], childDerivs[1]),
                              ExpressionTreeNode(new Multiply(), children[1], childDerivs[0]));
}

ExpressionTreeNode Operation::Divide::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                    const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Divide(),
                              ExpressionTreeNode(new Subtract(),
                                                 ExpressionTreeNode(new Multiply(), children[1], childDerivs[0]),
                                                 ExpressionTreeNode(new Multiply(), children[0], childDerivs[1])),
                              ExpressionTreeNode(new Square(), children[1]));
}

ExpressionTreeNode Operation::Power::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                   const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Add(),
                              ExpressionTreeNode(new Multiply(),
                                                 ExpressionTreeNode(new Multiply(),
                                                                    children[1],
                                                                    ExpressionTreeNode(new Power(),
                                                                                       children[0], ExpressionTreeNode(new AddConstant(-1.0), children[1]))),
                                                 childDerivs[0]),
                              ExpressionTreeNode(new Multiply(),
                                                 ExpressionTreeNode(new Multiply(),
                                                                    ExpressionTreeNode(new Log(), children[0]),
                                                                    ExpressionTreeNode(new Power(), children[0], children[1])),
                                                 childDerivs[1]));
}

ExpressionTreeNode Operation::Negate::differentiate(const std::vector<ExpressionTreeNode>& /*children*/, const std::vector<ExpressionTreeNode>& childDerivs,
                                                    const std::string& /*variable*/) const { return ExpressionTreeNode(new Negate(), childDerivs[0]); }

ExpressionTreeNode Operation::Sqrt::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                  const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(), ExpressionTreeNode(new MultiplyConstant(0.5),
                                                                 ExpressionTreeNode(new Reciprocal(),
                                                                                    ExpressionTreeNode(new Sqrt(), children[0]))),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::Exp::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                 const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(), ExpressionTreeNode(new Exp(), children[0]), childDerivs[0]);
}

ExpressionTreeNode Operation::Log::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                 const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(), ExpressionTreeNode(new Reciprocal(), children[0]), childDerivs[0]);
}

ExpressionTreeNode Operation::Sin::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                 const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(), ExpressionTreeNode(new Cos(), children[0]), childDerivs[0]);
}

ExpressionTreeNode Operation::Cos::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                 const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(),
                              ExpressionTreeNode(new Negate(),
                                                 ExpressionTreeNode(new Sin(), children[0])),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::Sec::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                 const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(),
                              ExpressionTreeNode(new Multiply(),
                                                 ExpressionTreeNode(new Sec(), children[0]),
                                                 ExpressionTreeNode(new Tan(), children[0])),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::Csc::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                 const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(),
                              ExpressionTreeNode(new Negate(),
                                                 ExpressionTreeNode(new Multiply(),
                                                                    ExpressionTreeNode(new Csc(), children[0]),
                                                                    ExpressionTreeNode(new Cot(), children[0]))),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::Tan::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                 const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(),
                              ExpressionTreeNode(new Square(),
                                                 ExpressionTreeNode(new Sec(), children[0])),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::Cot::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                 const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(),
                              ExpressionTreeNode(new Negate(),
                                                 ExpressionTreeNode(new Square(),
                                                                    ExpressionTreeNode(new Csc(), children[0]))),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::Asin::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                  const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(),
                              ExpressionTreeNode(new Reciprocal(),
                                                 ExpressionTreeNode(new Sqrt(),
                                                                    ExpressionTreeNode(new Subtract(),
                                                                                       ExpressionTreeNode(new Constant(1.0)),
                                                                                       ExpressionTreeNode(new Square(), children[0])))),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::Acos::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                  const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(),
                              ExpressionTreeNode(new Negate(),
                                                 ExpressionTreeNode(new Reciprocal(),
                                                                    ExpressionTreeNode(new Sqrt(),
                                                                                       ExpressionTreeNode(new Subtract(),
                                                                                                          ExpressionTreeNode(new Constant(1.0)),
                                                                                                          ExpressionTreeNode(new Square(), children[0]))))),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::Atan::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                  const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(),
                              ExpressionTreeNode(new Reciprocal(),
                                                 ExpressionTreeNode(new AddConstant(1.0),
                                                                    ExpressionTreeNode(new Square(), children[0]))),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::Sinh::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                  const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(), ExpressionTreeNode(new Cosh(), children[0]), childDerivs[0]);
}

ExpressionTreeNode Operation::Cosh::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                  const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(), ExpressionTreeNode(new Sinh(), children[0]), childDerivs[0]);
}

ExpressionTreeNode Operation::Tanh::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                  const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(),
                              ExpressionTreeNode(new Subtract(),
                                                 ExpressionTreeNode(new Constant(1.0)),
                                                 ExpressionTreeNode(new Square(),
                                                                    ExpressionTreeNode(new Tanh(), children[0]))),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::Erf::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                 const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(),
                              ExpressionTreeNode(new Multiply(),
                                                 ExpressionTreeNode(new Constant(2.0 / sqrt(M_PI))),
                                                 ExpressionTreeNode(new Exp(),
                                                                    ExpressionTreeNode(new Negate(),
                                                                                       ExpressionTreeNode(new Square(), children[0])))),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::Erfc::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                  const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(),
                              ExpressionTreeNode(new Multiply(),
                                                 ExpressionTreeNode(new Constant(-2.0 / sqrt(M_PI))),
                                                 ExpressionTreeNode(new Exp(),
                                                                    ExpressionTreeNode(new Negate(),
                                                                                       ExpressionTreeNode(new Square(), children[0])))),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::Step::differentiate(const std::vector<ExpressionTreeNode>& /*children*/, const std::vector<ExpressionTreeNode>& /*childDerivs*/,
                                                  const std::string& /*variable*/) const { return ExpressionTreeNode(new Constant(0.0)); }

ExpressionTreeNode Operation::Delta::differentiate(const std::vector<ExpressionTreeNode>& /*children*/, const std::vector<ExpressionTreeNode>& /*childDerivs*/,
                                                   const std::string& /*variable*/) const { return ExpressionTreeNode(new Constant(0.0)); }

ExpressionTreeNode Operation::Square::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                    const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(),
                              ExpressionTreeNode(new MultiplyConstant(2.0),
                                                 children[0]),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::Cube::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                  const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(),
                              ExpressionTreeNode(new MultiplyConstant(3.0),
                                                 ExpressionTreeNode(new Square(), children[0])),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::Reciprocal::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                        const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(),
                              ExpressionTreeNode(new Negate(),
                                                 ExpressionTreeNode(new Reciprocal(),
                                                                    ExpressionTreeNode(new Square(), children[0]))),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::AddConstant::differentiate(const std::vector<ExpressionTreeNode>& /*children*/,
                                                         const std::vector<ExpressionTreeNode>& childDerivs, const std::string& /*variable*/) const
{
    return childDerivs[0];
}

ExpressionTreeNode Operation::MultiplyConstant::differentiate(const std::vector<ExpressionTreeNode>& /*children*/,
                                                              const std::vector<ExpressionTreeNode>& childDerivs, const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new MultiplyConstant(value), childDerivs[0]);
}

ExpressionTreeNode Operation::PowerConstant::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                           const std::string& /*variable*/) const
{
    return ExpressionTreeNode(new Multiply(), ExpressionTreeNode(new MultiplyConstant(value), ExpressionTreeNode(new PowerConstant(value - 1), children[0])),
                              childDerivs[0]);
}

ExpressionTreeNode Operation::Min::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                 const std::string& /*variable*/) const
{
    const ExpressionTreeNode step(new Step(), ExpressionTreeNode(new Subtract(), children[0], children[1]));
    return ExpressionTreeNode(new Subtract(),
                              ExpressionTreeNode(new Multiply(), childDerivs[1], step),
                              ExpressionTreeNode(new Multiply(), childDerivs[0],
                                                 ExpressionTreeNode(new AddConstant(-1), step)));
}

ExpressionTreeNode Operation::Max::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                 const std::string& /*variable*/) const
{
    const ExpressionTreeNode step(new Step(), ExpressionTreeNode(new Subtract(), children[0], children[1]));
    return ExpressionTreeNode(new Subtract(),
                              ExpressionTreeNode(new Multiply(), childDerivs[0], step),
                              ExpressionTreeNode(new Multiply(), childDerivs[1],
                                                 ExpressionTreeNode(new AddConstant(-1), step)));
}

ExpressionTreeNode Operation::Abs::differentiate(const std::vector<ExpressionTreeNode>& children, const std::vector<ExpressionTreeNode>& childDerivs,
                                                 const std::string& /*variable*/) const
{
    const ExpressionTreeNode step(new Step(), children[0]);
    return ExpressionTreeNode(new Multiply(), childDerivs[0], ExpressionTreeNode(new AddConstant(-1), ExpressionTreeNode(new MultiplyConstant(2), step)));
}
