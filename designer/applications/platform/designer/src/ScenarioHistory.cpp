// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "ScenarioHistory.h"

#include <QUndoCommand>

namespace OpenViBE {
namespace Designer {

class ScenarioHistory::SnapshotUndoCommand : public QUndoCommand
{
public:
    SnapshotUndoCommand(const RestoreCallback& restore, const QByteArray& before, const QByteArray& after, const QString& label)
        : QUndoCommand(label), m_restore(restore), m_before(before), m_after(after) {}

    void undo() override { m_restore(m_before); }

    void redo() override
    {
        // QUndoStack::push() calls redo() right away, but the action has already been applied
        // to the scenario by the time it is recorded.
        if (m_firstRedo)
        {
            m_firstRedo = false;
        }
        else
        {
            m_restore(m_after);
        }
    }

private:
    const RestoreCallback& m_restore;
    QByteArray m_before;
    QByteArray m_after;
    bool m_firstRedo = true;
};

ScenarioHistory::ScenarioHistory(int limit, RestoreCallback restore, QObject* parent)
    : QObject(parent), m_restore(std::move(restore))
{
    // Only honoured while the stack is empty.
    m_stack.setUndoLimit(limit);

    connect(&m_stack, &QUndoStack::canUndoChanged, this, &ScenarioHistory::changed);
    connect(&m_stack, &QUndoStack::canRedoChanged, this, &ScenarioHistory::changed);
    connect(&m_stack, &QUndoStack::undoTextChanged, this, &ScenarioHistory::changed);
    connect(&m_stack, &QUndoStack::redoTextChanged, this, &ScenarioHistory::changed);
    connect(&m_stack, &QUndoStack::cleanChanged, this, &ScenarioHistory::changed);
}

void ScenarioHistory::push(const QByteArray& before, const QByteArray& after, const QString& label)
{
    m_stack.push(new SnapshotUndoCommand(m_restore, before, after, label));
}

}  // namespace Designer
}  // namespace OpenViBE
