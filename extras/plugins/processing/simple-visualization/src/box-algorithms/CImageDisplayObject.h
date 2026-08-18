// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <QtCore>
#include <QQuickView>

#include <openvibe/ov_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

class CImageDisplayObject : public QObject
{
    Q_OBJECT
public:
    CImageDisplayObject(Kernel::ILogManager& logManager, const bool scaleToFit, QQmlEngine* engine);
    virtual ~CImageDisplayObject();

    void setImage(const std::string& image);

    QQuickItem* visualItem() { return m_visualItem; }
signals:

protected:
    QQmlEngine* m_jsEngine = nullptr;
    QQuickItem* m_visualItem = nullptr;

    std::vector<std::string> m_images;
    bool m_scaleToFit = false;

    
    Kernel::ILogManager &m_logManager;
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE