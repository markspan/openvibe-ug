/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "parameterObject.h"
#include <openvibe/parameters/parameterListInt.h>

namespace OpenViBE {
namespace Parameters {

class OV_PARAMETERS_OBJECT_EXPORT ParameterListIntObject: public ParameterObject
{
    Q_OBJECT
    Q_PROPERTY(QList<int> available_values READ availableValues WRITE setAvailableValues NOTIFY availableValuesChanged)
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)

public:
    ParameterListIntObject(std::vector<int> available_values);
    ParameterListIntObject(const std::string& name, const std::string& description, const std::vector<int>& available_values, int value = 0);
    ParameterListIntObject(ParameterTraits *param);
    virtual ~ParameterListIntObject();

    QList<int> availableValues() const;
    void setAvailableValues(QList<int> values);

    int value() const;
    void setValue(int value);

    int index() const;
    void setIndex(int index);

signals:
    void indexChanged(int);
    void valueChanged(int);
    void availableValuesChanged(const QList<int>&);

private:
    ParameterListInt *m_listInt = nullptr;
};

DECLARE_PARAMETER_REGISTRATION(ParameterListIntObject)
} // namespace Parameters
} // namespace OpenViBE
