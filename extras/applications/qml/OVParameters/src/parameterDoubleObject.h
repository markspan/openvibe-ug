/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "parameterObject.h"
#include <openvibe/parameters/parameterDouble.h>

namespace OpenViBE {
namespace Parameters {

class OV_PARAMETERS_OBJECT_EXPORT ParameterDoubleObject: public ParameterObject
{
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double min READ min WRITE setMin NOTIFY minChanged)
    Q_PROPERTY(double max READ max WRITE setMax NOTIFY maxChanged)
    Q_PROPERTY(int decimals READ decimals NOTIFY decimalsChanged)

public:
    ParameterDoubleObject(const std::string& name, const std::string& description, double value);
    ParameterDoubleObject(const std::string& name, const std::string& description, double value, double min, double max);
    ParameterDoubleObject(const std::string& name, const std::string& description, double value, double min, double max, int decimals);
    ParameterDoubleObject(ParameterTraits *param);
    virtual ~ParameterDoubleObject();

    double value() const;
    void setValue(double value);

    double min() const;
    void setMin(double value);

    double max() const;
    void setMax(double value);

    int decimals() const;

signals:
    void valueChanged(const double&);
    void minChanged(const double&);
    void maxChanged(const double&);
    void decimalsChanged(const int&);

private:
    ParameterDouble *m_double = nullptr;
};

DECLARE_PARAMETER_REGISTRATION(ParameterDoubleObject)
} // namespace Parameters
} // namespace OpenViBE
