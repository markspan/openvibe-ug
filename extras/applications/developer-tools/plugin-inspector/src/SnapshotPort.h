#pragma once

#include <QObject>
#include <QString>

namespace OpenViBE {
namespace PluginInspector {

/**
 * @brief One input or output port handed to Box.qml when rendering a snapshot.
 *
 * Box.qml's port delegates declare `required property string name` / `dataType`,
 * which only resolve against QObject properties exposed as model roles - a plain
 * QVariantMap does not satisfy them, hence this minimal QObject.
 */
class SnapshotPort final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString dataType READ dataType CONSTANT)

public:
    SnapshotPort(QString name, QString dataType, QObject* parent = nullptr)
        : QObject(parent), m_name(std::move(name)), m_dataType(std::move(dataType)) { }

    QString name() const { return m_name; }
    /// Human readable type name, e.g. "Signal" - this is what StreamColorMapper switches on.
    QString dataType() const { return m_dataType; }

private:
    QString m_name;
    QString m_dataType;
};

}  // namespace PluginInspector
}  // namespace OpenViBE
