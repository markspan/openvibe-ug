/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once

#include <openvibe/ov_all.h>

#include <QMetaMethod>
#include <QObject>

#include <parametersObject.h>

namespace OpenViBE {
namespace Designer {

/**
 * @brief setUIParameters Update list of parameter objects from a box or scenario
 * @param context The kernel context
 * @param box The box or scenario to get the parameters from
 * @param parameters The list of parameters to update
 * 
 * @return true if the parameters were updated, false otherwise
 */
bool setUIParameters(const Kernel::IKernelContext* context, Kernel::IBox *box, Parameters::ParameterObjects& parameters);

/**
 * @brief Connects the `valueChanged` signal of every parameter object to `slot` on `receiver`.
 *
 * Each ParameterObject subclass declares its own typed `valueChanged(T)`, so the connection is
 * made through the meta-object; `slot` must take no argument. Already connected pairs are skipped.
 */
void connectParameterValueChanged(const Parameters::ParameterObjects& parameters, QObject* receiver, const QMetaMethod& slot);

}
}
