/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "parameterBoolObject.h"

namespace OpenViBE {
namespace Parameters {

ParameterBoolObject::ParameterBoolObject(const std::string& name, const std::string& description, bool value) : ParameterObject()
{
    m_bool = new ParameterBool(name, description, value);
    m_param = m_bool;
    m_connectionId = m_bool->connect([this](std::any value) {
        emit this->valueChanged(std::any_cast<bool>(value));
    });
}
ParameterBoolObject::ParameterBoolObject(ParameterTraits *param) : ParameterObject(param)
{
    m_deleteParam = false;
    m_bool = dynamic_cast<ParameterBool *>(param);
    m_connectionId = m_bool->connect([this](std::any value) {
        emit this->valueChanged(std::any_cast<bool>(value));
    });
}

ParameterBoolObject::~ParameterBoolObject()
{
    if (m_param) {
        m_param->cancelOnDestroy(m_destroyId);
        m_param->disconnect(m_connectionId);
        if (m_deleteParam)
            delete m_param;
    }
    m_bool = nullptr;
}

bool ParameterBoolObject::value() const 
{
    return m_bool->value();
}

void ParameterBoolObject::setValue(bool value) 
{
    if(value != this->value()) {
        m_bool->setValue(value);
    }
}

REGISTER_PARAMETER_OBJECT("bool", ParameterBoolObject)
} // namespace Parameters
} // namespace OpenViBE
