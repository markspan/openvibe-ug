/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "parameterStringVectorObject.h"

#include <stdexcept>

namespace OpenViBE {
namespace Parameters {

void parameterStringVectorObject::connectSignals()
{
    if (!m_stringVector) return;
    m_connectionId = m_stringVector->connect([this](std::any value) {
        auto ss = std::any_cast<std::vector<std::string>>(value);
        QStringList ll;
        for (const auto& s : ss)
            ll << QString::fromStdString(s);
        emit this->valueChanged(ll);
    });
}

parameterStringVectorObject::parameterStringVectorObject(const std::vector<std::string>& value) : ParameterObject()
{
    m_stringVector = new ParameterStringVector(value);
    m_param = m_stringVector;
    connectSignals();
}

parameterStringVectorObject::parameterStringVectorObject(const std::string& name, const std::string& description, const std::vector<std::string>& value) : ParameterObject()
{
    m_stringVector = new ParameterStringVector(name, description, value);
    m_param = m_stringVector;
    connectSignals();
}

parameterStringVectorObject::parameterStringVectorObject(ParameterTraits *param) : ParameterObject(param)
{
    m_deleteParam = false;
    m_stringVector = dynamic_cast<ParameterStringVector*>(param);
    if (!m_stringVector)
        throw std::invalid_argument("parameterStringVectorObject: param is not a ParameterStringVector");
    m_param = m_stringVector;
    connectSignals();
}

parameterStringVectorObject::~parameterStringVectorObject()
{
    if (m_param) {
        m_param->cancelOnDestroy(m_destroyId);
        m_param->disconnect(m_connectionId);
        if (m_deleteParam)
            delete m_param;
    }
    m_stringVector = nullptr;
}

QStringList parameterStringVectorObject::value() const
{
    QStringList ll;
    for (const auto& s : m_stringVector->value())
        ll << QString::fromStdString(s);
    return ll;
}

void parameterStringVectorObject::setValue(QStringList value)
{
    std::vector<std::string> vv;
    for (const auto& s : value)
        vv.push_back(s.toStdString());
    m_stringVector->setValue(vv);
}

QString parameterStringVectorObject::getAt(int index) const
{
    return QString::fromStdString(m_stringVector->getAt(index));
}

void parameterStringVectorObject::setAt(int index, const QString& value)
{
    m_stringVector->setAt(index, value.toStdString());
}

REGISTER_PARAMETER_OBJECT("stringvector", parameterStringVectorObject)

} // namespace Parameters
} // namespace OpenViBE
