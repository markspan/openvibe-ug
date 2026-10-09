// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "IOPort.h"

namespace OpenViBE {
namespace Designer {


IOPort::IOPort(Kernel::IKernelContext* context, Kernel::IBox* box, PortType portType, int portIndex)
    : m_context(context),
      m_box(box),
      m_portType(portType),
      m_portIndex(portIndex)
{ }

IOPort::~IOPort()
{
}

QString IOPort::name() const
{
    CString cName;
    if(m_portType == PortType::Input)
    {
        m_box->getInputName(m_portIndex, cName);
    }
    else
    {
        m_box->getOutputName(m_portIndex, cName);
    }
    return QString(cName.toASCIIString());
}

void IOPort::setName(const QString &name)
{
    if (this->name() != name)
    {
        CString cName = CString(name.toStdString().c_str());
        const bool success = (m_portType == PortType::Input) ? m_box->setInputName(m_portIndex, cName) : m_box->setOutputName(m_portIndex, cName);
        if (success)
        {
            emit nameChanged();
            emit edited(tr("Rename port"), 0);
        }
    }
}

QString IOPort::dataType() const
{
    CIdentifier typeID;
    if(m_portType == PortType::Input)
    {
        m_box->getInputType(m_portIndex, typeID);
    }
    else
    {
        m_box->getOutputType(m_portIndex, typeID);
    }
    CString ovName = m_context->getTypeManager().getTypeName(typeID);
    return QString(ovName.toASCIIString());
}

int IOPort::index() const
{
    return m_portIndex;
}

void IOPort::setDataType(const QString &dataType)
{
    CString ovName = CString(dataType.toStdString().c_str());
    CIdentifier newDataType = m_context->getTypeManager().getTypeFromName(ovName);
    QString oldDataType = this->dataType();
    if(newDataType != CIdentifier::undefined() && dataType != oldDataType)
    {
        const bool isInput = (m_portType == PortType::Input);
        const bool ok = isInput ? m_box->setInputType(m_portIndex, newDataType)
                                : m_box->setOutputType(m_portIndex, newDataType);
        if(!ok)
        {
            m_context->getLogManager() << Kernel::LogLevel_Warning << "Could not set " << (isInput ? "input" : "output") << " type to " << dataType.toStdString()
                                       << " for port " << m_portIndex
                                       << "  reverting to " << oldDataType.toStdString() << "\n";
        }
        else
        {
            //the setInputType may call a callback that will change the type again
            if(this->dataType() == dataType)
            {
                emit dataTypeChanged();
            }
            emit edited(tr("Change port type"), 0);
        }
    }
}

} // namespace Designer
} // namespace OpenViBE
