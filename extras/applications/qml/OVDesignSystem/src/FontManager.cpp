// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include <iostream>

#include <QJSEngine>
#include <QDir>
#include <QFontDatabase>

#include "FontManager.h"

namespace OpenViBE {
namespace DesignSystem {

FontManager FontManager::s_instance;

FontManager::FontManager(QObject *parent)
    : QObject(parent)
{ }

void FontManager::initialize() {
    // Load fonts
    std::cout << "[  INF  ] Loading fonts" << "\n";

    for (const QString &fontFolder : fontFolders) {
        QDir fontsDir(fontFolder);
        QStringList filters;
        filters << "*.ttf";
        
        QFileInfoList fontFiles = fontsDir.entryInfoList(filters, QDir::Files);
        if (fontFiles.isEmpty()) {
            std::cout << "[  ERR  ] No font files found in " << fontFolder.toStdString() << "\n";
            continue;
        }
        
        for (const QFileInfo &fileInfo : fontFiles) {
            QString fontPath = fileInfo.absoluteFilePath();
            QFontDatabase::addApplicationFont(fontPath);
        }
    }
}

FontManager* FontManager::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)

    FontManager *manager = &FontManager::instance();
    QJSEngine::setObjectOwnership(manager, QJSEngine::CppOwnership);
    return manager;
}

FontManager& FontManager::instance()
{
    static bool m_isInitialized = false;
    if (!m_isInitialized) {
        s_instance.initialize();
        m_isInitialized = true;
    }
    return s_instance;
}

} // namespace DesignSystem
} // namespace OpenViBE