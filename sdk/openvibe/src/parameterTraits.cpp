/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include <iterator>
#include <algorithm>
#include "parameters/parameterTraits.h"

namespace OpenViBE {
namespace Parameters {

ParameterTraits::ParameterTraits() {}

ParameterTraits::ParameterTraits(const std::string& name, const std::string& description)
    : m_name(name)
    , m_description(description)
{ }

ParameterTraits::ParameterTraits(const ParameterTraits& param)
{
    m_name = param.m_name;
    m_description = param.m_description;
    m_connections = param.m_connections;
}

ParameterTraits::~ParameterTraits()
{
    for (auto& [id, cb] : m_onDestroyCallbacks)
        cb();
    disconnectAllConnections();
}

uint64_t ParameterTraits::onDestroy(std::function<void()> callback) {
    m_onDestroyCallbacks[m_nextDestroyId] = std::move(callback);
    return m_nextDestroyId++;
}

void ParameterTraits::cancelOnDestroy(uint64_t id) {
    m_onDestroyCallbacks.erase(id);
}

uint64_t ParameterTraits::connect(std::function<void(std::any)> slot)
{
    m_connections[m_nextConnectionId] = slot;
    return m_nextConnectionId++;
}

void ParameterTraits::disconnect(uint64_t id) {
    m_connections.erase(id);
}

void ParameterTraits::disconnect(void)
{
    m_connections.clear();
}

std::string ParameterTraits::name() const
{
    return m_name;
}

void ParameterTraits::setName(const std::string& name)
{
    m_name = name;
}

std::string ParameterTraits::description() const
{
    return m_description;
}

void ParameterTraits::setDescription(const std::string& description)
{
    m_description = description;
}

void ParameterTraits::sync(void) {
    if(m_syncing)
        return;

    m_syncing = true;
    for(auto& c : m_connections) {
        c.second(this->valueAsAny());
    }
    m_syncing = false;
}

uint64_t ParameterTraits::connectTo(ParameterTraits* target) {
    if (this->type() != target->type()) {
        return UINT64_MAX;
    }
    if (m_connectedParameters.find(target) != m_connectedParameters.end()) {
        return m_connectedParameters[target];
    }

    // Connect the lambda to the target's connections
    uint64_t connectionId = this->connect([target](std::any value) {
        target->setValue(value);
    });
    m_connectedParameters[target] = connectionId;

    target->connectTo(this);
    return connectionId;
}

void ParameterTraits::disconnectFrom(ParameterTraits* target) {
    if (m_connectedParameters.find(target) == m_connectedParameters.end()) {
        return;
    }
    disconnect(m_connectedParameters[target]);
    m_connectedParameters.erase(target);
    target->disconnectFrom(this);
}

void ParameterTraits::disconnectAllConnections() {
    // loop through m_connectedParametrs map, without using the iterator as we delete elements of the map progressively
    std::vector<ParameterTraits*> keys;
    keys.reserve(m_connectedParameters.size());
    for (const auto& pair : m_connectedParameters) {
        keys.push_back(pair.first);
    }

    for (ParameterTraits* param : keys) {
        disconnectFrom(param);
    }
}

std::string ParameterTraits::readField(const std::string& expectedKey, std::istream& is) {
    std::string line;    
    if (!std::getline(is, line))
        throw std::runtime_error("Unexpected end of input while reading " + expectedKey);

    std::size_t sep = line.find(": ");
    if (sep == std::string::npos)
        throw std::runtime_error("Malformed line: " + line);

    std::string key = line.substr(0, sep);
    std::string value = line.substr(sep + 2);

    if (key != expectedKey)
        throw std::runtime_error("Expected key '" + expectedKey + "', got '" + key + "'");

    return value;
}

std::string ParameterTraits::writeField(const std::string& key, const std::string& value) {
    return key + ": " + value + "\n";
}

} // namespace Parameters
} // namespace OpenViBE
