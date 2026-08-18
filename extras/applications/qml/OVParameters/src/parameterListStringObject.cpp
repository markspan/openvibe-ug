/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "parameterListStringObject.h"

namespace OpenViBE {
namespace Parameters {

ParameterListStringObject::ParameterListStringObject(std::vector<std::string> available_values): ParameterObject()
{
    m_listString = new ParameterListString(available_values);
    m_param = m_listString;
    m_connectionId = m_listString->connect([this](std::any value) {
        const QString ss = QString::fromStdString(std::any_cast<std::string>(value));
        emit this->valueChanged(ss);
        emit indexChanged(this->index());
    });
    m_listString->setAvailableValuesNotifier([this](const std::vector<std::string>& values) {
        QStringList ll;
        for(auto s : values) {
            ll << QString::fromStdString(s);
        }
        emit this->availableValuesChanged(ll);
    });
}

ParameterListStringObject::ParameterListStringObject(const std::string& name, const std::string& description, const std::vector<std::string>& available_values, const std::string& value) : ParameterObject()
{
    m_listString = new ParameterListString(name, description, available_values, value);
    m_param = m_listString;
    m_connectionId = m_listString->connect([this](std::any value) {
        const QString ss = QString::fromStdString(std::any_cast<std::string>(value));
        emit this->valueChanged(ss);
        emit indexChanged(this->index());
    });
    m_listString->setAvailableValuesNotifier([this](const std::vector<std::string>& values) {
        QStringList ll;
        for(auto s : values) {
            ll << QString::fromStdString(s);
        }
        emit this->availableValuesChanged(ll);
    });
}

ParameterListStringObject::ParameterListStringObject(ParameterTraits *param) : ParameterObject(param)
{
    m_deleteParam = false;
    m_listString = dynamic_cast<ParameterListString *>(param);
    m_param = m_listString;
    m_connectionId = m_listString->connect([this](std::any value) {
        const QString ss = QString::fromStdString(std::any_cast<std::string>(value));
        emit this->valueChanged(ss);
        emit indexChanged(this->index());
    });
    m_listString->setAvailableValuesNotifier([this](const std::vector<std::string>& values) {
        QStringList ll;
        for(auto s : values) {
            ll << QString::fromStdString(s);
        }
        emit this->availableValuesChanged(ll);
    });
}

ParameterListStringObject::~ParameterListStringObject()
{
    if (m_param) {
        m_param->cancelOnDestroy(m_destroyId);
        m_param->disconnect(m_connectionId);
        if (m_deleteParam)
            delete m_param;
    }
    m_listString = nullptr;
}

QString ParameterListStringObject::value() const 
{
    return QString::fromStdString(m_listString->value());
}

void ParameterListStringObject::setValue(QString value) 
{
    if(value != this->value()) {
        m_listString->setValue(value.toStdString());
    }
}

QStringList ParameterListStringObject::availableValues() const
{
    QStringList ll;
    for(auto s : m_listString->availableValues()) {
        ll << QString::fromStdString(s);
    }
    return ll;
}

void ParameterListStringObject::setAvailableValues(QStringList values) 
{
    std::vector<std::string> vv;
    for(auto s : values) {
        vv.push_back(s.toStdString());
    }
    m_listString->setAvailableValues(vv, false);
}

int ParameterListStringObject::index() const 
{
    const auto ll = m_listString->availableValues();
    auto it = std::find(ll.cbegin(), ll.cend(), m_listString->value());
    if(it != ll.end()) {
        return std::distance(ll.cbegin(), it);
    }

    return -1;
}

void ParameterListStringObject::setIndex(int index)
{
    const auto values = m_listString->availableValues();
    if (index < 0 || static_cast<size_t>(index) >= values.size())
        return;
    this->setValue(QString::fromStdString(values.at(index)));
}

REGISTER_PARAMETER_OBJECT("stringlist", ParameterListStringObject)
} // namespace Parameters
} // namespace OpenViBE
