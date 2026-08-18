#pragma once

#include "../ovasCConfigurationBuilder.h"
#include <iostream>

namespace OpenViBE {
namespace AcquisitionServer {
class CConfigurationTMSIRefa32B final : public CConfigurationBuilder
{
public:
    explicit CConfigurationTMSIRefa32B(const char* gtkBuilderFilename);

    bool preConfigure() override;
    bool postConfigure() override;
    bool setDeviceList(std::vector<std::string> deviceList, std::string* deviceMaster, std::vector<std::string>* deviceSlaves);
    void buttonRemoveSlaveDevice();
    void buttonAddSlaveDevice();

protected:
    std::string* m_deviceMaster = nullptr;
    std::vector<std::string> m_devices;
    std::vector<std::string>* m_deviceSlaves = nullptr;
    std::vector<std::string> m_deviceSlavesTemp;
};

// Translates a vector of strings to a stream for storing configuration. Used strings cannot contain ';'.
inline std::ostream& operator<<(std::ostream& out, const std::vector<std::string>& var)
{
    for (auto it = var.begin(); it != var.end(); ++it) { out << (*it) << ";"; }
    return out;
}

inline std::istream& operator>>(std::istream& in, std::vector<std::string>& var)
{
    var.clear();
    std::string token;
    while (std::getline(in, token, ';')) { var.push_back(token); }
    return in;
}
}  // namespace AcquisitionServer
}  // namespace OpenViBE
