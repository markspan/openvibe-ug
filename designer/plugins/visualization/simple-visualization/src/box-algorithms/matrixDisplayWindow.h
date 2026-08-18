// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <openvibe/ov_all.h>
#include <QtCore>
#include <QColor>
#include <QQmlEngine>
#include <QtQml>
#include <QJSValue>
#include <QQuickItem>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

class MatrixCell : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(QColor col READ col WRITE setCol NOTIFY colorChanged)
    Q_PROPERTY(bool isLabel READ isLabel CONSTANT)

public:
    MatrixCell(QObject *parent = nullptr) : QObject(parent) {}

    QString value() const { return m_value; }
    void setValue(QString value) {
        if (m_value != value) {
            m_value = value;
            emit valueChanged();
        }
    }
    void setValue(double value) {
        setValue(QString::number(value, 'f', 2)); // Format to 2 decimal places
    }

    QColor col() const { return m_color; }
    void setCol(const QColor &color) {
        if (m_color != color) {
            m_color = color;
            emit colorChanged();
        }
    }
    bool isLabel() const { return m_isLabel; }

signals:
    void valueChanged();
    void colorChanged();

protected:
    friend class MatrixDisplayWindow;
    bool m_isLabel = false;
    QString m_value = ""; // Value of the cell
    QColor m_color = Qt::white; // Color of the cell
};

class MatrixDisplayWindow : public QObject {
    Q_OBJECT
    Q_PROPERTY(int nRow READ nRow NOTIFY nRowChanged)
    Q_PROPERTY(int nCol READ nCol NOTIFY nColChanged)
    Q_PROPERTY(QQmlListProperty<MatrixCell> cells READ cells NOTIFY cellsChanged)

public:
    MatrixDisplayWindow(QQmlEngine* engine);
    virtual ~MatrixDisplayWindow();

    int nRow() const;
    int nCol() const;

    QQmlListProperty<MatrixCell> cells();

    void setCellValue(int row, int col, double value, double red, double green, double blue, bool notify = true);
    void setColumnLabel(int col, const QString &label, bool notify=true); //will call setCellValue
    void setRowLabel(int row, const QString &label, bool notify=true); //will call setCellValue
    void refresh(); // forces a refresh of the display
    void initMatrix(int nRow, int nCol);

    QQuickItem *visualItem() { return m_visualItem; };

signals:
    void nRowChanged();
    void nColChanged();
    void cellsChanged();


protected:
    QQmlEngine *m_jsEngine = nullptr; // JavaScript engine for QML
    QQuickItem *m_visualItem = nullptr; // Visual item for QML
    QList<MatrixCell*> m_cells; // List of matrix cells

    int m_nRow = 0; // Number of rows in the matrix
    int m_nCol = 0; // Number of columns in the matrix
};


}
}
}