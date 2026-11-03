// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see speller.md

#include "P300SpellerState.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

using namespace OpenViBE::Plugins::SimpleVisualization;

namespace {

SP300SpellerLayout layout2x3()
{
    SP300SpellerLayout layout;
    layout.rows    = 2;
    layout.columns = 3;
    layout.labels  = {"A", "B", "C", "D", "<", "F"};
    return layout;
}

int style(const CP300SpellerState& state, const int row, const int column) { return state.styles()[size_t(row * 3 + column)]; }

const int noFlash  = int(EP300CellStyle::NoFlash);
const int flash    = int(EP300CellStyle::Flash);
const int target   = int(EP300CellStyle::Target);
const int selected = int(EP300CellStyle::Selected);

std::string writeFile(const std::string& name, const std::string& text)
{
    const std::string path = testing::TempDir() + name;
    std::ofstream(path) << text;
    return path;
}

// A cell as the interface files have it: an event box holding a label. The first cell of the table has no packing at all.
std::string cell(const std::string& label, const std::string& packing)
{
    return "<child><object class=\"GtkEventBox\" id=\"e" + label + "\"><child><object class=\"GtkLabel\" id=\"l" + label + "\">"
           "<property name=\"label\" translatable=\"yes\">" + label + "</property></object></child></object>"
           + (packing.empty() ? "" : "<packing>" + packing + "</packing>") + "</child>";
}

std::string attach(const char* name, const int value) { return std::string("<property name=\"") + name + "\">" + std::to_string(value) + "</property>"; }

}  // namespace

TEST(P300SpellerLayoutTest, ReadsTheGridLeavingOutDefaults)
{
    const std::string path = writeFile("speller-grid.ui", "<?xml version=\"1.0\"?><interface><object class=\"GtkWindow\" id=\"main\"><child>"
        "<object class=\"GtkTable\" id=\"p300-speller-table\"><property name=\"n_rows\">2</property><property name=\"n_columns\">2</property>"
        + cell("A", "")
        + cell("B", attach("left_attach", 1) + attach("right_attach", 2))
        + cell("C", attach("top_attach", 1) + attach("bottom_attach", 2))
        + cell("D", attach("left_attach", 1) + attach("right_attach", 2) + attach("top_attach", 1) + attach("bottom_attach", 2))
        + "</object></child><child><object class=\"GtkLabel\" id=\"label-result\"><property name=\"label\">not a cell</property></object></child>"
          "</object></interface>");

    SP300SpellerLayout layout;
    std::string error;
    ASSERT_TRUE(loadP300SpellerLayout(path, layout, error)) << error;
    EXPECT_EQ(layout.rows, 2);
    EXPECT_EQ(layout.columns, 2);
    EXPECT_EQ(layout.label(0, 0), "A");
    EXPECT_EQ(layout.label(0, 1), "B");
    EXPECT_EQ(layout.label(1, 0), "C");
    EXPECT_EQ(layout.label(1, 1), "D");
    std::remove(path.c_str());
}

TEST(P300SpellerLayoutTest, SaysWhatIsWrongWithAFile)
{
    SP300SpellerLayout layout;
    std::string error;
    EXPECT_FALSE(loadP300SpellerLayout(testing::TempDir() + "there-is-no-such-file.ui", layout, error));
    EXPECT_NE(error.find("can not read"), std::string::npos);

    const std::string path = writeFile("speller-no-table.ui", "<interface><object class=\"GtkWindow\" id=\"main\"/></interface>");
    EXPECT_FALSE(loadP300SpellerLayout(path, layout, error));
    EXPECT_NE(error.find("p300-speller-table"), std::string::npos);
    std::remove(path.c_str());
}

TEST(P300SpellerStateTest, FlashesARowThenAColumn)
{
    CP300SpellerState state(layout2x3());
    state.flash(1, -1);
    EXPECT_EQ(style(state, 1, 0), flash);
    EXPECT_EQ(style(state, 1, 2), flash);
    EXPECT_EQ(style(state, 0, 0), noFlash);

    // The row goes out as the column comes on
    state.flash(-1, 2);
    EXPECT_EQ(style(state, 0, 2), flash);
    EXPECT_EQ(style(state, 1, 2), flash);
    EXPECT_EQ(style(state, 1, 0), noFlash);
}

TEST(P300SpellerStateTest, AFlashAlsoClearsTheTargetCell)
{
    CP300SpellerState state(layout2x3());
    state.showTarget(0, 1);
    EXPECT_EQ(style(state, 0, 1), target);
    state.flash(1, -1);
    EXPECT_EQ(style(state, 0, 1), noFlash);
}

TEST(P300SpellerStateTest, TargetsAddUp)
{
    CP300SpellerState state(layout2x3());
    state.showTarget(0, 0);
    state.showTarget(1, 2);
    EXPECT_EQ(state.targetText(), "AF");
    EXPECT_EQ(style(state, 0, 0), target);
    EXPECT_EQ(style(state, 1, 2), target);
}

TEST(P300SpellerStateTest, ColorsAResultByItsTarget)
{
    CP300SpellerState state(layout2x3());
    state.showTarget(0, 0);
    state.showTarget(0, 0);
    state.showTarget(0, 0);

    state.showSelected(0, 0);
    EXPECT_EQ(style(state, 0, 0), selected);
    const std::string correct = state.resultMarkup();
    EXPECT_NE(correct.find("#4cc94c"), std::string::npos);
    EXPECT_NE(correct.find(">A<"), std::string::npos);

    state.showSelected(0, 1);       // Right row, wrong column
    EXPECT_NE(state.resultMarkup().find("#ffa726"), std::string::npos);

    state.showSelected(1, 1);       // Neither
    EXPECT_NE(state.resultMarkup().find("#ff6b5e"), std::string::npos);
    EXPECT_NE(state.resultMarkup().find("&lt;"), std::string::npos);
}

TEST(P300SpellerStateTest, AResultWithoutTargetHasNoColor)
{
    CP300SpellerState state(layout2x3());
    state.showSelected(0, 2);
    EXPECT_EQ(state.resultMarkup(), "C");
    state.rejectSelection();
    EXPECT_EQ(state.resultMarkup(), "C*");
}

TEST(P300SpellerStateTest, ClearingTextKeepsTheCells)
{
    CP300SpellerState state(layout2x3());
    state.showTarget(0, 0);
    state.clearText();
    EXPECT_EQ(state.targetText(), "");
    EXPECT_EQ(style(state, 0, 0), target);

    state.clearCells();
    EXPECT_EQ(style(state, 0, 0), noFlash);
}

TEST(P300SpellerStateTest, ReportsAChangeOnlyWhenThereIsOne)
{
    CP300SpellerState state(layout2x3());
    EXPECT_FALSE(state.takeChanged());

    state.flash(0, -1);
    EXPECT_TRUE(state.takeChanged());
    EXPECT_FALSE(state.takeChanged());

    state.flash(0, -1);     // The same flash draws nothing new: there would be no frame to wait for
    EXPECT_FALSE(state.takeChanged());

    state.clearText();      // Nothing to clear
    EXPECT_FALSE(state.takeChanged());

    state.clearCells();
    EXPECT_TRUE(state.takeChanged());
}

TEST(P300FrameTrackerTest, AnUpdateIsOnScreenWithTheFrameSynchronizedAfterIt)
{
    CP300FrameTracker tracker;
    uint64_t update = 0, time = 0;

    tracker.onWritten(1);
    tracker.onSwapped(100);     // This frame was already being rendered
    EXPECT_FALSE(tracker.takeDisplayed(update, time));

    tracker.onSynchronized();
    EXPECT_FALSE(tracker.takeDisplayed(update, time));
    tracker.onSwapped(116);
    ASSERT_TRUE(tracker.takeDisplayed(update, time));
    EXPECT_EQ(update, 1u);
    EXPECT_EQ(time, 116u);

    // Later frames say nothing new
    tracker.onSynchronized();
    tracker.onSwapped(132);
    EXPECT_FALSE(tracker.takeDisplayed(update, time));
}

TEST(P300FrameTrackerTest, UpdatesInOneFrameAreReportedOnce)
{
    CP300FrameTracker tracker;
    uint64_t update = 0, time = 0;

    tracker.onWritten(1);
    tracker.onWritten(2);
    tracker.onSynchronized();
    tracker.onSwapped(100);
    ASSERT_TRUE(tracker.takeDisplayed(update, time));
    EXPECT_EQ(update, 2u);
    EXPECT_FALSE(tracker.takeDisplayed(update, time));
}

TEST(P300FrameTrackerTest, AHiddenWindowNeedsNoFrame)
{
    CP300FrameTracker tracker;
    uint64_t update = 0, time = 0;

    tracker.onDisplayedWithoutFrame(1, 50);
    ASSERT_TRUE(tracker.takeDisplayed(update, time));
    EXPECT_EQ(update, 1u);
    EXPECT_EQ(time, 50u);

    // It stays reported once, also when frames return
    tracker.onSynchronized();
    tracker.onSwapped(100);
    EXPECT_FALSE(tracker.takeDisplayed(update, time));
}
