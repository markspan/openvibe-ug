// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <openvibe/ov_all.h>
#include <QtCore>

namespace OpenViBE {  
namespace Designer {

class CommentObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString identifier READ identifier CONSTANT)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QPoint center READ center WRITE setCenter NOTIFY centerChanged)

signals:
    void textChanged();
    void centerChanged();

public:
    CommentObject(Kernel::IKernelContext* kernelCtx, Kernel::IComment *comment);
    virtual ~CommentObject();

    Q_INVOKABLE void notifyCenterChanged(int new_x, int new_y);

    CIdentifier id() const;
    QString identifier() const;
    QString text() const;
    QPoint center() const;

    void setText(const QString &text);
    void setCenter(const QPoint &center);

protected:
    friend class DesignerScenario;
    Kernel::IKernelContext* m_kernelCtx = nullptr;
    Kernel::IComment *m_comment = nullptr;
    QPoint m_center = QPoint(50,50);
};

}
}