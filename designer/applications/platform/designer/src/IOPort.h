// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <openvibe/ov_all.h>
#include <QtCore>

namespace OpenViBE {
namespace Designer {

class IOPort : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString dataType READ dataType WRITE setDataType NOTIFY dataTypeChanged)
    Q_PROPERTY(int index READ index CONSTANT)

signals:
    void nameChanged();
    void nameChanged(const QString&);
    void dataTypeChanged();
    void dataTypeChanged(const QString&);
    /** A user-level edit of the port reached the kernel (see BoxObject::edited). */
    void edited(const QString& label, int delayMs);

public:
    enum PortType
    {
        Input,
        Output
    };
    Q_ENUM(PortType)

    IOPort(Kernel::IKernelContext* context, Kernel::IBox* box, PortType portType, int portIndex);
    virtual ~IOPort();

    QString name() const;
    QString dataType() const;
    int index() const;

    void setName(const QString &name);
    void setDataType(const QString &dataType);

protected:
    Kernel::IKernelContext* m_context = nullptr;
    Kernel::IBox* m_box = nullptr;
    PortType m_portType = PortType::Input;
    int m_portIndex = 0;
};

} // namespace Designer
} // namespace OpenViBE