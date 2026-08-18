// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <QtCore>
#include <QQmlListProperty>
#include <QColor>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QQuickItem>
#include <QQmlEngine>

#include "openvibe/ov_all.h"

namespace OpenViBE {
namespace Designer {

class WindowManagerNode: public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString nodeType READ nodeType CONSTANT)
  Q_PROPERTY(QString id READ id CONSTANT)
  Q_PROPERTY(QString name READ name NOTIFY nameChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY(QQmlListProperty<WindowManagerNode> children READ children NOTIFY childrenChanged)
  Q_PROPERTY(QQuickItem* visual READ visual NOTIFY visualChanged)
  Q_PROPERTY(bool fullScreen READ fullScreen NOTIFY fullScreenChanged)

public:
    enum NodeType {
        Box,
        VerticalSplit,
        HorizontalSplit,
        Undefined
    };

    explicit WindowManagerNode(const Kernel::IKernelContext& ctx, NodeType type, QString id, QString boxId, QString boxName = QString(), QColor color = QColor(0, 0, 0));

    QString nodeType();
    QString id();
    QString name();
    void setName(QString name);
    QColor color();
    void setColor(QColor color);
    QQmlListProperty<WindowManagerNode> children();
    QQuickItem* visual() {
        QQmlEngine::setObjectOwnership(m_visual, QQmlEngine::CppOwnership);
        return m_visual; 
    }
    bool fullScreen() { return m_fullScreen; }


    QList<WindowManagerNode*>& childrenList() { 
        for (auto child : m_children) {
            QQmlEngine::setObjectOwnership(child, QQmlEngine::CppOwnership);
        }
        return m_children;
    } // For C++ use

    bool addChild(WindowManagerNode* child);
    QString boxId();
    void setVisual(QQuickItem* visual) { 
        if (m_visual != visual) {
            m_visual = visual;
            emit visualChanged();
        }
    }
    void setFullScreen(bool fullScreen) {
        if (m_fullScreen != fullScreen) {
            m_fullScreen = fullScreen;
            emit fullScreenChanged();
        }
    }

signals:
    void nodeTypeChanged();
    void childrenChanged();
    void nameChanged();
    void colorChanged();
    void visualChanged();
    void fullScreenChanged();

private:
    const Kernel::IKernelContext& m_kernelCtx;
    NodeType m_nodeType;
    QString m_id;
    QString m_boxId;
    QString m_boxName;
    QColor m_color;
    bool m_fullScreen = false;

    // Node is either a box with a visual or a split with children
    QList<WindowManagerNode*> m_children;
    QQuickItem* m_visual = nullptr;
};

}  // Designer
}  // OpenViBE