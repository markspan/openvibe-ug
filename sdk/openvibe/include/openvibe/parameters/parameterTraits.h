/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#pragma once

#include <any>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <set>

#include <nlohmann/json.hpp>

#include "../ov_defines.h"

namespace OpenViBE {
namespace Parameters {

class OV_API ParameterTraits {
/**
 * @brief The ParameterTraits class represents the base class for parameter traits.
 * 
 * This class provides common functionality and properties for defining parameter traits.
 * Parameter traits define the characteristics and behavior of parameters used in OpenViBE applications.
 */
public:
    /**
     * @brief Default constructor for ParameterTraits.
     */
    ParameterTraits();

    /**
     * @brief Constructor for ParameterTraits.
     * @param name The name of the parameter.
     * @param description The description of the parameter.
     */
    ParameterTraits(const std::string& name, const std::string& description);

    /**
     * @brief Copy constructor for ParameterTraits.
     * @param param The ParameterTraits object to be copied.
     */
    ParameterTraits(const ParameterTraits& param);

    /**
     * @brief Destructor for ParameterTraits.
     */
    virtual ~ParameterTraits();

    /**
     * @brief Clone the parameter traits
     * Allows to create a deep copy of a derived class while manipulating a ParameterTraits base object
     * @return A deep copy of the parameter
     */
    virtual ParameterTraits* clone() const = 0;

    /**
     * @brief Get the name of the parameter.
     * @return The name of the parameter.
     */
    std::string name() const;

    /**
     * @brief Set the name of the parameter.
     * @param name The name of the parameter.
     */
    void setName(const std::string& name);

    /**
     * @brief Get the description of the parameter.
     * @return The description of the parameter.
     */
    std::string description() const;

    /**
     * @brief Set the description of the parameter.
     * @param description The description of the parameter.
     */
    void setDescription(const std::string& description);

    /**
     * @brief Set the value of the parameter.
     * @param value The value to be set.
     * @param notify Flag indicating whether to notify connected slots about the value change.
     */
    virtual void setValue(const std::any& value, bool notify = true) = 0;

    /**
     * @brief Get the value of the parameter as std::any.
     * @return The value of the parameter as std::any.
     */
    virtual std::any valueAsAny() const = 0;

    /**
     * @brief Get the type of the parameter.
     * @return The type of the parameter.
     */
    virtual std::string type() const = 0;

    /**
     * @brief Get the value of the parameter as a string.
     * @return The value of the parameter as a string.
     */
    virtual std::string toString() const = 0;

    /**
     * @brief Set the value of the parameter from a string.
     * @param value The string value to be set.
     */
    virtual void fromString(const std::string& value) = 0;

    /**
     * @brief Serialize the parameter to an output stream.
     * @param os The output stream to serialize to.
     */
    virtual void serialize(std::ostream& os) const = 0;

    /**
     * @brief Serialize the parameter to a json object.
     * @return The json object representing the parameter.
     */
    virtual nlohmann::json serialize() const = 0;

    /**
     * @brief Deserialize the parameter from an input stream.
     * @param is The input stream to deserialize from.
     */
    virtual void deserialize(std::istream& is) = 0;

    /**
     * @brief Deserialize the parameter from a json object.
     * @param j The json object to deserialize from.
     */
    virtual void deserialize(const nlohmann::json& j) = 0;


public:
    /**
     * @brief Synchronize the parameter.
     * 
     * This function synchronizes the parameter with its current value.
     */
    void sync(void);

    /**
     * @brief Connect a slot to the parameter.
     * @param slot The slot function to be connected.
     * @return The ID of the connection.
     */
    uint64_t connect(std::function<void(std::any)> slot);
    
    /**
     * @brief Disconnect a slot from the parameter.
     * @param id The ID of the connection to be disconnected.
     */
    void disconnect(uint64_t id);

    /**
     * @brief Disconnect all slots from the parameter.
     */
    void disconnect(void);

    /**
     * @brief Connects this parameter to another parameter.
     * @param target The target parameter to connect to.
     */
    uint64_t connectTo(ParameterTraits* target);

    /**
     * @brief Disconnects from a specific parameter.
     * @param target The target parameter to disconnect from.
     */
    void disconnectFrom(ParameterTraits* target);

    /**
     * @brief Disconnects from all connected parameters.
     */
    void disconnectAllConnections();

    /**
     * @brief Register a callback invoked just before this parameter is destroyed.
     * @return An ID for use with cancelOnDestroy().
     */
    uint64_t onDestroy(std::function<void()> callback);

    /**
     * @brief Unregister a destruction callback by ID.
     */
    void cancelOnDestroy(uint64_t id);

protected:
    std::string m_name; /**< The name of the parameter. */
    std::string m_description; /**< The description of the parameter. */
    std::map<uint64_t, std::function<void(std::any)>> m_connections; /**< The map of connected slots mapped by ID. */
    uint64_t m_nextConnectionId = 0; /**< Variable incremented monotonically in order to provide unique IDs for connections */
    bool m_syncing = false;
    std::map<ParameterTraits*, uint64_t> m_connectedParameters; /** map connected parameter and connection id */
    std::map<uint64_t, std::function<void()>> m_onDestroyCallbacks;
    uint64_t m_nextDestroyId = 0;

    static std::string readField(const std::string&, std::istream& is);
    static std::string writeField(const std::string&, const std::string&);

    friend std::ostream& operator<<(std::ostream& os, const ParameterTraits& param);
    friend std::istream& operator>>(std::istream& is, ParameterTraits& param);
    friend class ParameterFactory;
};

/**
 * @brief Template base class for parameters with move semantics enforcement.
 *
 * This class extends ParameterTraits and uses CRTP to ensure that any
 * derived class implements move constructor and move assignment.
 *
 * @tparam T The underlying data type of the parameter.
 * @tparam Derived The actual class being implemented (CRTP).
 */
template <typename T, typename Derived>
class ParameterBase: public ParameterTraits
{
public:
    /**
     * @brief Default constructor.
     * Performs static checks for move semantics on the Derived class.
     */
    ParameterBase() : ParameterTraits() {
        static_assert(std::is_move_constructible_v<Derived>, "Derived class must implement a move constructor.");
        static_assert(std::is_move_assignable_v<Derived>, "Derived class must implement a move assignment operator.");
    }

    /**
     * @brief Constructor with name and description.
     * Performs static checks for move semantics on the Derived class.
     *
     * @param name The name of the parameter.
     * @param description The description of the parameter.
     */
    ParameterBase(const std::string& name, const std::string& description) : ParameterTraits(name, description) {
        static_assert(std::is_move_constructible_v<Derived>, "Derived class must implement a move constructor.");
        static_assert(std::is_move_assignable_v<Derived>, "Derived class must implement a move assignment operator.");
    }

    /**
     * @brief Constructor with name, description, and initial value.
     * Performs static checks for move semantics on the Derived class.
     *
     * @param name The name of the parameter.
     * @param description The description of the parameter.
     * @param value The initial value of the parameter.
     */
    ParameterBase(const std::string& name, const std::string& description, T value) : ParameterTraits(name, description) {
        static_assert(std::is_move_constructible_v<Derived>, "Derived class must implement a move constructor.");
        static_assert(std::is_move_assignable_v<Derived>, "Derived class must implement a move assignment operator.");
        setValue(value);
    }

    /**
     * @brief Copy constructor for ParameterBase.
     * @param other The object to copy from.
     */
    ParameterBase(const ParameterBase& other) : ParameterTraits(other), m_value(other.m_value) { }

    /**
     * @brief Move constructor for ParameterBase.
     * @param other The object to move from.
     */
    ParameterBase(ParameterBase&& other) noexcept : ParameterTraits(std::move(other))
    {
        m_value = std::move(other.m_value);
    }

    /**
     * @brief Move assignment operator.
     * Follows the SESE principle with a single return statement.
     *
     * @param other The object to move from.
     * @return Reference to this object.
     */
    ParameterBase& operator=(ParameterBase&& other) noexcept
    {
        if (this != &other)
        {
            // Moving members from ParameterTraits
            m_name = std::move(other.m_name);
            m_description = std::move(other.m_description);
            m_connections = std::move(other.m_connections);
            m_connectedParameters = std::move(other.m_connectedParameters);

            // Moving own members
            m_value = std::move(other.m_value);
        }

        return *this;
    }

    /** 
     * @brief Assignment operator from a value.
     * Allows setting the parameter's value using a value of the same type.
     * @param value The value to set.
     * @return Reference to this object.
     */
    template<typename U = T, typename = std::enable_if_t<!std::is_convertible_v<U, std::string>>>
    ParameterBase& operator=(const T& value) {
        setValue(value);
        return *this;
    }

    /**
     * @brief Copy assignment operator.
     * @param other The object to copy from.
     * @return Reference to this object.
     */
    ParameterBase& operator=(const ParameterBase& other)
    {
        if (this != &other)
        {
            m_name = other.m_name;
            m_description = other.m_description;
            m_value = other.m_value;
        }

        return *this;
    }

    /**
     * @brief Constructor with initial value.
     * Performs static checks for move semantics on the Derived class.
     *
     * @param value The initial value of the parameter.
     */
    ParameterBase(const T& value) : ParameterTraits() {
        static_assert(std::is_move_constructible_v<Derived>, "Derived class must implement a move constructor.");
        static_assert(std::is_move_assignable_v<Derived>, "Derived class must implement a move assignment operator.");
        setValue(value);
    }

    /**
     * @brief Destructor for ParameterBase.
     */
    virtual ~ParameterBase() = default;

    /**
     * @brief Get the value of the parameter.
     * @return The value of the parameter.
     */
    virtual const T& value() const {
        return m_value;
    }

    /**
     * @brief Implicit conversion operator to the underlying type T.
     * Allows using the parameter object directly as its value type.
     *
     * @return The value of the parameter.
     */
    operator T() const {
        return m_value;
    }

    /**
     * @brief Implicit conversion operator to std::string.
     * Enabled only if T is not implicitly convertible to std::string to avoid ambiguity.
     *
     * @return The string representation of the parameter's value.
     */
    template<typename U = T, typename = std::enable_if_t<!std::is_convertible_v<U, std::string>>>
    operator std::string() const {
        return toString();
    }

    /**
     * @brief Assignment operator from a string.
     * Allows setting the parameter's value using a string representation.
     *
     * @param str The string representation of the value to set.
     * @return Reference to this object.
     */
    ParameterBase<T, Derived>& operator=(const std::string& str) {
        fromString(str);
        return *this;
    }
    
    /**
    * @brief Set the value of the parameter from a std::any.
    * @param value The value to be set as std::any.
    * @param notify Flag indicating whether to notify connected slots about the value change.
    * @override from ParameterTraits
    * 
    */ 
    void setValue(const std::any& value, bool notify=true) override {
        if (value.type() == typeid(T)) {
            setValue(std::any_cast<T>(value), notify);
        } else if (value.type() == typeid(const char*)) {
            this->fromString(std::any_cast<const char*>(value));
        } else if (value.type() == typeid(std::string)){
            this->fromString(std::any_cast<std::string>(value));
        } else {
            std::cout << "ParameterBase::setValue: cannot convert value "
                      << value.type().name() << " to type " << this->type() << std::endl;
        }
    }

    /**
     * @brief Get the value of the parameter as std::any.
     * @return The value of the parameter as std::any.
     * @override from ParameterTraits
     */
    std::any valueAsAny() const override {
        return std::any(m_value);
    }

    virtual void setValue(const T& value, bool notify=true) {
        m_value = value;
        if (notify)
            sync();
    }

    virtual void serialize(std::ostream& os) const override {
        os << writeField("type", type());
        os << writeField("name", m_name);
        os << writeField("description", m_description);
        os << writeField("value", toString());
    }

    /**
     * @brief Serialize the parameter to a json object.
     * @return The json object representing the parameter.
     * @override from ParameterTraits
     * @info The JSON object will contain the fields "type", "name", "description", and "value". The "type" field will contain the type of the parameter as returned by the type() method. The "name" field will contain the name of the parameter. The "description" field will contain the description of the parameter. The "value" field will contain the string representation of the parameter's value as returned by the toString() method.
     * @details The JSON object will have the following structure:
     * {
     *   "type": "<type>",
     *   "name": "<name>",
     *   "description": "<description>",
     *   "value": "<value>"
     * }
     * @throws std::runtime_error if the parameter cannot be serialized to a string.
     */
    virtual nlohmann::json serialize() const override
    {
        nlohmann::json j;

        // Here j is not const, so [] is allowed.
        j["type"] = type();
        j["name"] = m_name;
        j["description"] = m_description;
        j["value"] = m_value; // Direct assignment, no T() cast needed.

        return j;
    }

    virtual void deserialize(std::istream& is) override {
        std::string typeStr = readField("type", is);
        if (typeStr != type()) {
            throw std::runtime_error("Type mismatch in deserialize: expected " + type() + ", got " + typeStr);
        }
        
        m_name = readField("name", is);
        m_description = readField("description", is);
        fromString(readField("value", is));
    }

    /**
     * @brief Deserialize the parameter from a json object.
     * @param j The json object containing the parameter data.
     * @override from ParameterTraits
     * @throws std::runtime_error if the "type" field is missing or does not match the expected type, or if the "value" field is missing or of an incorrect type.
     * @info The JSON object is expected to contain the fields "type", "name", "description", and "value". The "type" field must match the type returned by the type() method of the parameter. The "value" field must be a string that can be parsed by the fromString() method of the parameter.
     * @details The JSON object is expected to have the following structure: 
     * {
     *   "type": "<type>",
     *   "name": "<name>",
     *   "description": "<description>",
     *   "value": "<value>"
     * }
     */
    virtual void deserialize(const nlohmann::json& j) override
    {
        if (j.at("type").get<std::string>() != type())
        {
            throw std::runtime_error("Type mismatch in deserialize: expected " + type() + ", got " + j.at("type").get<std::string>());
        }
        else
        {
            m_name = j.at("name").get<std::string>();
            m_description = j.at("description").get<std::string>();
            m_value = j.at("value").template get<T>();
        }
    }


protected:
    T m_value;
};

using Parameters = std::vector<ParameterTraits*>;

inline std::ostream& operator<<(std::ostream& os, const OpenViBE::Parameters::ParameterTraits& param)
{
    os << param.toString();
    return os;
}

inline std::istream& operator>>(std::istream& is, OpenViBE::Parameters::ParameterTraits& param)
{
    std::string str;
    is >> str;
    param.fromString(str);
    return is;
}
} // namespace Parameters
} // namespace OpenViBE
