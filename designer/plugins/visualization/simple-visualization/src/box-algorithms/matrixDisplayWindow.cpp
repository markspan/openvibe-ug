// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "matrixDisplayWindow.h"
#include <QQmlContext>
#include <QQmlComponent>
#include <QQuickView>
#include <QQmlEngine>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

MatrixDisplayWindow::MatrixDisplayWindow(QQmlEngine* engine) : m_jsEngine(engine), m_nRow(0), m_nCol(0)
{
    qRegisterMetaType<MatrixCell *>();

    QQmlContext *context = new QQmlContext(m_jsEngine->rootContext());

    context->setContextProperty("dataSource", this);
    #if defined TARGET_OS_Windows
        context->setContextProperty("TARGET_OS_Windows", QVariant(true));
    #else
        context->setContextProperty("TARGET_OS_Windows", QVariant(false));
    #endif

    QQmlComponent component(m_jsEngine, QUrl("qrc:/qml/MatrixDisplayWindow.qml"));

    QObject *object = component.create(context);
    m_visualItem = qobject_cast<QQuickItem *>(object);
    if (m_visualItem) {
        context->setParent(m_visualItem);
    }
}


MatrixDisplayWindow::~MatrixDisplayWindow()
{
    // Clean up the visual item before anything else
    if (m_visualItem) {
        m_visualItem->deleteLater();
        m_visualItem = nullptr;
    }

    for (MatrixCell *cell : m_cells) {
        delete cell;
    }
    m_cells.clear();

    m_jsEngine = nullptr;
}

int MatrixDisplayWindow::nRow() const
{
    return m_nRow;
}

void MatrixDisplayWindow::initMatrix(int nRow, int nCol)
{
    m_nRow = nRow;
    m_nCol = nCol;

    // Clear existing cells
    qDeleteAll(m_cells);
    m_cells.clear();

    // Initialize cells
     // +1 for labels
    for (int i = 0; i < (m_nRow+1) * (m_nCol+1); ++i) {
        MatrixCell *cell = new MatrixCell();
        cell->moveToThread(m_jsEngine->thread());
        if(i == 0) {
            // Top-left cell (0,0)
            cell->setValue(""); // Empty value for the top-left corner
            cell->m_isLabel = true;
        } else if(i % (m_nCol + 1) == 0) {
            // First column (row label)
            cell->setValue(QString::number(i / (m_nCol + 1))); // Row label
            cell->m_isLabel = true;
        } else if (i < (m_nCol + 1)) {
            // First row (column label)
            cell->setValue(QString::number(i - 1)); // Column label
            cell->m_isLabel = true;
        }
        else {
            // Regular cell
            cell->setValue("X"); // Default value
        }
        cell->setCol(Qt::white); // Default color
        m_cells.append(cell);
    }
    emit nRowChanged();
    emit nColChanged();
    emit cellsChanged();
}

int MatrixDisplayWindow::nCol() const
{
    return m_nCol;
}

QQmlListProperty<MatrixCell> MatrixDisplayWindow::cells()
{
    return QQmlListProperty<MatrixCell>(this, &m_cells);
}

void MatrixDisplayWindow::setCellValue(int row, int col, double value, double red, double green, double blue, bool notify)
{
    if (row < 0 || col < 0 || row >= m_nRow || col >= m_nCol) {
        qWarning() << "Invalid cell coordinates:" << row << col;
        return;
    }

    int index = (row+1) * (m_nCol+1) + (col+1);
    MatrixCell *cell = nullptr;

    if (index < m_cells.size()) {
        cell = m_cells[index];
    } else {
        qWarning() << "Wrong cell index:" << index << "for row:" << row << "col:" << col;
        return;
    }

    cell->setValue(value);
    cell->setCol(QColor(red, green, blue));

    if (notify) {
        emit cellsChanged();
    }
}

void MatrixDisplayWindow::setColumnLabel(int col, const QString &label, bool notify)
{
    MatrixCell *cell = m_cells.value(col+1);
    if (cell) {
        cell->setValue(label);
        cell->setCol(Qt::white);
        if (notify) {
            emit cellsChanged();
        }
    }
}

void MatrixDisplayWindow::setRowLabel(int row, const QString &label, bool notify)
{
    MatrixCell *cell = m_cells.value((row+1) * (m_nCol+1));
    if (cell) {
        cell->setValue(label);
        cell->setCol(Qt::white);
        if (notify) {
            emit cellsChanged();
        }
    }
}

void MatrixDisplayWindow::refresh()
{
    emit cellsChanged();
}

}
}
}
