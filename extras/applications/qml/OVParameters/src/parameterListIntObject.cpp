/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "parameterListIntObject.h"

namespace OpenViBE {
namespace Parameters {

ParameterListIntObject::ParameterListIntObject(std::vector<int> available_values): ParameterObject()
{
    m_listInt = new ParameterListInt(available_values);
    m_param = m_listInt;
    m_connectionId = m_listInt->connect([this](std::any value) {
        const int v = std::any_cast<int>(value);
        emit this->valueChanged(v);
        emit indexChanged(this->index());
    });
    m_listInt->setAvailableValuesNotifier([this](const std::vector<int>& values) {
        QList<int> ll(values.begin(), values.end());
        emit this->availableValuesChanged(ll);
    });
}

ParameterListIntObject::ParameterListIntObject(const std::string& name, const std::string& description, const std::vector<int>& available_values, int value) : ParameterObject()
{
    m_listInt = new ParameterListInt(name, description, available_values, value);
    m_param = m_listInt;
    m_connectionId = m_listInt->connect([this](std::any value) {
        const int v = std::any_cast<int>(value);
        emit this->valueChanged(v);
        emit indexChanged(this->index());
    });
    m_listInt->setAvailableValuesNotifier([this](const std::vector<int>& values) {
        QList<int> ll(values.begin(), values.end());
        emit this->availableValuesChanged(ll);
    });
}

ParameterListIntObject::ParameterListIntObject(ParameterTraits *param) : ParameterObject(param)
{
    m_deleteParam = false;
    m_listInt = dynamic_cast<ParameterListInt *>(param);
    m_param = m_listInt;
    m_connectionId = m_listInt->connect([this](std::any value) {
        const int v = std::any_cast<int>(value);
        emit this->valueChanged(v);
        emit indexChanged(this->index());
    });
    m_listInt->setAvailableValuesNotifier([this](const std::vector<int>& values) {
        QList<int> ll(values.begin(), values.end());
        emit this->availableValuesChanged(ll);
    });
}

ParameterListIntObject::~ParameterListIntObject()
{
    if (m_param) {
        m_param->cancelOnDestroy(m_destroyId);
        m_param->disconnect(m_connectionId);
        if (m_deleteParam)
            delete m_param;
    }
    m_listInt = nullptr;
}

int ParameterListIntObject::value() const
{
    return m_listInt->value();
}

void ParameterListIntObject::setValue(int value)
{
    if (value != this->value()) {
        m_listInt->setValue(value);
    }
}

QList<int> ParameterListIntObject::availableValues() const
{
    const auto& v = m_listInt->availableValues();
    return QList<int>(v.begin(), v.end());
}

void ParameterListIntObject::setAvailableValues(QList<int> values)
{
    m_listInt->setAvailableValues(std::vector<int>(values.begin(), values.end()), false);
}

int ParameterListIntObject::index() const
{
    const auto ll = m_listInt->availableValues();
    auto it = std::find(ll.cbegin(), ll.cend(), m_listInt->value());
    if (it != ll.end()) {
        return std::distance(ll.cbegin(), it);
    }
    return -1;
}

void ParameterListIntObject::setIndex(int index)
{
    const auto values = m_listInt->availableValues();
    if (index < 0 || static_cast<size_t>(index) >= values.size())
        return;
    setValue(values.at(index));
}

REGISTER_PARAMETER_OBJECT("intlist", ParameterListIntObject)
} // namespace Parameters
} // namespace OpenViBE
