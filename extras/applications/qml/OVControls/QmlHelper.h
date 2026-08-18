/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once

#include <QtCore>
#include <QtQml>

class QmlHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static QmlHelper *instance();
    static QmlHelper *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    /**
     * \brief Set the value of a property of an object
     * \param pObject The object
     * \param name The name of the property
     * \param v The value to set the property to
     */
    Q_INVOKABLE void setValue (QObject *pObject, const QString& name, QVariant v);

    /**
     * \brief Write data to a file
     * \param fileName The file to write to
     * \param data The data to write
     * \return True if the file could be opened, false otherwise
     */
    Q_INVOKABLE bool write(const QUrl& fileName, const QString& data);

    /**
     * \brief Read data from a file
     * \param fileName The file to read from
     * \return The data read from the file, or an empty string if the file could not be opened
     */
    Q_INVOKABLE QString read(const QUrl& fileName);

private:
    QmlHelper() = default;
    ~QmlHelper() = default;
    static QmlHelper *s_instance;
};
