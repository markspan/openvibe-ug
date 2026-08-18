/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "parameterColorGradientObject.h"

namespace OpenViBE {
namespace Parameters {

ParameterColorGradientObject::ParameterColorGradientObject(const std::string& name, const std::string& description, const QString& value) : ParameterObject()
{
    m_colorGradient = new ParameterColorGradient(name, description, value.toStdString());
    m_param = m_colorGradient;
    m_connectionId = m_colorGradient->connect([this](std::any value) {
        const QString ss = QString::fromStdString(std::any_cast<std::string>(value));
        emit this->valueChanged(ss);
    });
}

ParameterColorGradientObject::ParameterColorGradientObject(ParameterTraits *param) : ParameterObject(param)
{
    m_deleteParam = false;
    m_colorGradient = dynamic_cast<ParameterColorGradient *>(param);
    m_param = m_colorGradient;
    m_connectionId = m_colorGradient->connect([this](std::any value) {
        const QString ss = QString::fromStdString(std::any_cast<std::string>(value));
        emit this->valueChanged(ss);
    });
}

ParameterColorGradientObject::~ParameterColorGradientObject()
{
    if (m_param) {
        m_param->cancelOnDestroy(m_destroyId);
        m_param->disconnect(m_connectionId);
        if (m_deleteParam)
            delete m_param;
    }
    m_colorGradient = nullptr;
}

QString ParameterColorGradientObject::value() const 
{
    return QString::fromStdString(m_colorGradient->value());
}

void ParameterColorGradientObject::setValue(QString value) 
{
    if(value != this->value()) {
        m_colorGradient->setValue(value.toStdString());
    }
}

REGISTER_PARAMETER_OBJECT("colorgradient", ParameterColorGradientObject)
} // namespace Parameters
} // namespace OpenViBE
