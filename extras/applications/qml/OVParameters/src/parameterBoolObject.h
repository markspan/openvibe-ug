/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once 

#include "parameterObject.h"
#include <openvibe/parameters/parameterBool.h>

namespace OpenViBE {
namespace Parameters {

class OV_PARAMETERS_OBJECT_EXPORT ParameterBoolObject: public ParameterObject
{
    Q_OBJECT
    Q_PROPERTY(bool value READ value WRITE setValue NOTIFY valueChanged)

public:
    ParameterBoolObject(const std::string& name, const std::string& description, bool value);
    ParameterBoolObject(ParameterTraits *param);
    virtual ~ParameterBoolObject();

    bool value() const;
    void setValue(bool value);

signals:
    void valueChanged(const bool&);

private:
    ParameterBool *m_bool = nullptr;
};

DECLARE_PARAMETER_REGISTRATION(ParameterBoolObject)
} // namespace Parameters
} // namespace OpenViBE
