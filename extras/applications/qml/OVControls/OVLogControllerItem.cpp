/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "OVLogControllerItem.h"

OVLogControllerItem::OVLogControllerItem(const QString &message, const QString &level) : m_message(message), m_level(level)
{
    m_time = QDateTime::currentDateTime().toString("hh:mm:ss");
}

OVLogControllerItem::~OVLogControllerItem()
{
}

QString OVLogControllerItem::message() const
{
    return m_message;
}

QString OVLogControllerItem::level() const
{
    return m_level;
}

QString OVLogControllerItem::time() const
{
    return m_time;
}

