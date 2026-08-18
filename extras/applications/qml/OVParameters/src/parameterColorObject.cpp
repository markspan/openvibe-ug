/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "parameterColorObject.h"

namespace OpenViBE {
namespace Parameters {

ParameterColorObject::ParameterColorObject(const std::string& name, const std::string& description, const QString& value) : ParameterObject()
{
    m_color = new ParameterColor(name, description, value.toStdString());
    m_param = m_color;
    m_connectionId = m_color->connect([this](std::any value) {
        const QString ss = QString::fromStdString(std::any_cast<std::string>(value));
        emit this->valueChanged(ss);
    });
}

ParameterColorObject::ParameterColorObject(ParameterTraits *param) : ParameterObject(param)
{
    m_deleteParam = false;
    m_color = dynamic_cast<ParameterColor *>(param);
    m_param = m_color;
    m_connectionId = m_color->connect([this](std::any value) {
        const QString ss = QString::fromStdString(std::any_cast<std::string>(value));
        emit this->valueChanged(ss);
    });
}

ParameterColorObject::~ParameterColorObject()
{
    if (m_param) {
        m_param->cancelOnDestroy(m_destroyId);
        m_param->disconnect(m_connectionId);
        if (m_deleteParam)
            delete m_param;
    }
    m_color = nullptr;
}

QString ParameterColorObject::value() const 
{
    return QString::fromStdString(m_color->value());
}

void ParameterColorObject::setValue(QString value) 
{
    if(value != this->value()) {
        m_color->setValue(value.toStdString());
    }
}

REGISTER_PARAMETER_OBJECT("color", ParameterColorObject)
} // namespace Parameters
} // namespace OpenViBE
