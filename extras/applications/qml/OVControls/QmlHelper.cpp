/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/ 

#include "QmlHelper.h"


QmlHelper *QmlHelper::instance() 
{
    if (!s_instance) {
        s_instance = new QmlHelper();
    }

    return s_instance;
}

QmlHelper *QmlHelper::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    QmlHelper *result = QmlHelper::instance();
    return result;
}

void QmlHelper::setValue (QObject *pObject, const QString& name, QVariant v) 
{
    QByteArray n = name.toUtf8();
    if (v.isNull()) {
        v = QVariant(pObject->property(n.data()).metaType());
    }

    pObject->setProperty(n.data(), v);

    // Force signal emission
    const QMetaObject* meta = pObject->metaObject();
    int propIndex = meta->indexOfProperty(n.data());
    if (propIndex != -1) {
        QMetaProperty prop = meta->property(propIndex);
        if (prop.hasNotifySignal()) {
            prop.notifySignal().invoke(pObject);
        }
    }
}

bool QmlHelper::write(const QUrl& fileName, const QString& data)
{
    QFile file(fileName.toLocalFile());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << data;
    file.close();

    return true;
}

QString QmlHelper::read(const QUrl& fileName)
{
    QFile file(fileName.toLocalFile());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream in(&file);
    QString data = in.readAll();
    file.close();

    return data;
}

QmlHelper *QmlHelper::s_instance = nullptr;