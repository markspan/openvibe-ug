/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include "parameterObject.h"
#include <openvibe/parameters/parameterPath.h>

namespace OpenViBE {
namespace Parameters {

class OV_PARAMETERS_OBJECT_EXPORT ParameterPathObject: public ParameterObject
{
    Q_OBJECT
    Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(QStringList filters READ filters WRITE setFilters NOTIFY filtersChanged)
    Q_PROPERTY(QString mode READ mode CONSTANT)
    Q_PROPERTY(QUrl dirName READ dirName NOTIFY valueChanged)
    Q_PROPERTY(QUrl baseName READ baseName NOTIFY valueChanged)

public:
    ParameterPathObject(const std::string& name, const std::string& description, const QString& value, const OpenMode mode = Read, const std::vector<std::string>& filters = {});
    ParameterPathObject(ParameterTraits *param);
    virtual ~ParameterPathObject();

    QString value() const;
    void setValue(QString value);
    /// Sets the value even when it does not change, so that choosing the same file again reloads it.
    Q_INVOKABLE void choose(QString value);

    QStringList filters() const;
    void setFilters(QStringList filters);

    QString mode() const;
    QUrl dirName() const;
    QUrl baseName() const;

signals:
    void valueChanged(const QString&);
    void filtersChanged(const QStringList&);

private:
    static QString decode(const QString& value);

    ParameterPath *m_path = nullptr;
};

DECLARE_PARAMETER_REGISTRATION(ParameterPathObject)
} // namespace Parameters
} // namespace OpenViBE
