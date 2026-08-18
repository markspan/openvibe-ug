/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "parameterObject.h"
#include <openvibe/parameters/parameterColor.h>

namespace OpenViBE {
namespace Parameters {

class OV_PARAMETERS_OBJECT_EXPORT ParameterColorObject: public ParameterObject
{
    Q_OBJECT
    Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)

public:
    ParameterColorObject(const std::string& name, const std::string& description, const QString& value);
    ParameterColorObject(ParameterTraits *param);
    virtual ~ParameterColorObject();

    QString value() const; // "r,g,b"
    void setValue(QString value);


signals:
    void valueChanged(const QString&);

private:
    ParameterColor *m_color = nullptr;
};

DECLARE_PARAMETER_REGISTRATION(ParameterColorObject)
} // namespace Parameters
} // namespace OpenViBE
