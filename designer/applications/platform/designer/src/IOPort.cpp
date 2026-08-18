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
    if(m_portType == PortType::Input) {
        m_box->getInputName(m_portIndex, cName);
    } else {
        m_box->getOutputName(m_portIndex, cName);
    }
    return QString(cName.toASCIIString());
}

void IOPort::setName(const QString &name)
{
    if (this->name() == name) {
         return; 
    }
    CString cName = CString(name.toStdString().c_str());
    if(m_portType == PortType::Input) {
        m_box->setInputName(m_portIndex, cName);
    } else {
        m_box->setOutputName(m_portIndex, cName);
    }
}

QString IOPort::dataType() const
{
    CIdentifier typeID;
    if(m_portType == PortType::Input) {
        m_box->getInputType(m_portIndex, typeID);
    } else {
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
    if(newDataType != CIdentifier::undefined() && dataType != oldDataType) {
        if(m_portType == PortType::Input) {
            if(!m_box->setInputType(m_portIndex, newDataType)) {
                m_context->getLogManager() << Kernel::LogLevel_Warning << "Could not set input type to " << dataType.toStdString() 
                                           << " for port " << m_portIndex 
                                           << "  reverting to " << oldDataType.toStdString() << "\n";
                return;
            }
        } else {
            if(!m_box->setOutputType(m_portIndex, newDataType)) {
                m_context->getLogManager() << Kernel::LogLevel_Warning << "Could not set output type to " << dataType.toStdString() 
                                           << " for port " << m_portIndex 
                                           << "  reverting to " << oldDataType.toStdString() << "\n";
                return;
            }
        }

        //the setInputType may call a callback that will change the type again
        if(this->dataType() == dataType) {
            emit dataTypeChanged();
        }
    }
}

} // namespace Designer
} // namespace OpenViBE
