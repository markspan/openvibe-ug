// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <QtCore>
#include <QQuickView>

#include <openvibe/ov_all.h>

#include <functional>
#include <string>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

class CImageDisplayObject : public QObject
{
    Q_OBJECT
public:
    CImageDisplayObject(Kernel::ILogManager& logManager, const bool scaleToFit, QQmlEngine* engine);
    virtual ~CImageDisplayObject();

    /**
     * \brief Shows an image (or clears the view when the path is empty).
     *
     * Safe to call from the player thread!
     */
    void setImage(const std::string& image, std::function<void()> onDisplayed);

    QQuickItem* visualItem() { return m_visualItem; }
signals:

protected:
    QQmlEngine* m_jsEngine = nullptr;
    QQuickItem* m_visualItem = nullptr;

    std::vector<std::string> m_images;
    std::string m_currentImage;  ///< last image handed to setImage(); the QML property is never read back
    bool m_scaleToFit = false;

    
    Kernel::ILogManager &m_logManager;
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
