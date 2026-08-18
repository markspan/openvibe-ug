/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "parameterObject.h"
#include <openvibe/parameters/parameterColorGradient.h>

namespace OpenViBE {
namespace Parameters {

class OV_PARAMETERS_OBJECT_EXPORT ParameterColorGradientObject: public ParameterObject
{
    Q_OBJECT
    Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)

public:
    ParameterColorGradientObject(const std::string& name, const std::string& description, const QString& value);
    ParameterColorGradientObject(ParameterTraits *param);
    virtual ~ParameterColorGradientObject();

    QString value() const; // "percentage:r,g,b; percentage:r,g,b; ..."
    void setValue(QString value);


signals:
    void valueChanged(const QString&);

private:
    ParameterColorGradient *m_colorGradient = nullptr;
};

DECLARE_PARAMETER_REGISTRATION(ParameterColorGradientObject)
} // namespace Parameters
} // namespace OpenViBE
