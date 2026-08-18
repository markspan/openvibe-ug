#pragma once

#include <ovas_base.h>

#include <map>
#include <iostream>

#include "ovasCSettingsHelperOperators.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
  * \class Property
  * \author Jussi T. Lindgren (Inria)
  * \date 2013-11
  * \brief Base class for properties. A property is essentially a <name,value> pair. 
  *
  * \note This class is intended for typed inheritance. It carries no data.
  */
class Property
{
public:
    explicit Property(const CString& name) : m_name(name) { }

    virtual ~Property() {}

    CString getName() const { return m_name; }

    virtual std::ostream& toStream(std::ostream& out) const = 0;
    virtual std::istream& fromStream(std::istream& in) = 0;

private:
    CString m_name;
};

// Helper operators to map base class << and >> to those of the derived classes. These can not be members.
inline std::ostream& operator<<(std::ostream& out, const Property& obj) { return obj.toStream(out); }
inline std::istream& operator>>(std::istream& in, Property& obj) { return obj.fromStream(in); }

/*
 * \class TypedProperty
 * \author Jussi T. Lindgren (Inria)
 * \date 2013-11
 * \brief A property with a typed data pointer.
 *
 * \note The class does NOT own the data pointer, but may allow modification of its contents via replaceData().
 */
template <typename T>
class TypedProperty final : public Property
{
public:
    TypedProperty(const CString& name, T* data) : Property(name), m_data(data) { }

    // Access data
    const T* getData() const { return m_data; }

    // Overwrites existing data with new contents.
    void replaceData(T& data) { *m_data = data; }

    std::ostream& toStream(std::ostream& out) const override
    {
        out << *m_data;
        return out;
    }

    std::istream& fromStream(std::istream& in) override
    {
        in >> *m_data;
        return in;
    }

private:
    T* m_data = nullptr;
};

#ifdef OV_SH_SUPPORT_GETTERS
    /*
     * \class GetterSetterProperty
     * \author Jussi T. Lindgren (Inria)
     * \date 2013-11
     * \brief A property for situations where the data can only be accessed by getters and setters
     *
     * \note Type T is the type of the object that has the getters and setters as a member. The
     * types V and W indicate the actual data type that the getters and setters deal with. We
     * use two types V and W as in the case of the setter, the type is usually const, but for getter, 
     * its not. This avoids the compiler getting confused.
     */
    template <typename T, typename V, typename W>
    class GetterSetterProperty : public Property
    {
    public:
        GetterSetterProperty(const CString& name, T& obj, V ( T::*getter)() const, bool ( T::*setter)(W))
            : Property(name), m_obj(obj), m_getterFunction(getter), m_setterFunction(setter) { };

        virtual std::ostream& toStream(std::ostream& out) const
        { 
            // std::cout << "Writing " << (m_obj.*m_getter)() << "\n"; 
            out << (m_obj.*m_getterFunction)();
            return out;
        }

        virtual std::istream& fromStream(std::istream& in)
        {
            W tmp;
            in >> tmp; 
            // std::cout << "Reading " << tmp << "\n";
            (m_obj.*m_setterFunction)(tmp);
            return in;
        }

    private:

        T& m_obj;
        V ( T::*m_getterFunction )() const;
        bool ( T::*m_setterFunction )(W);
    };
#endif

/*
 * \class SettingsHelper
 * \author Jussi T. Lindgren (Inria)
 * \date 2013-11
 * \brief Convenience helper class that eases the saving and loading of variables (properties) to/from the configuration manager
 *
 * \note For registering exotic types, the user must provide the << and >> overloads to/from streams
 */
class SettingsHelper
{
public:
    SettingsHelper(const char* prefix, Kernel::IConfigurationManager& rMgr) : m_prefix(prefix), m_configManager(rMgr) { }

    ~SettingsHelper()
    {
        for (auto it = m_properties.begin(); it != m_properties.end(); ++it) { delete(it->second); }
        m_properties.clear();
    }

    // Register or replace a variable. The variable must remain valid for the lifetime of the SettingsHelper object.
    template <typename T>
    bool add(const CString& name, T* var)
    {
        if (!var) { return false; } // std::cout << "Tried to add a NULL pointer\n";

        // If key is in map, replace
        auto it = m_properties.find(name);
        if (it != m_properties.end()) {
            delete it->second;
        }   // m_rContext.getLogManager() << Kernel::LogLevel_Trace << "Replacing key [" << name << "]\n";

        TypedProperty<T>* myProperty        = new TypedProperty<T>(name, var);
        m_properties[myProperty->getName()] = myProperty;

        return true;
    }

#ifdef OV_SH_SUPPORT_GETTERS
    // Register or replace a property used via setters and getters. The actual object must be provided as well and
        // must remain valid for the lifetime of the SettingsHelper object.
        template <typename T, typename V, typename W>
        bool add(const CString& name, T& obj, V ( T::*getter)() const, bool ( T::*setter)(W))
        {
            if (getter == nullptr || setter == nullptr) { return false; }   // std::cout << "Tried to add a NULL pointer\n";

            // If key is in map, replace
            auto it = m_properties.find(name);
            if (it != m_properties.end()) { delete it->second; }    // m_rContext.getLogManager() << Kernel::LogLevel_Trace << "Replacing key [" << name << "]\n";

            GetterSetterProperty<T, V, W>* myProperty = new GetterSetterProperty<T, V, W>(name, obj, getter, setter);
            m_properties[myProperty->getName()]      = myProperty;

            return true;
        }
#endif

    // Save all registered variables to the configuration manager
    void save();

    // Load all registered variables from the configuration manager
    void load();

    // Get access to the registered variables
    const std::map<CString, Property*>& getAllProperties() const { return m_properties; }

private:
    CString m_prefix;
    Kernel::IConfigurationManager& m_configManager;

    std::map<CString, Property*> m_properties;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
