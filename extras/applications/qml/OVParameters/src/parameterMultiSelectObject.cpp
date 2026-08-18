/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "parameterMultiSelectObject.h"

namespace OpenViBE {
namespace Parameters {

ParameterMultiSelectObject::ParameterMultiSelectObject(std::vector<std::string> availableValues): ParameterObject()
{
    m_multiSelect = new ParameterMultiSelect(availableValues);
    m_param = m_multiSelect;
    m_connectionId = m_multiSelect->connect([this](std::any value) {
        auto ss = std::any_cast<std::vector<std::string>>(value);
        QStringList ll;
        for(const auto& s : ss) {
            ll << QString::fromStdString(s);
        }
        emit this->valueChanged(ll);
    });
    m_multiSelect->setAvailableValuesNotifier([this](const std::vector<std::string>& values) {
        QStringList ll;
        for(auto s : values) {
            ll << QString::fromStdString(s);
        }
        emit this->availableValuesChanged(ll);
    });
}

ParameterMultiSelectObject::ParameterMultiSelectObject(const std::string& name, const std::string& description, const std::vector<std::string>& availableValues, const std::vector<std::string>& value) : ParameterObject()
{
    m_multiSelect = new ParameterMultiSelect(name, description, availableValues, value);
    m_param = m_multiSelect;
    m_connectionId = m_multiSelect->connect([this](std::any value) {
        auto ss = std::any_cast<std::vector<std::string>>(value);
        QStringList ll;
        for(auto s : ss) {
            ll << QString::fromStdString(s);
        }
        emit this->valueChanged(ll);
    });
    m_multiSelect->setAvailableValuesNotifier([this](const std::vector<std::string>& values) {
        QStringList ll;
        for(auto s : values) {
            ll << QString::fromStdString(s);
        }
        emit this->availableValuesChanged(ll);
    });
}

ParameterMultiSelectObject::ParameterMultiSelectObject(ParameterTraits *param) : ParameterObject(param)
{
    m_deleteParam = false;
    m_multiSelect = dynamic_cast<ParameterMultiSelect *>(param);
    m_param = m_multiSelect;
    m_connectionId = m_multiSelect->connect([this](std::any value) {
        auto ss = std::any_cast<std::vector<std::string>>(value);
        QStringList ll;
        for(auto s : ss) {
            ll << QString::fromStdString(s);
        }
        emit this->valueChanged(ll);
    });
    m_multiSelect->setAvailableValuesNotifier([this](const std::vector<std::string>& values) {
        QStringList ll;
        for(auto s : values) {
            ll << QString::fromStdString(s);
        }
        emit this->availableValuesChanged(ll);
    });
}

ParameterMultiSelectObject::~ParameterMultiSelectObject()
{
    if (m_param) {
        m_param->cancelOnDestroy(m_destroyId);
        m_param->disconnect(m_connectionId);
        if (m_deleteParam)
            delete m_param;
    }
    m_multiSelect = nullptr;
}

QStringList ParameterMultiSelectObject::value() const
{
    QStringList ll;
    for(auto s : m_multiSelect->value()) {
        ll << QString::fromStdString(s);
    }
    return ll;
}

void ParameterMultiSelectObject::setValue(QStringList value)
{
    if(value != this->value()) {
        std::vector<std::string> vv;
        for(auto s : value) {
            vv.push_back(s.toStdString());
        }
        m_multiSelect->setValue(vv);
    }
}

QStringList ParameterMultiSelectObject::availableValues() const
{
    QStringList ll;
    for(auto s : m_multiSelect->availableValues()) {
        ll << QString::fromStdString(s);
    }
    return ll;
}

void ParameterMultiSelectObject::setAvailableValues(QStringList values)
{
    std::vector<std::string> vv;
    for(auto s : values) {
        vv.push_back(s.toStdString());
    }
    m_multiSelect->setAvailableValues(vv, true);
}

REGISTER_PARAMETER_OBJECT("multiselect", ParameterMultiSelectObject)
} // namespace Parameters
} // namespace OpenViBE
