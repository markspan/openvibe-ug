#pragma once

#include <ovas_base.h>
#include "ovasIHeader.h"

#include <map>

/*
 * \brief Operators used to convert between typical variables (as used in properties & settings) and streams
 *
 * 
 */

namespace OpenViBE {
namespace AcquisitionServer {
inline std::ostream& operator<<(std::ostream& out, const CString& var)
{
    out << std::string(var.toASCIIString());
    return out;
}

inline std::istream& operator>>(std::istream& in, CString& var)
{
    std::string tmp;
    std::getline(in, tmp);
    var.set(tmp.c_str());
    // std::cout << "Parsed [" << var.toASCIIString() << "]\n";
    return in;
}

// Writes fields of IHeader to a stream
inline std::ostream& operator<<(std::ostream& out, const IHeader& var)
{
    if (var.isExperimentIDSet()) { out << "ExperimentID " << var.getExperimentID() << " "; }
    if (var.isSubjectAgeSet()) { out << "SubjectAge " << var.getSubjectAge() << " "; }
    if (var.isSubjectGenderSet()) { out << "SubjectGender " << var.getSubjectGender() << " "; }
    out << "ImpedanceCheck " << var.isImpedanceCheckRequested() << " ";
    if (var.isSamplingFrequencySet()) { out << "SamplingFrequency " << var.getSamplingFrequency() << " "; }
    if (var.isChannelCountSet()) { out << "Channels " << var.getChannelCount() << " "; }

    if (var.isChannelCountSet() && var.isChannelNameSet()) {
        out << "Names ";
        for (size_t i = 0; i < var.getChannelCount(); ++i) { out << var.getChannelName(i) << ";"; }
        out << " ";
    }

    if (var.isChannelCountSet() && var.isChannelGainSet()) {
        out << "Gains ";
        for (size_t i = 0; i < var.getChannelCount(); ++i) { out << var.getChannelGain(i) << " "; }
    }

    return out;
}

// Reads fields of IHeader from a stream
inline std::istream& operator>>(std::istream& in, IHeader& var)
{
    std::string token;

    while (std::getline(in, token, ' ')) {
        if (token.empty()) { continue; }

        // std::cout << "Got token [" << token << "]\n";

        if (token == "ExperimentID") {
            // std::cout << "Parsing experiment id\n";
            uint32_t tmp;
            in >> tmp;
            var.setExperimentID(tmp);
        }
        else if (token == "SubjectAge") {
            // std::cout << "Parsing age\n";
            uint32_t tmp;
            in >> tmp;
            var.setSubjectAge(tmp);
        }
        else if (token == "SubjectGender") {
            // std::cout << "Parsing gender\n";
            uint32_t tmp;
            in >> tmp;
            var.setSubjectGender(tmp);
        }
        else if (token == "ImpedanceCheck") {
            // std::cout << "Parsing impedance check\n";
            bool tmp;
            in >> tmp;
            var.setImpedanceCheckRequested(tmp);
        }
        else if (token == "SamplingFrequency") {
            // std::cout << "Parsing freq\n";

            uint32_t tmp;
            in >> tmp;
            var.setSamplingFrequency(tmp);

            // std::cout << "  Got " << sampling << "\n";
        }
        else if (token == "Channels") {
            // std::cout << "Parsing chn count\n";
            uint32_t tmp;
            in >> tmp;
            var.setChannelCount(tmp);

            // std::cout << "  Got " << nChannels << "\n";
        }
        else if (token == "Names" && var.isChannelCountSet()) {
            // std::cout << "Parsing names\n";

            std::string tmp;
            for (size_t i = 0; i < var.getChannelCount(); ++i) {
                std::getline(in, tmp, ';');
                // std::cout << "  Parsed " << value << "\n";
                var.setChannelName(i, tmp.c_str());
            }
        }
        else if (token == "Gains" && var.isChannelCountSet()) {
            // std::cout << "Parsing gains\n";
            float tmp;
            for (size_t i = 0; i < var.getChannelCount(); ++i) {
                in >> tmp;
                var.setChannelGain(i, tmp);
                // std::cout << "  Parsed " << tmpGain << "\n";
            }
        }
        else { }// std::cout << "Unexpected token [" << token << "]\n";
    }

    return in;
}

inline std::ostream& operator<<(std::ostream& out, const std::map<uint32_t, uint32_t>& var)
{
    for (auto it = var.begin(); it != var.end(); ++it) { out << it->first << " " << it->second << " "; }
    return out;
}

inline std::istream& operator>>(std::istream& in, std::map<uint32_t, uint32_t>& var)
{
    var.clear();
    uint32_t key;
    uint32_t value;
    while (in >> key) {
        in >> value;
        var[key] = value;
    }

    return in;
}
}  // namespace AcquisitionServer
}  // namespace OpenViBE
