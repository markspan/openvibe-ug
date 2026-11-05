/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "parameterPathObject.h"

namespace OpenViBE {
namespace Parameters {

ParameterPathObject::ParameterPathObject(const std::string& name, const std::string& description, const QString& value, const OpenMode mode, const std::vector<std::string>& filters) : ParameterObject()
{
    std::string valueStd = value.toStdString();
    /*
    // -> deactivated. Openvibe sometime checks that path is equal to "" to check if it's set or not. 
    //if the path does not exist, set it to home directory
    if(mode == OpenMode::Read || mode == OpenMode::Append) {
        QFileInfo pathInfo(value);
        if(!pathInfo.exists()) {
            valueStd = QDir::homePath().toStdString();
        }
    }
    */

    m_path = new ParameterPath(name, description, valueStd, mode, filters);

    m_param = m_path;
    m_connectionId = m_path->connect([this](std::any value) {
        const QString ss = QString::fromStdString(std::any_cast<std::string>(value));
        emit this->valueChanged(ss);
    });
}

ParameterPathObject::ParameterPathObject(ParameterTraits *param) : ParameterObject(param)
{
    m_deleteParam = false;
    m_path = dynamic_cast<ParameterPath *>(param);
    m_param = m_path;

/*
    if(m_path->mode() == OpenMode::Read || m_path->mode() == OpenMode::Append) {
        //if the path does not exist, set it to home directory
        QFileInfo pathInfo(QString::fromStdString(m_path->value()));
        if(!pathInfo.exists()) {
            m_path->setValue(QDir::homePath().toStdString());
        }
    }
*/

    m_connectionId = m_path->connect([this](std::any value) {
        const QString ss = QString::fromStdString(std::any_cast<std::string>(value));
        emit this->valueChanged(ss);
    });
}

ParameterPathObject::~ParameterPathObject()
{
    if (m_param) {
        m_param->cancelOnDestroy(m_destroyId);
        m_param->disconnect(m_connectionId);
        if (m_deleteParam)
            delete m_param;
    }
    m_path = nullptr;
}

QString ParameterPathObject::value() const 
{
    return QString::fromStdString(m_path->value());
}

QString ParameterPathObject::decode(const QString& value)
{
    // UG fork: always forward slashes. A box's setting goes through the kernel's token expansion, where a backslash is an escape
    // character, so a box could not open a file picked here on Windows: "C:\a\b.ov" became "C:ab.ov". Upstream uses native
    // separators for a picked file. A typed or pasted Windows path gets the same treatment.
    const QUrl url(value);
    return url.isLocalFile() ? url.toLocalFile() : QDir::fromNativeSeparators(value);
}

void ParameterPathObject::setValue(QString value)
{
    const QString decoded_value = decode(value);
    if(decoded_value != this->value()) {
        m_path->setValue(decoded_value.toStdString());
    }
}

// UG fork: setValue() ignores an unchanged value; this notifies the listeners regardless
void ParameterPathObject::choose(QString value)
{
    m_path->setValue(decode(value).toStdString());
}

QStringList ParameterPathObject::filters() const 
{
    QStringList filters;
    for(auto f : m_path->filters()) {
        filters << QString::fromStdString(f);
    }

    return filters;
}

void ParameterPathObject::setFilters(QStringList filters) 
{
    std::vector<std::string> stdFilters;    
    for(auto f : filters) {
        stdFilters.push_back(f.toStdString());
    }
    if(stdFilters != m_path->filters()) {
        m_path->setFilters(stdFilters);
    }
}

QString ParameterPathObject::mode() const 
{
    switch(m_path->mode()) {
        case Read:
            return "Read";
        case Write:
            return "Write";
        case ReadWrite:
            return "ReadWrite";
        case Append:
            return "Append";
        default:
            break;
    }
    return "Read";
}

QUrl ParameterPathObject::dirName() const 
{
    QUrl url;
    QFileInfo pathInfo(QString::fromStdString(m_path->value()));
    if(pathInfo.isDir()) {
        url.setPath(pathInfo.absoluteFilePath());
    } else {
        url.setPath(pathInfo.dir().path());
    }
    url.setScheme("file");
    return url;
}

QUrl ParameterPathObject::baseName() const 
{
    QFileInfo pathInfo(QString::fromStdString(m_path->value()));
    if(pathInfo.isDir()) {
        return QUrl();
    } else {
        QUrl url(pathInfo.absoluteFilePath());
        url.setScheme("file");
        return url;
    }
}


REGISTER_PARAMETER_OBJECT("path", ParameterPathObject)
} // namespace Parameters
} // namespace OpenViBE
