// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <QObject>
#include <QString>
#include <qqml.h>

namespace OpenViBE {
namespace DesignSystem {

class FontManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    
    Q_PROPERTY(QString textFontName READ textFontName NOTIFY textFontNameChanged)
    Q_PROPERTY(QString titleFontName READ titleFontName NOTIFY titleFontNameChanged)

public:
    static FontManager* create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static FontManager& instance();

    QString textFontName() const { return "Be Vietnam Pro"; }
    QString titleFontName() const { return "Space Grotesk"; }

    void initialize();

signals:
    void textFontNameChanged();
    void titleFontNameChanged();

private:
    explicit FontManager(QObject *parent = nullptr);
    static FontManager s_instance;

    QStringList fontFolders = { ":/qt/qml/DesignSystem/resources/fonts/Be_Vietnam_Pro/",
                                ":/qt/qml/DesignSystem/resources/fonts/Space_Grotesk/static/" };
};

} // namespace DesignSystem
} // namespace OpenViBE