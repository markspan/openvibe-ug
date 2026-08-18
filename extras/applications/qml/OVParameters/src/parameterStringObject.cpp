/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "parameterStringObject.h"

namespace OpenViBE {
namespace Parameters {

ParameterStringObject::ParameterStringObject(const std::string& name, const std::string& description, const QString& value) : ParameterObject()
{
    m_string = new ParameterString(name, description, value.toStdString());
    m_param = m_string;
    m_connectionId = m_string->connect([this](std::any value) {
        const QString ss = QString::fromStdString(std::any_cast<std::string>(value));
        emit this->valueChanged(ss);
    });
}

ParameterStringObject::ParameterStringObject(ParameterTraits *param) : ParameterObject(param)
{
    m_deleteParam = false;
    m_string = dynamic_cast<ParameterString *>(param);
    m_param = m_string;
    m_connectionId = m_string->connect([this](std::any value) {
        const QString ss = QString::fromStdString(std::any_cast<std::string>(value));
        emit this->valueChanged(ss);
    });
}

ParameterStringObject::~ParameterStringObject()
{
    if (m_param) {
        m_param->cancelOnDestroy(m_destroyId);
        m_param->disconnect(m_connectionId);
        if (m_deleteParam)
            delete m_param;
    }
    m_string = nullptr;
}

QString ParameterStringObject::value() const 
{
    return QString::fromStdString(m_string->value());
}

void ParameterStringObject::setValue(QString value) 
{
    if(value != this->value()) {
        m_string->setValue(value.toStdString());
    }
}

REGISTER_PARAMETER_OBJECT("string", ParameterStringObject)
} // namespace Parameters
} // namespace OpenViBE
