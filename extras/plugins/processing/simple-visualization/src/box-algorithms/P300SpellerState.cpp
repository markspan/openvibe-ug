// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see speller.md

#include "P300SpellerState.h"

#include <tinyxml2.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

namespace {

const char* propertyText(const tinyxml2::XMLElement* parent, const char* name)
{
    for (auto* property = parent->FirstChildElement("property"); property; property = property->NextSiblingElement("property")) {
        const char* attribute = property->Attribute("name");
        if (attribute && std::strcmp(attribute, name) == 0) { return property->GetText() ? property->GetText() : ""; }
    }
    return nullptr;
}

int propertyInt(const tinyxml2::XMLElement* parent, const char* name, const int fallback)
{
    const char* text = propertyText(parent, name);
    return text ? std::atoi(text) : fallback;
}

// The first object of the given id under this element, at any depth
const tinyxml2::XMLElement* findObject(const tinyxml2::XMLElement* element, const char* id)
{
    for (auto* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (std::strcmp(child->Name(), "object") == 0) {
            const char* childId = child->Attribute("id");
            if (childId && std::strcmp(childId, id) == 0) { return child; }
        }
        if (const auto* found = findObject(child, id)) { return found; }
    }
    return nullptr;
}

// The text of the first label under this element, at any depth
const char* findLabel(const tinyxml2::XMLElement* element)
{
    if (std::strcmp(element->Name(), "object") == 0) {
        const char* objectClass = element->Attribute("class");
        if (objectClass && std::strcmp(objectClass, "GtkLabel") == 0) { return propertyText(element, "label"); }
    }
    for (auto* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (const char* label = findLabel(child)) { return label; }
    }
    return nullptr;
}

std::string escapeMarkup(const std::string& text)
{
    std::string escaped;
    for (const char c : text) {
        switch (c) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

}  // namespace

bool loadP300SpellerLayout(const std::string& path, SP300SpellerLayout& layout, std::string& error)
{
    tinyxml2::XMLDocument document;
    if (document.LoadFile(path.c_str()) != tinyxml2::XML_SUCCESS) {
        error = "can not read " + path;
        return false;
    }

    const tinyxml2::XMLElement* root  = document.RootElement();
    const tinyxml2::XMLElement* table = root ? findObject(root, "p300-speller-table") : nullptr;
    if (!table) {
        error = path + " has no table named p300-speller-table";
        return false;
    }

    layout.rows    = propertyInt(table, "n_rows", 0);
    layout.columns = propertyInt(table, "n_columns", 0);
    if (layout.rows <= 0 || layout.columns <= 0) {
        error = path + " does not say how many rows and columns its table has";
        return false;
    }
    layout.labels.assign(size_t(layout.rows * layout.columns), "");

    for (auto* child = table->FirstChildElement("child"); child; child = child->NextSiblingElement("child")) {
        const char* label = findLabel(child);
        if (!label) { continue; }

        // A file leaves out the values that are the default, down to the whole packing of the first cell
        const tinyxml2::XMLElement* packing = child->FirstChildElement("packing");
        const int left   = packing ? propertyInt(packing, "left_attach", 0) : 0;
        const int right  = packing ? propertyInt(packing, "right_attach", left + 1) : left + 1;
        const int top    = packing ? propertyInt(packing, "top_attach", 0) : 0;
        const int bottom = packing ? propertyInt(packing, "bottom_attach", top + 1) : top + 1;
        for (int row = std::max(top, 0); row < std::min(bottom, layout.rows); ++row) {
            for (int column = std::max(left, 0); column < std::min(right, layout.columns); ++column) {
                layout.labels[size_t(row * layout.columns + column)] = label;
            }
        }
    }
    return true;
}

CP300SpellerState::CP300SpellerState(SP300SpellerLayout layout)
    : m_layout(std::move(layout)), m_styles(size_t(m_layout.rows * m_layout.columns), int(EP300CellStyle::NoFlash)) { }

void CP300SpellerState::setStyle(const size_t index, const EP300CellStyle style)
{
    if (m_styles[index] != int(style)) {
        m_styles[index] = int(style);
        m_changed       = true;
    }
}

void CP300SpellerState::clearCells()
{
    for (size_t i = 0; i < m_styles.size(); ++i) { setStyle(i, EP300CellStyle::NoFlash); }
}

void CP300SpellerState::flash(const int row, const int column)
{
    for (int r = 0; r < m_layout.rows; ++r) {
        for (int c = 0; c < m_layout.columns; ++c) {
            const bool inFlash = (row != -1 && column != -1) ? (r == row && c == column) : (r == row || c == column);
            setStyle(size_t(r * m_layout.columns + c), inFlash ? EP300CellStyle::Flash : EP300CellStyle::NoFlash);
        }
    }
}

void CP300SpellerState::showTarget(const int row, const int column)
{
    setStyle(size_t(row * m_layout.columns + column), EP300CellStyle::Target);
    m_target += m_layout.label(row, column);
    m_targetHistory.emplace_back(row, column);
    m_changed = true;
}

void CP300SpellerState::showSelected(const int row, const int column)
{
    setStyle(size_t(row * m_layout.columns + column), EP300CellStyle::Selected);

    const std::string label = escapeMarkup(m_layout.label(row, column));
    if (m_targetHistory.empty()) { m_result += label; }
    else {
        const std::pair<int, int> target = m_targetHistory.front();
        m_targetHistory.pop_front();

        const bool correct     = target.first == row && target.second == column;
        const bool halfCorrect = target.first == row || target.second == column;
        // Readable on the dark bar under the grid
        const char* color = correct ? "#4cc94c" : (halfCorrect ? "#ffa726" : "#ff6b5e");
        m_result += std::string("<font color=\"") + color + "\">" + label + "</font>";
    }
    m_changed = true;
}

void CP300SpellerState::rejectSelection()
{
    m_result += "*";
    m_changed = true;
}

void CP300SpellerState::clearText()
{
    if (!m_target.empty() || !m_result.empty()) { m_changed = true; }
    m_target.clear();
    m_result.clear();
}

bool CP300SpellerState::takeChanged()
{
    const bool changed = m_changed;
    m_changed          = false;
    return changed;
}

void CP300FrameTracker::onWritten(const uint64_t update)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_written = update;
}

void CP300FrameTracker::onDisplayedWithoutFrame(const uint64_t update, const uint64_t time)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_written = update;
    m_synced  = update;
    m_stamped = update;
    m_displayed.emplace_back(update, time);
}

void CP300FrameTracker::onSynchronized()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_synced = m_written;
}

void CP300FrameTracker::onSwapped(const uint64_t time)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_synced > m_stamped) {
        m_stamped = m_synced;
        m_displayed.emplace_back(m_synced, time);
    }
}

bool CP300FrameTracker::takeDisplayed(uint64_t& update, uint64_t& time)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_displayed.empty()) { return false; }
    update = m_displayed.front().first;
    time   = m_displayed.front().second;
    m_displayed.pop_front();
    return true;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
