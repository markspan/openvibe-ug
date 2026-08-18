#pragma once

/**
 * @file AbstractWriterBuilder.h
 * @brief Header file for the AbstractWriterBuilder class.
 *
 * This file defines the `AbstractWriterBuilder` class, which is a template class for
 * creating writer builders. It inherits from `AbstractWriterBuilder` and
 * implements methods to create and manage writer instances.
 */

#include <AbstractWriter.h> ///< Include the header file for the AbstractDriver class.

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
 * @brief Abstract base class for writer builders.
 *
 * This class serves as a base for creating different types of writer builders.
 * It defines the interface for creating and destroying writer instances, as well
 * as getting the writer name and type hash.
 */
class AbstractWriterBuilder
{
public:
    AbstractWriterBuilder() = default;
    virtual ~AbstractWriterBuilder() = default;

    virtual AbstractWriter* createWriter() const = 0; ///< Pure virtual function to create a writer instance.
    virtual void destroyWriter(AbstractWriter* writer) const { delete writer; }; ///< Virtual function to destroy a writer instance.

    virtual constexpr std::string getWriterName() const = 0; ///< Pure virtual function to get the writer name.
    virtual constexpr size_t getWriterTypeHash() const = 0; ///< Pure virtual function to get the writer type hash.
    virtual AbstractWriter* create() const { return createWriter(); }
    virtual std::string getName() const { return getWriterName(); }


};


#ifdef BUILTIN_PLUGIN

#include <AcquisitionManager.h> ///< Include the header file for the AcquisitionManager class.

/**
 * @brief Template class for creating writer builders.
 *
 * This class is a template that allows for the creation of writer builders
 * for different types of writers. It inherits from AbstractWriterBuilder and
 * implements the necessary methods to create and manage writer instances.
 *
 * @tparam Writer The type of writer to be created.
 * @tparam name The name of the writer as a string literal.
 * 
 */
template<typename Writer, const char* WriterName>
class WriterBuilder : public AbstractWriterBuilder
{
public:
    WriterBuilder() { AcquisitionManager::instance().registerWriter(this); } ///< Constructor that registers the Writer builder with the AcquisitionManager instance.
    virtual ~WriterBuilder() = default;

    AbstractWriter* createWriter() const override final { return new Writer(WriterName); } ///< Function to create a Writer instance of type Writer.

    constexpr std::string getWriterName() const override final { return WriterName; } ///< Function to get the Writer name.    }
    constexpr size_t  getWriterTypeHash() const override final { return typeid(Writer).hash_code(); } ///< Function to get the Writer type hash using RTTI.

};

/**
 * @brief Macro to register a writer builder.
 *
 * This macro simplifies the registration of a writer builder by creating a static instance of the WriterBuilder class.
 * It takes the writer type and name as parameters.
 *
 * @param WriterType The type of the writer to be registered.
 * @param WriterName The name of the writer as a string literal.
 * 
 */
#define REGISTER_WRITER(Writer, Name) \
constexpr const static char Writer##WriterInstance[] = Name; \
const static WriterBuilder<Writer, Writer##WriterInstance> Writer##BuilderInstance;

#else

// PLUGIN_PUBLIC 

/**
 * @brief Macro to register a writer builder for plugin usage.
 *
 * This macro is used when the writer builder is part of a plugin. It creates a static instance
 * of the WriterBuilder class and ensures that it is exported for use in other modules.
 *
 * @param WriterType The type of the writer to be registered.
 * @param WriterName The name of the writer as a string literal.
 * 
 */
template<typename Writer, const char* WriterName>
class WriterBuilder : public AbstractWriterBuilder
{
public:
    WriterBuilder() { } ///< Constructor that registers the Writer builder with the AquisitionManager instance.
    virtual ~WriterBuilder() = default;

    AbstractWriter* createWriter() const override final { return new Writer(WriterName); } ///< Function to create a Writer instance of type Writer.

    constexpr std::string getWriterName() const override final { return WriterName; } ///< Function to get the Writer name.    }
    constexpr size_t  getWriterTypeHash() const override final { return typeid(Writer).hash_code(); } ///< Function to get the Writer type hash using RTTI.

};

/**
 * @brief Macro to register a writer builder.
 *
 * This macro simplifies the registration of a writer builder by creating a static instance
 * of the WriterBuilder class. It takes the writer type and name as parameters.
 *
 * @param WriterType The type of the writer to be registered.
 * @param WriterName The name of the writer as a string literal.
 */
#define REGISTER_WRITER(Writer, Name) \
constexpr const static char Writer##WriterInstance[] = Name; \
EXTERN_C PLUGIN_PUBLIC AbstractWriterBuilder * getInstance() \
{ \
    static WriterBuilder<Writer, Writer##WriterInstance> instance; \
    return & instance; \
}

#endif // BUILTIN_PLUGIN
