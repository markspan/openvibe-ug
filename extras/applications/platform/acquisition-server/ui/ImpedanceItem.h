/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once
#include <QtCore>

class ImpedanceItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(double value READ value NOTIFY valueChanged)
    Q_PROPERTY(double fraction READ fraction NOTIFY valueChanged)

public:
    ImpedanceItem(const QString& name, double value, double fraction, QObject* parent = nullptr)
        : QObject(parent), m_name(name), m_value(value), m_fraction(fraction) {}
    
    QString name() const { return m_name; }
    double value() const { return m_value; }
    double fraction() const { return m_fraction; }

    void setValue(double value) {
        if (m_value    != value) {
            m_value    = value;
            emit valueChanged();
        }
    }

    void setFraction(double fraction) {
        if (m_fraction != fraction) {
            m_fraction = fraction;
            emit fractionChanged();
        }
    }

signals:
    void nameChanged();
    void valueChanged();
    void fractionChanged();
protected:
    QString m_name;
    double m_value;
    double m_fraction;
};
