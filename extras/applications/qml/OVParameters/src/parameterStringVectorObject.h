/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#pragma once

#include "parameterObject.h"
#include <openvibe/parameters/parameterStringVector.h>

namespace OpenViBE {
namespace Parameters {

/**
 * @brief Qt/QML wrapper for a string-vector parameter.
 *
 * Bridges a @c ParameterStringVector (the underlying data model) to the Qt
 * property system so that QML bindings and C++ Qt code can read and write the
 * value as a @c QStringList.  Changes made through the underlying
 * @c ParameterStringVector are forwarded as the @c valueChanged signal.
 */
class OV_PARAMETERS_OBJECT_EXPORT parameterStringVectorObject: public ParameterObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList value READ value WRITE setValue NOTIFY valueChanged)

public:
    /**
     * @brief Constructs a standalone parameter with an optional initial value.
     * @param value Initial string vector (defaults to empty).
     */
    parameterStringVectorObject(const std::vector<std::string>& value = {});

    /**
     * @brief Constructs a named, described parameter with an optional initial value.
     * @param name        Human-readable parameter name.
     * @param description Short description of the parameter's purpose.
     * @param value       Initial string vector (defaults to empty).
     */
    parameterStringVectorObject(const std::string& name, const std::string& description, const std::vector<std::string>& value = {});

    /**
     * @brief Wraps an existing @c ParameterTraits instance.
     *
     * The object does **not** take ownership of @p param; the caller is
     * responsible for its lifetime.
     *
     * @param param Pointer to a @c ParameterStringVector.  Throws
     *              @c std::invalid_argument if the cast fails.
     */
    parameterStringVectorObject(ParameterTraits *param);

    virtual ~parameterStringVectorObject();

    /**
     * @brief Returns the current value as a @c QStringList.
     * @return Copy of the underlying string vector converted to Qt strings.
     */
    QStringList value() const;

    /**
     * @brief Replaces the parameter value from a @c QStringList.
     * @param value New string vector.  Triggers @c valueChanged if the
     *              underlying @c ParameterStringVector notifies observers.
     */
    void setValue(QStringList value);

    /**
     * @brief Returns the element at @p index as a @c QString (QML-invokable).
     * @param index Zero-based index into the vector.
     * @return The string at @p index, or an empty string if out of range.
     */
    Q_INVOKABLE QString getAt(int index) const;

    /**
     * @brief Sets the element at @p index to @p value (QML-invokable).
     * @param index Zero-based index into the vector.
     * @param value New string value.  Out-of-range indices are silently ignored.
     */
    Q_INVOKABLE void setAt(int index, const QString& value);

signals:
    /**
     * @brief Emitted whenever the underlying parameter value changes.
     * @param value The updated string list.
     */
    void valueChanged(const QStringList&);

private:
    /// Subscribes to the underlying @c ParameterStringVector change callback
    /// and forwards notifications as the @c valueChanged Qt signal.
    void connectSignals();

    /// Typed pointer to the underlying parameter; also accessible as @c m_param
    /// through the @c ParameterObject base class.
    ParameterStringVector *m_stringVector = nullptr;

};

DECLARE_PARAMETER_REGISTRATION(parameterStringVectorObject)
} // namespace Parameters
} // namespace OpenViBE