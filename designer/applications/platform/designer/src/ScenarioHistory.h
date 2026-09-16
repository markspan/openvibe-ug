// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <functional>

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QUndoStack>

namespace OpenViBE {
namespace Designer {

/**
 * @brief Undo/redo history of a scenario, as a stack of whole-scenario snapshots.
 *
 * Each entry holds the serialized scenario before and after one user action (memento pattern);
 * undoing or redoing hands the matching snapshot to the `restore` callback. Snapshots are
 * QByteArrays, so consecutive entries share their common buffer through implicit sharing.
 *
 * Wraps a QUndoStack to get the undo limit, the undo/redo texts and the clean state (dirty
 * marker) for free.
 */
class ScenarioHistory : public QObject
{
    Q_OBJECT

public:
    using RestoreCallback = std::function<bool(const QByteArray& snapshot)>;

    ScenarioHistory(int limit, RestoreCallback restore, QObject* parent = nullptr);

    /** Records one action. `before`/`after` are the compressed snapshots around it. */
    void push(const QByteArray& before, const QByteArray& after, const QString& label);

    bool canUndo() const { return m_stack.canUndo(); }
    bool canRedo() const { return m_stack.canRedo(); }
    QString undoText() const { return m_stack.undoText(); }
    QString redoText() const { return m_stack.redoText(); }
    bool isClean() const { return m_stack.isClean(); }

    void setClean() { m_stack.setClean(); }
    void undo() { m_stack.undo(); }
    void redo() { m_stack.redo(); }

signals:
    /** Emitted whenever canUndo/canRedo/undoText/redoText/isClean may have changed. */
    void changed();

private:
    class SnapshotUndoCommand;

    RestoreCallback m_restore;
    QUndoStack m_stack;
};

}  // namespace Designer
}  // namespace OpenViBE
