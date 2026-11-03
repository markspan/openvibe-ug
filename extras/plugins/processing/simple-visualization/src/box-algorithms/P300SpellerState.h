// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see speller.md

#pragma once

#include <cstdint>
#include <deque>
#include <list>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

/// What is shown in a cell. The values index the color and font size lists of P300SpellerVisualization.qml.
enum class EP300CellStyle { NoFlash = 0, Flash = 1, Target = 2, Selected = 3 };

/// The letter grid of a speller.
struct SP300SpellerLayout
{
    int rows    = 0;
    int columns = 0;
    std::vector<std::string> labels;    ///< rows * columns, row after row

    const std::string& label(const int row, const int column) const { return labels[size_t(row * columns + column)]; }
};

/**
 * \brief Reads the letter grid from an interface file: the GtkTable "p300-speller-table" of a GtkBuilder file.
 *
 * Those files predate the Qt port. They stay the format so that a scenario's "Interface filename" setting, and a
 * lab's own grid, keep working. Only the table is used from them.
 *
 * \return false with a message in error if the file holds no such table.
 */
bool loadP300SpellerLayout(const std::string& path, SP300SpellerLayout& layout, std::string& error);

/**
 * \brief What the speller shows: the style of each cell, and the target and result text.
 *
 * Free of Qt and of the player, to be tested on its own. The box drives it, the view shows it.
 */
class CP300SpellerState
{
public:
    explicit CP300SpellerState(SP300SpellerLayout layout);

    void clearCells();                      ///< Every cell back to no flash
    void flash(int row, int column);        ///< The row or column flashes, every other cell goes back to no flash. -1 for the other one.
    void showTarget(int row, int column);   ///< Marks the cell and adds its letter to the target text
    void showSelected(int row, int column); ///< Marks the cell and adds its letter to the result, colored by the oldest target
    void rejectSelection();                 ///< Adds a '*' to the result
    void clearText();

    const std::vector<int>& styles() const { return m_styles; }
    const std::string& targetText() const { return m_target; }
    const std::string& resultMarkup() const { return m_result; }    ///< In Qt's styled text

    /// True when something changed since the last call: a change is what needs a new frame.
    bool takeChanged();

private:
    void setStyle(size_t index, EP300CellStyle style);

    SP300SpellerLayout m_layout;
    std::vector<int> m_styles;
    std::string m_target;
    std::string m_result;
    std::list<std::pair<int, int>> m_targetHistory;
    bool m_changed = false;
};

/**
 * \brief Tells when an update of the view reached the screen, and at what time.
 *
 * The GUI thread writes an update into the QML item. It is on screen with the first frame that was synchronized after
 * that write: a frame swapped earlier was already being rendered, and would date the update too early. The player thread
 * takes the results. Called from the GUI thread, the render thread and the player thread.
 */
class CP300FrameTracker
{
public:
    void onWritten(uint64_t update);                ///< GUI thread: the update is in the QML item
    void onDisplayedWithoutFrame(uint64_t update, uint64_t time);   ///< GUI thread: no window to render it, so it will not get a frame
    void onSynchronized();                          ///< The scene graph took over what was written
    void onSwapped(uint64_t time);                  ///< The frame of the last synchronization is on its way to the screen

    /// Player thread. The next displayed update, and when; false if there is none. An update reported here includes all earlier ones.
    bool takeDisplayed(uint64_t& update, uint64_t& time);

private:
    std::mutex m_mutex;
    uint64_t m_written = 0;
    uint64_t m_synced  = 0;
    uint64_t m_stamped = 0;
    std::deque<std::pair<uint64_t, uint64_t>> m_displayed;
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
