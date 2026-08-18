#pragma once

#include <ov_common_defines.h>
#include <openvibe/parameters/parameterTraits.h>

#include <list>

namespace OpenViBE {
namespace Services {

class IService
{
public:
    virtual ~IService() = default;

    virtual std::list<Parameters::ParameterTraits*> parameters() = 0;
};

extern "C"
{
    OV_API_Export IService* getServiceInstance();
}

}
}