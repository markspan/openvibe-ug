/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "parameterObject.h"
#include <openvibe/parameters/parameterMultiSelect.h>

namespace OpenViBE {
namespace Parameters {

class OV_PARAMETERS_OBJECT_EXPORT ParameterMultiSelectObject: public ParameterObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList available_values READ availableValues WRITE setAvailableValues NOTIFY availableValuesChanged)
    Q_PROPERTY(QStringList value READ value WRITE setValue NOTIFY valueChanged)


public:
    ParameterMultiSelectObject(std::vector<std::string> available_values);
    ParameterMultiSelectObject(const std::string& name, const std::string& description, const std::vector<std::string>& available_values, const std::vector<std::string>& value = {});
    ParameterMultiSelectObject(ParameterTraits *param);
    virtual ~ParameterMultiSelectObject();

    QStringList availableValues() const;
    void setAvailableValues(QStringList value);

    QStringList value() const;
    void setValue(QStringList value);

signals:
    void valueChanged(const QStringList&);
    void availableValuesChanged(const QStringList&);

private:
    ParameterMultiSelect *m_multiSelect = nullptr;
};

DECLARE_PARAMETER_REGISTRATION(ParameterMultiSelectObject)
} // namespace Parameters
} // namespace OpenViBE
