// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <QtQml>

class QmlEngineSingleton {
public:
    static QmlEngineSingleton& getInstance() {
        static QmlEngineSingleton instance;
        return instance;
    }

    void setEngine(QQmlEngine *engine) {
        m_engine = engine;
    }

    QQmlEngine* engine() {
        return m_engine;
    }

private:
    QQmlEngine *m_engine;

    // Private constructor and destructor to enforce singleton pattern
    QmlEngineSingleton() {}
    ~QmlEngineSingleton() {}
    QmlEngineSingleton(const QmlEngineSingleton&);
    QmlEngineSingleton& operator=(const QmlEngineSingleton&);
};