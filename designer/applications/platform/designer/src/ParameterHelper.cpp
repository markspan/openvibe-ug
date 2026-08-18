/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "ParameterHelper.h"

namespace OpenViBE {
namespace Designer {

bool setUIParameters(const Kernel::IKernelContext* context, Kernel::IBox *box, Parameters::ParameterObjects& parameters)
{
    context->getLogManager() << Kernel::LogLevel_Debug << "ParameterHelper::setUIParameters\n";
    if (!box)
    {
        context->getLogManager() << Kernel::LogLevel_Warning  << "ParameterHelper - updateParameters: box is null";
        return false;
    }

    // See if we can be more efficient by only updating the parameters that have changed
    for(size_t i = 0; i < parameters.size(); /* Inc inside loop */ ) {
        // If the parameter is still in the box, we keep it and move on to the next
        if( box->hasSettingWithName(CString(parameters[i]->name().toStdString().c_str())) ){
            i++;
        } else {
            // If the parameter is not in the box anymore, we remove it.
            delete parameters[i];
            parameters.erase(parameters.begin() + i);
        }
    }

    // Add new parameters
    for(size_t i=0; i < box->getSettingCount(); i++) {
        Parameters::ParameterTraits *param;        
        box->getSettingValue(i, param);

        // Check if the parameter is already in the list and skip it if it is
        if (std::find_if(parameters.begin(), parameters.end(), [param](Parameters::ParameterObject* paramObj) {
            return paramObj->name().toStdString() == param->name();
        }) != parameters.end()) {
            continue;
        }

        parameters.insert(parameters.begin() + i, OpenViBE::Parameters::ParameterObject::fromParameter(param));
    }

    return true;
}

}
}