/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "parameterObject.h"
#include <openvibe/parameters/parameterInt.h>

namespace OpenViBE {
namespace Parameters {

class OV_PARAMETERS_OBJECT_EXPORT ParameterIntObject: public ParameterObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int min READ min WRITE setMin NOTIFY minChanged)
    Q_PROPERTY(int max READ max WRITE setMax NOTIFY maxChanged)

public:
    ParameterIntObject(const std::string& name, const std::string& description, int value);
    ParameterIntObject(const std::string& name, const std::string& description, int value, int min, int max);
    ParameterIntObject(ParameterTraits *param);
    virtual ~ParameterIntObject();

    int value() const;
    void setValue(int value);

    int min() const;
    void setMin(int value);

    int max() const;
    void setMax(int value);

signals:
    void valueChanged(const int&);
    void minChanged(const int&);
    void maxChanged(const int&);

private:
    ParameterInt *m_int = nullptr;
};

DECLARE_PARAMETER_REGISTRATION(ParameterIntObject)
} // namespace Parameters
} // namespace OpenViBE
