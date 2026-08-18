/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "parameterObject.h"

namespace OpenViBE {
namespace Parameters {

ParameterObject::ParameterObject(ParameterTraits *param)
{
    m_param = param;
    m_destroyId = param->onDestroy([this]() {
        m_param = nullptr;
    });
}

ParameterObject::~ParameterObject() 
{
    //no delete here as it will be deleted by its children
    m_param = nullptr; 
}

QString ParameterObject::name() const 
{ 
    if(m_param) 
        return QString::fromStdString(m_param->name());
    return "";
}

QString ParameterObject::description() const 
{
    if(m_param)
        return QString::fromStdString(m_param->description());
    return "";
}

QString ParameterObject::type() const 
{
    if(m_param)
        return QString::fromStdString(m_param->type()); 
    return "";
}

void ParameterObject::setName(const QString &name) 
{
    //doc on https://doc.qt.io/qt-6/qtqml-cppintegration-exposecppattributes.html
    // the NOTIFY signal is emitted only if the value actually changes
    // it this notifies the QML engine that any bindings involving the author property must be updated
    if(m_param && name != this->name()) {
        m_param->setName(name.toStdString());
        emit nameChanged(name);
    }
}

void ParameterObject::setDescription(const QString &description) 
{
    if(m_param && description != this->description()) {
        m_param->setDescription(description.toStdString());
        emit descriptionChanged(description);
    }
}

ParameterTraits *ParameterObject::parameter() const 
{
    return m_param;
}

void ParameterObject::notifyNameChanged(const QString& n) 
{
    emit nameChanged(n);
}

void ParameterObject::notifyDescriptionChanged(const QString& d) 
{
    emit descriptionChanged(d);
}

ParameterObject *ParameterObject::fromParameter(ParameterTraits *param) {
    auto func = object_factory()[param->type()];
    if(func) {
        return func(param);
    } else {
        qWarning() << "WARNING: No function found for type " << QString::fromStdString(param->type());
    }
    return nullptr;
}

QMap<std::string, std::function<ParameterObject*(ParameterTraits *)> > &object_factory() {
    static QMap<std::string, std::function<ParameterObject*(ParameterTraits *)> > _instance;
    return _instance;
}

} // namespace Parameters
} // namespace OpenViBE