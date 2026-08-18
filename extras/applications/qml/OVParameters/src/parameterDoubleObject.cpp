/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "parameterDoubleObject.h"

namespace OpenViBE {
namespace Parameters {

ParameterDoubleObject::ParameterDoubleObject(const std::string& name, const std::string& description, double value) : ParameterObject()
{
    m_double = new ParameterDouble(name, description, value);
    m_param = m_double;
    m_connectionId = m_double->connect([this](std::any value) {
        emit this->valueChanged(std::any_cast<double>(value));
    });
    m_double->setMinNotifier([this](const double& value) {
        emit this->minChanged(value);
    });
    m_double->setMaxNotifier([this](const double& value) {
        emit this->maxChanged(value);
    });
}

ParameterDoubleObject::ParameterDoubleObject(const std::string& name, const std::string& description, double value, double min, double max) : ParameterObject()
{
    m_double = new ParameterDouble(name, description, value, min, max);
    m_param = m_double;
    m_connectionId = m_double->connect([this](std::any value) {
        emit this->valueChanged(std::any_cast<double>(value));
    });
    m_double->setMinNotifier([this](const double& value) {
        emit this->minChanged(value);
    });
    m_double->setMaxNotifier([this](const double& value) {
        emit this->maxChanged(value);
    });
}

ParameterDoubleObject::ParameterDoubleObject(const std::string& name, const std::string& description, double value, double min, double max, int decimals) : ParameterObject()
{
    m_double = new ParameterDouble(name, description, value, min, max, decimals);
    m_param = m_double;
    m_connectionId = m_double->connect([this](std::any value) {
        emit this->valueChanged(std::any_cast<double>(value));
    });
    m_double->setMinNotifier([this](const double& value) {
        emit this->minChanged(value);
    });
    m_double->setMaxNotifier([this](const double& value) {
        emit this->maxChanged(value);
    });
}

ParameterDoubleObject::ParameterDoubleObject(ParameterTraits *param) : ParameterObject(param)
{
    m_deleteParam = false;
    m_double = dynamic_cast<ParameterDouble *>(param);
    m_connectionId = m_double->connect([this](std::any value) {
        emit this->valueChanged(std::any_cast<double>(value));
    });
    m_double->setMinNotifier([this](const double& value) {
        emit this->minChanged(value);
    });
    m_double->setMaxNotifier([this](const double& value) {
        emit this->maxChanged(value);
    });
}

ParameterDoubleObject::~ParameterDoubleObject()
{
    if (m_param) {
        m_param->cancelOnDestroy(m_destroyId);
        m_param->disconnect(m_connectionId);
        if (m_deleteParam)
            delete m_param;
    }
    m_double = nullptr;
}

double ParameterDoubleObject::value() const 
{
    return m_double->value();
}

void ParameterDoubleObject::setValue(double value) 
{
    if(value != this->value()) {
        m_double->setValue(value);
    }
}

double ParameterDoubleObject::min() const 
{
    return m_double->min();
}

void ParameterDoubleObject::setMin(double value) 
{
    if(value != this->min()) {
        m_double->setMin(value);
    }
}

double ParameterDoubleObject::max() const 
{
    return m_double->max();
}

void ParameterDoubleObject::setMax(double value) 
{
    if(value != this->max()) {
        m_double->setMax(value);
    }
}

int ParameterDoubleObject::decimals() const 
{
    return m_double->decimals();
}

REGISTER_PARAMETER_OBJECT("double", ParameterDoubleObject)
} // namespace Parameters
} // namespace OpenViBE
