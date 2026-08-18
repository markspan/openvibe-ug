#pragma once

#include <map>
#include <string>

#include "ovas_base.h"
#include "ovasIHeader.h"

namespace OpenViBE {
namespace AcquisitionServer {

inline static bool loadParametersFromConfig(const Kernel::IConfigurationManager& cfgManager,const std::map<std::string, OpenViBE::Parameters::ParameterTraits*> & parameters, std::string prefix = "", IHeader* header = nullptr) {
    bool res = true;
    for (const auto& [key, param] : parameters) {
        const std::string name = prefix + "_" + key;
        if (cfgManager.lookUpConfigurationTokenIdentifier(name.c_str()) != CIdentifier::undefined()) {
            const CString value = cfgManager.expand(CString("${") + name.c_str() + CString("}") );
            param->fromString(value.toASCIIString());
        } else {
            std::cout << "Parameter Token " << key << " not found. Full Identifier: " << name << std::endl;
            res = false;
        }
    }

    if(header) {
        const std::string name = prefix + "_Header";
        if (cfgManager.lookUpConfigurationTokenIdentifier(name.c_str()) != CIdentifier::undefined()) {
            const CString value = cfgManager.expand(CString("${") + name.c_str() + CString("}") );
            std::cout << "ParameterHelper::loadParametersFromConfig: headerAsString: " << value << std::endl;
            header->fromString(value.toASCIIString());
        } else {
            std::cout << "Token Header not found for prefix " << prefix << std::endl;
            res = false;
        }
    }

    return res;
}

inline static bool loadParametersFromConfig(const Kernel::IConfigurationManager& cfgManager,const OpenViBE::Parameters::Parameters & parameters, std::string prefix = "", IHeader* header = nullptr) {
    std::map<std::string, OpenViBE::Parameters::ParameterTraits*> params;
    for(auto *param : parameters) {
        params[param->name()] = param;
    }
    return loadParametersFromConfig(cfgManager, params, prefix, header);
}

inline static bool saveParametersToConfig(Kernel::IConfigurationManager& cfgManager, const std::map<std::string, OpenViBE::Parameters::ParameterTraits*> & parameters, std::string prefix = "", const IHeader* header = nullptr) {
    bool res = true;
    for (const auto& [key, param] : parameters) {
        const std::string name = prefix + "_" + key;
        const std::string paramAsString = param->toString();
        CIdentifier id     = cfgManager.lookUpConfigurationTokenIdentifier(name.c_str());
        if (id == CIdentifier::undefined()) {
            cfgManager.createConfigurationToken(name.c_str(), paramAsString.c_str()); }
        else {
            // replacing token value
            cfgManager.setConfigurationTokenValue(id, paramAsString.c_str());
        }
    }

    if(header) {
        const std::string name = prefix + "_Header";
        const std::string headerAsString = header->toString();
        CIdentifier id     = cfgManager.lookUpConfigurationTokenIdentifier(name.c_str());
        if (id == CIdentifier::undefined()) {
            cfgManager.createConfigurationToken(name.c_str(), headerAsString.c_str()); }
        else {
            // replacing token value
            cfgManager.setConfigurationTokenValue(id, headerAsString.c_str());
        }
    }

    return res;
}

inline static bool saveParametersToConfig(Kernel::IConfigurationManager& cfgManager,const OpenViBE::Parameters::Parameters & parameters, std::string prefix = "", const IHeader* header = nullptr) {
    std::map<std::string, OpenViBE::Parameters::ParameterTraits*> params;
    for(auto *param : parameters) {
        params[param->name()] = param;
    }
    return saveParametersToConfig(cfgManager, params, prefix, header);
}


}  // namespace AcquisitionServer
}  // namespace OpenViBE