// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "DocumentationUrl.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>

#include <cctype>

namespace OpenViBE {
namespace Designer {
namespace DocumentationUrl {

std::string boxAlgorithmSlug(const std::string& name)
{
    std::string slug;
    bool upperNext = false;
    for (const char c : name)
    {
        if (std::isalnum(static_cast<unsigned char>(c)))
        {
            slug += upperNext ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c;
            upperNext = false;
        }
        else
        {
            upperNext = true;
        }
    }
    return slug;
}

QString toBrowsableUrl(const QString& target)
{
    QString result;
    const QUrl url(target);
    // A one-letter scheme is a Windows drive letter ("C:/..."), not a real scheme.
    const bool isRemote = url.scheme().size() > 1 && !url.isLocalFile();
    if (isRemote)
    {
        result = target;
    }
    else
    {
        // QDir::cleanPath collapses the "bin/.." of the configured base.
        const QString localPath = url.isLocalFile() ? url.toLocalFile() : target;
        const QFileInfo info(QDir::cleanPath(localPath));
        if (info.isFile())
        {
            result = QUrl::fromLocalFile(info.absoluteFilePath()).toString();
        }
    }
    return result;
}

}  // namespace DocumentationUrl
}  // namespace Designer
}  // namespace OpenViBE
