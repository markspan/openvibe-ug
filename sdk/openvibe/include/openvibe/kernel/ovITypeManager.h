#pragma once

#include "ovIKernelObject.h"
#include <vector>

namespace OpenViBE {
namespace Kernel {
/**
 * \class ITypeManager
 * \brief Type manager, provides information on platform's handled types, parameters, streams etc...
 * \author Yann Renard (INRIA/IRISA)
 * \date 2007-03-14
 *
 * This manager concentrates all information about types in the OpenViBE
 * platform. Any non-standard type should be declared in the type identifier
 * by the kernel or the plugins in order to be used. Registered types range
 * from box settings to streams, eventually including inter-type auto casts.
 *
 * See the different functions for more details.
 */
class OV_API ITypeManager : public IKernelObject
{
public:

    /** \name Type registration */
    //@{

    /**
     * \brief Gets next type identifier
     * \param previousID [in] : The identifier
     *        for the preceeding type
     * \return The identifier of the next type in case of success.
     * \return \c CIdentifier::undefined() on error.
     * \note Giving \c CIdentifier::undefined() as \c previousID
     *       will cause this function to return the first type
     *       identifier.
     */
    virtual CIdentifier getNextTypeIdentifier(const CIdentifier& previousID) const = 0;

    /**
     * \brief Get a sorted vector of pairs of registered types identifiers associated to types names, 
     * sorted by type name in increasing alphabetical order.
     * \return the sorted vector
     */
    virtual std::vector<std::pair<CIdentifier, CString>> getSortedTypes() const = 0;

    /**
     * \brief Registers a new simple type of data
     * \param typeID [in] : the identifier for this type
     * \param name [in] : the name for this type
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool registerType(const CIdentifier& typeID, const CString& name) = 0;
    /**
     * \brief Registers a new communication stream type
     * \param typeID [in] : the identifier for this type
     * \param name [in] : the name for this type
     * \param parentTypeID [in] : the parent stream type identifier
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * Stream types are organized as a hierarchy, meaning that any
     * derived stream should at least contain the parent stream content,
     * plus other informations. This will be used by applications in order
     * to determine what stream is compatible with what other stream and
     * to allow box connections or not.
     */
    virtual bool registerStreamType(const CIdentifier& typeID, const CString& name, const CIdentifier& parentTypeID) = 0;
    /**
     * \brief Registers a new enumeration type
     * \param typeID [in] : the type identifier for this type
     * \param name [in] : the name for this type
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \sa registerEnumerationEntry
     *
     * An enumeration should have several possible values.
     * This values have to be created thanks to \c registerEnumerationEntry
     */
    virtual bool registerEnumerationType(const CIdentifier& typeID, const CString& name) = 0;
    /**
     * \brief Registers a new enumeration value for a given enumeration type
     * \param typeID [in] : the type identifier of the enumeration which new entry has to be registered
     * \param name [in] : the name of the entry to register
     * \param value [in] : the value of the entry to register
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \note the enumeration has to be registered
     * \sa registerEnumerationType
     */
    virtual bool registerEnumerationEntry(const CIdentifier& typeID, const CString& name, uint64_t value) = 0;

    /**
     * \brief Registers a new bitmask type
     * \param typeID [in] : the type identifier for this type
     * \param name [in] : the name for this type
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \sa registerBitMaskEntry
     *
     * A bitmask should have several possible values.
     * This values have to be created thanks to \c registerBitMaskEntry
     */
    virtual bool registerBitMaskType(const CIdentifier& typeID, const CString& name) = 0;
    /**
     * \brief Registers a new bitmask value for a given bitmask type
     * \param typeID [in] : the type identifier of the bitmask which new entry has to be registered
     * \param name [in] : the name of the entry to register
     * \param value [in] : the value of the entry to register
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \note the bitmask has to be registered
     * \sa registerBitMaskType
     */
    virtual bool registerBitMaskEntry(const CIdentifier& typeID, const CString& name, uint64_t value) = 0;

    //@}
    /** \name Registration verification */
    //@{

    /**
     * \brief Tests if a specific type has been registered
     * \param typeID [in] : the type identifier which registration has to be tested
     * \return \e true if the specified type has been registered.
     * \return \e false if the specified type has not been registered.
     */
    virtual bool isRegistered(const CIdentifier& typeID) const = 0;
    /**
     * \brief Tests if a specific type has been registered and is a stream
     * \param typeID [in] : the type identifier which registration has to be tested
     * \return \e true if the specified type has been registered is a stream.
     * \return \e false if the specified type has not been registered or is not a stream.
     */
    virtual bool isStream(const CIdentifier& typeID) const = 0;
    /**
     * \brief Tests is a specific type has been registered, is a stream and is derived from another registered stream type
     * \param typeID [in] : the type identifier which registration and derivation has to be tested
     * \param parentTypeID [in] : the type identifier of the supposed parent stream
     * \return \e true in case \c typeID is registered as a stream type and derived from \c parentTypeID
     * \return \e false in case \c parentTypeID is not registered
     * \return \e false in case \c parentTypeID is not a stream
     * \return \e false in case \c typeID is not registered
     * \return \e false in case \c typeID is not a stream
     * \return \e false in case \c typeID is not derived from parentTypeID
     * \note The derivation can be indirect (\c typeID can derive an
     *       intermediate stream type which derives \c parentTypeID)
     */
    virtual bool isDerivedFromStream(const CIdentifier& typeID, const CIdentifier& parentTypeID) const = 0;
    /**
     * \brief Tests if a specific type has been registered and is an enumeration
     * \param typeID [in] : the type identifier which registration has to be tested
     * \return \e true if the specified type has been registered and is an enumeration.
     * \return \e false if the specified type has not been registered or is not an enumeration.
     */
    virtual bool isEnumeration(const CIdentifier& typeID) const = 0;
    /**
     * \brief Tests if a specific type has been registered and is a bitmask
     * \param typeID [in] : the type identifier which registration has to be tested
     * \return \e true if the specified type has been registered and is a bitmask.
     * \return \e false if the specified type has not been registered or is not a bitmask.
     */
    virtual bool isBitMask(const CIdentifier& typeID) const = 0;

    //@}
    /** \name Type identification */
    //@{

    /**
     * \brief Gets the name of a specified type
     * \param typeID [in] : the type identifier which name should be returned
     * \return the name of the speficied type.
     */
    virtual CString getTypeName(const CIdentifier& typeID) const = 0;

    /**
     * \brief Gets the type identifier from a type name
     * \param name [in] : the name of the type for which the identifier should be returned
     * \return the type identifier corresponding to the specified name.
     * \return \c CIdentifier::undefined() on error.
     */
    virtual CIdentifier getTypeFromName(const CString& name) const = 0;

    /**
     * \brief Gets the parent stream type for a stream type
     * \param typeID [in] : the stream type identifier which parent stream type be returned
     * \return the parent stream type on success.
     * \return \c CIdentifier::undefined() on error.
     * \note The specified type identifier has to be a stream type.
     */
    virtual CIdentifier getStreamParentType(const CIdentifier& typeID) const = 0;

    //@}
    /** \name Enumeration entry accessors */
    //@{

    /**
     * \brief Gets the number of enumeration entry for an enumeration type
     * \param typeID [in] : the enumeration type identifier
     * \return the number of entry for this enumeration type.
     */
    virtual size_t getEnumerationEntryCount(const CIdentifier& typeID) const = 0;
    /**
     * \brief Gets details for a specific enumeration type entry
     * \param typeID [in] : the enumeration type identifier
     * \param index [in] : the index of the entry which details should be returned
     * \param name [out] : the name of the specified entry
     * \param value [out] : the value of the speficied entry
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool getEnumerationEntry(const CIdentifier& typeID, uint64_t index, CString& name, uint64_t& value) const = 0;
    /**
     * \brief Converts an enumeration entry value to an enumeration entry name
     * \param typeID [in] : the enumeration type identifier
     * \param value [in] : the enumeration entry value
     * \return the enumeration entry name corresponding to the specified value.
     * \warning on error, an empty string is returned.
     */
    virtual CString getEnumerationEntryNameFromValue(const CIdentifier& typeID, uint64_t value) const = 0;
    /**
     * \brief Converts an enumeration entry name to an enumeration entry value
     * \param typeID [in] : the enumeration type identifier
     * \param name [in] : the enumeration entry name
     * \return the enumeration entry value corresponding to the specified name.
     * \warning on error, \c 0xffffffffffffffffLL is returned.
     */
    virtual uint64_t getEnumerationEntryValueFromName(const CIdentifier& typeID, const CString& name) const = 0;

    //@}
    /** \name Bitmask entry accessors */
    //@{

    /**
     * \brief Gets the number of bitmask entry for a bitmask type
     * \param typeID [in] : the bitmask type identifier
     * \return the number of entry for this bitmask type.
     */
    virtual size_t getBitMaskEntryCount(const CIdentifier& typeID) const = 0;
    /**
     * \brief Gets details for a specific bitmask type entry
     * \param typeID [in] : the bitmask type identifier
     * \param index [in] : the index of the entry which details should be returned
     * \param name [out] : the name of the specified entry
     * \param value [out] : the value of the speficied entry
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool getBitMaskEntry(const CIdentifier& typeID, uint64_t index, CString& name, uint64_t& value) const = 0;
    /**
     * \brief Converts a bitmask entry value to a bitmask entry name
     * \param typeID [in] : the bitmask type identifier
     * \param value [in] : the bitmask entry value
     * \return the bitmask entry name corresponding to the specified value.
     * \warning on error, an empty string is returned.
     */
    virtual CString getBitMaskEntryNameFromValue(const CIdentifier& typeID, uint64_t value) const = 0;
    /**
     * \brief Converts a bitmask entry name to a bitmask entry value
     * \param typeID [in] : the bitmask type identifier
     * \param name [in] : the bitmask entry name
     * \return the bitmask entry value corresponding to the specified name.
     * \warning on error, \c 0xffffffffffffffffLL is returned.
     */
    virtual uint64_t getBitMaskEntryValueFromName(const CIdentifier& typeID, const CString& name) const = 0;
    /**
     * \brief Computes the textual value of a composition of numerical entries
     * \param typeID [in] : the bitmask type identifier
     * \param value [in] : the composition of numerical entries
     * \return the bitmask entry composition name.
     * \warning on error, an empty string is returned.
     */
    virtual CString getBitMaskEntryCompositionNameFromValue(const CIdentifier& typeID, uint64_t value) const = 0;
    /**
     * \brief Computes the integer value of a composition of textual entries
     * \param typeID [in] : the bitmask type identifier
     * \param name [in] : the composition of textual entries
     * \return the bitmask entry composition value.
     * \warning on error, \c 0x0000000000000000LL is returned.
     */
    virtual uint64_t getBitMaskEntryCompositionValueFromName(const CIdentifier& typeID, const CString& name) const = 0;

    /**
    * \brief Evaluate the string arithmetic expression value 
    * to a numeric value as a float
    * \param value [in] : arithmetic expression to evaluate
    * \param result [out] : result of evaluation
    * \return true if the arithmetic evaluation succeeded, 
    * \return false if the arithmetic expression is incorrect
    */
    virtual bool evaluateSettingValue(CString value, double& result) const = 0;

    //@}

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_TypeManager)
};
}  // namespace Kernel
}  // namespace OpenViBE
