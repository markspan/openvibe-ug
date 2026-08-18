/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "parameterObject.h"
#include <openvibe/parameters/parameterString.h>

namespace OpenViBE {
namespace Parameters {

class OV_PARAMETERS_OBJECT_EXPORT ParameterStringObject: public ParameterObject
{
    Q_OBJECT
    Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)

public:
    ParameterStringObject(const std::string& name, const std::string& description, const QString& value);
    ParameterStringObject(ParameterTraits *param);
    virtual ~ParameterStringObject();

    QString value() const;
    void setValue(QString value);

signals:
    void valueChanged(const QString&);

private:
    ParameterString *m_string = nullptr;
};

DECLARE_PARAMETER_REGISTRATION(ParameterStringObject)
} // namespace Parameters
} // namespace OpenViBE
