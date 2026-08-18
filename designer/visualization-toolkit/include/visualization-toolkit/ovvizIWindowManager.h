// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

class QQmlEngine;
class QQuickItem;

namespace OpenViBE {
namespace VisualizationToolkit {

/**
 * Interface for a window manager
 * This interface is used by the IVisualizationContext
 * The aim is to keep this interface UI framework agnostic
 */
class IWindowManager
{
  public:
    virtual ~IWindowManager() = default;

    virtual QQmlEngine* getQmlEngine() = 0;
    virtual void setVisualizationItem(CIdentifier& boxID, QQuickItem* item) = 0;
    virtual void clearVisualizationItem(CIdentifier& boxID) = 0;
    virtual void setVisualizationItemFullScreen(CIdentifier& boxID, bool fullScreen) = 0;

    virtual void show() = 0;
    virtual void hide() = 0;

};


} // namespace VisualizationToolkit
} // namespace OpenViBE