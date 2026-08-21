#pragma once

/**
 * @file DriverBuilder.h
 * @brief Header file for the DriverBuilder class.
 *
 * This file defines the `DriverBuilder` class, which is a template class for
 * creating driver builders. It inherits from `AbstractDriverBuilder` and
 * implements methods to create and manage driver instances.
 */

#include <AbstractDriver.h> ///< Include the header file for the AbstractDriver class.

#include <string>   ///< Forward declaration of the std::string class.
#include <typeinfo> ///< for typeid

#ifdef __cplusplus
#  define EXTERN_C extern "C"
#else
#  define EXTERN_C
#endif

#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__
  #ifdef __GNUC__
    #define PLUGIN_PUBLIC __attribute__ ((dllexport))
  #else
    #define PLUGIN_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
  #endif  
  #define PLUGIN_LOCAL
#else
  #if __GNUC__ >= 4
    #define PLUGIN_PUBLIC __attribute__ ((visibility ("default")))
    #define PLUGIN_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define PLUGIN_PUBLIC
    #define PLUGIN_LOCAL
  #endif
#endif


/**
 * @brief Abstract base class for driver builders.
 *
 * This class serves as a base for creating different types of driver builders.
 * It defines the interface for creating and destroying driver instances, as well
 * as getting the writer name and type hash.
 */
class AbstractDriverBuilder
{
public:
    AbstractDriverBuilder() = default;
    virtual ~AbstractDriverBuilder() = default;

    virtual AbstractDriver* createDriver() const = 0; ///< Pure virtual function to create a driver instance.
    virtual void destroyDriver(AbstractDriver* driver) const  { delete driver; }; ///< Virtual function to destroy a driver instance.

    virtual constexpr std::string getDriverName() const = 0; ///< Pure virtual function to get the driver name.
    virtual constexpr size_t getDriverTypeHash() const = 0; ///< Pure virtual function to get the driver type hash.
    virtual constexpr bool isDriverAux() const = 0; ///< Pure virtual function to get the driver fammily match (standard (false) auxiliary (true)).
    virtual AbstractDriver* create() const { return createDriver(); }
    virtual std::string getName()    const { return getDriverName(); }
    virtual bool auxiliary()         const { return isDriverAux(); }
};


#ifdef BUILTIN_PLUGIN

#include <AcquisitionManager.h> ///< Include the header file for the AcquisitionManager class.

/**
 * @brief Template class for creating driver builders.
 *
 * This class is a template that allows for the creation of driver builders
 * for different types of drivers. It inherits from AbstractDriverBuilder and
 * implements the necessary methods to create and manage driver instances.
 *
 * @tparam Driver The type of driver to be created.
 * @tparam name The name of the driver as a string literal.
 * @tparam aux The auxiliary flag indicating whether the driver is auxiliary (true) or standard (false).
 */
template<typename Driver, const char* DriverName, bool Aux>
class DriverBuilder : public AbstractDriverBuilder
{
public:
    DriverBuilder() { AcquisitionManager::instance().registerDriver(this); } ///< Constructor that registers the driver builder with the AcquisitionManager instance.
    virtual ~DriverBuilder() = default;

    AbstractDriver* createDriver() const override final { Driver* ptr=  new Driver(); ptr->setAuxiliaryStatus(Aux); return ptr; } ///< Function to create a driver instance of type Driver.

    constexpr std::string getDriverName() const override final { return DriverName; } ///< Function to get the driver name.    }
    constexpr size_t  getDriverTypeHash() const override final { return typeid(Driver).hash_code(); } ///< Function to get the driver type hash using RTTI.
    constexpr bool isDriverAux()          const override final { return Aux; } ///< Function to check if the driver is auxiliary.
};

/**
* @brief Macro to register a driver builder.
* 
* This macro simplifies the registration of a driver builder by creating a static instance of the `DriverBuilder` class.
* It takes the driver class name, driver name, and auxiliary flag as parameters.
* 
* @param Driver The type of driver to be registered.
* @param Name The name of the driver as a string literal.
* @param Aux The auxiliary flag indicating whether the driver is auxiliary (true) or standard (false).
*/
#define REGISTER_DRIVER(Driver, Name, Auxiliary) \
constexpr const static char Driver##DriverInstance[] = Name; \
const static DriverBuilder<Driver, Driver##DriverInstance, Auxiliary> Driver##BuilderInstance;

#else

/**
 * @brief Template class for creating driver builders, plugin flavour.
 *
 * Same contract as the BUILTIN_PLUGIN flavour above, minus the self-registration:
 * a plugin cannot reach the host's AcquisitionManager singleton, so the builder is
 * handed over through the exported getInstance() factory instead and the host
 * registers it itself (see CInitApp::loadOnePlugin).
 *
 * @tparam Driver The type of driver to be created.
 * @tparam DriverName The name of the driver as a string literal.
 * @tparam Aux The auxiliary flag indicating whether the driver is auxiliary (true) or standard (false).
 */
template<typename Driver, const char* DriverName, bool Aux>
class DriverBuilder : public AbstractDriverBuilder
{
public:
    DriverBuilder() = default; ///< Constructor. Unlike the builtin flavour, it does not self-register.
    virtual ~DriverBuilder() = default;

    AbstractDriver* createDriver() const override final { Driver* ptr = new Driver(); ptr->setAuxiliaryStatus(Aux); return ptr; } ///< Function to create a driver instance of type Driver.

    constexpr std::string getDriverName() const override final { return DriverName; } ///< Function to get the driver name.    }
    constexpr size_t  getDriverTypeHash() const override final { return typeid(Driver).hash_code(); } ///< Function to get the driver type hash using RTTI.
    constexpr bool isDriverAux()          const override final { return Aux; } ///< Function to check if the driver is auxiliary.
};

/**
* @brief Macro to register a driver builder.
* 
* This macro is used to register a driver builder for plugin builds. It exports the
* getInstance() factory that the host looks up with getProcAddr() once it has loaded
* the shared library.
*
* @note The exported symbol name is fixed, so a plugin carries exactly **one** driver.
*       Invoke this macro from a single translation unit -- the driver's .cpp, never its
*       header -- otherwise the factory ends up multiply defined.
*
* @param Driver The type of driver to be registered.
* @param Name The name of the driver as a string literal.
* @param Auxiliary The auxiliary flag indicating whether the driver is auxiliary (true) or standard (false).
*/
#define REGISTER_DRIVER(Driver, Name, Auxiliary) \
constexpr const static char Driver##DriverInstance[] = Name; \
EXTERN_C PLUGIN_PUBLIC AbstractDriverBuilder * getInstance() \
{ \
    static DriverBuilder<Driver, Driver##DriverInstance, Auxiliary> instance; \
    return & instance; \
}

#endif // BUILTIN_PLUGIN
