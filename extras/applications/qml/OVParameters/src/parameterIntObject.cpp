/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "parameterIntObject.h"

namespace OpenViBE {
namespace Parameters {

ParameterIntObject::ParameterIntObject(const std::string& name, const std::string& description, int value): ParameterObject()
{
    m_int = new ParameterInt(name, description, value);
    m_param = m_int;
    m_connectionId = m_int->connect([this](std::any value) {
            emit this->valueChanged(std::any_cast<int>(value));
    });
    m_int->setMinNotifier([this](const int& value) {
            emit this->minChanged(value);
    });
    m_int->setMaxNotifier([this](const int& value) {
            emit this->maxChanged(value);
    });
}

ParameterIntObject::ParameterIntObject(const std::string& name, const std::string& description, int value, int min, int max): ParameterObject()
{
    m_int = new ParameterInt(name, description, value, min, max);
    m_param = m_int;
    m_connectionId = m_int->connect([this](std::any value) {
            emit this->valueChanged(std::any_cast<int>(value));
    });
    m_int->setMinNotifier([this](const int& value) {
            emit this->minChanged(value);
    });
    m_int->setMaxNotifier([this](const int& value) {
            emit this->maxChanged(value);
    });
}


ParameterIntObject::ParameterIntObject(ParameterTraits *param) : ParameterObject(param)
{
    m_deleteParam = false;
    m_int = dynamic_cast<ParameterInt *>(param);
    m_connectionId = m_int->connect([this](std::any value) {
            emit this->valueChanged(std::any_cast<int>(value));
    });
    m_int->setMinNotifier([this](const int& value) {
            emit this->minChanged(value);
    });
    m_int->setMaxNotifier([this](const int& value) {
            emit this->maxChanged(value);
    });
}

ParameterIntObject::~ParameterIntObject()
{
    if (m_param) {
        m_param->cancelOnDestroy(m_destroyId);
        m_param->disconnect(m_connectionId);
        if (m_deleteParam)
            delete m_param;
    }
    m_int = nullptr;
}

int ParameterIntObject::value() const 
{
    return m_int->value();
}

void ParameterIntObject::setValue(int value) 
{
    if(value != this->value()) {
        m_int->setValue(value);
    }
}

int ParameterIntObject::min() const
{
    return m_int->min();
}

void ParameterIntObject::setMin(int value)
{
    if(value != this->min()) {
        m_int->setMin(value);
    }
}

int ParameterIntObject::max() const
{
    return m_int->max();
}

void ParameterIntObject::setMax(int value)
{
    if(value != this->max()) {
        m_int->setMax(value);
    }
}

REGISTER_PARAMETER_OBJECT("int", ParameterIntObject)

} // namespace Parameters
} // namespace OpenViBE