/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once

#include <QtCore>

class OVLogControllerItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(QString level READ level NOTIFY levelChanged)
    Q_PROPERTY(QString time READ time NOTIFY timeChanged)

public:
    OVLogControllerItem(const QString &message, const QString &level);
    ~OVLogControllerItem();

    QString message() const;
    QString level() const;
    QString time() const;

signals:
    void messageChanged();
    void levelChanged();
    void timeChanged();

private:
    QString m_message;
    QString m_level;
    QString m_time;
};
