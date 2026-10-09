// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <QString>
#include <string>

namespace OpenViBE {
namespace Designer {
namespace DocumentationUrl {

/**
 * @brief Builds the documentation page slug from a box algorithm name.
 *
 * Mirrors the plugin inspector Transform() (which names the generated pages) and the legacy designer
 * getBoxAlgorithmURL(): alphanumeric characters are kept and the first character following a dropped
 * separator is upper-cased. "Signal Display" gives "SignalDisplay", "CSV File Writer" gives "CSVFileWriter".
 * @param name Name of the box algorithm, as returned by the plugin object descriptor.
 * @return The slug used in the Doc_BoxAlgorithm_<slug>.html page name.
 */
std::string boxAlgorithmSlug(const std::string& name);

/**
 * @brief Turns an expanded configuration value into a URL that a browser can open.
 *
 * A remote URL (http, https, ...) is returned unchanged. A local path is normalized, checked for
 * existence and converted to a file URL, so that a missing page can be reported instead of
 * opening a "file not found" browser tab.
 * @param target A URL or a local file path, e.g. the expansion of ${Designer_HelpBrowserURLBase}.
 * @return The URL to open, or an empty string when the target is a local file that does not exist.
 */
QString toBrowsableUrl(const QString& target);

}  // namespace DocumentationUrl
}  // namespace Designer
}  // namespace OpenViBE
