#pragma once

#include "IReader.h"

namespace EBML {
/**
 * \class IReaderHelper
 * \author Yann Renard (INRIA/IRISA)
 * \date 2006-08-07
 * \brief Helper to read basic EBML types
 *
 * This class may be used by a IReaderCallback instance
 * in order to correctly parse simple types defined in the
 * EBML description such as integers, floats, strings etc...
 *
 * A similar class exists to help formating process...
 * See EBML::IWriterHelper for more details.
 *
 * Be sure to look at http://ebml.sourceforge.net/specs/ in
 * order to understand what EBML is and how it should be used.
 *
 * \todo long double reading implementation
 * \todo date reading implementation
 * \todo utf8 string reading implementation
 */
class EBML_API IReaderHelper
{
public:

    /**
     * \name Standard EBML value reading
     * \param buffer [in] : The buffer containing data
     * \param size [in] : The buffer size in bytes
     * \return The value contained in the buffer
     *
     * Be sure to look at http://ebml.sourceforge.net/specs/ in
     * order to understand standard EBML types.
     */
    //@{
    /**
     * \brief Gets an unsigned integer from the given buffer
     */
    virtual uint64_t getUInt(const void* buffer, const size_t size) = 0;
    /**
     * \brief Gets a signed integer from the given buffer
     */
    virtual int64_t getInt(const void* buffer, const size_t size) = 0;
    /**
     * \brief Gets a float from the given buffer
     */
    virtual double getDouble(const void* buffer, const size_t size) = 0;
    // virtual ??? getFloat80FromChildData(const void* buffer, const size_t size)=0;
    // virtual ??? getDateFromChildData(const void* buffer, const size_t size)=0;
    /**
     * \brief Gets an ASCII string from the given buffer
     * \warning The returned value is not permanent. It should be
     *          copied immediatly somewhere else by the caller.
     */
    virtual const char* getStr(const void* buffer, const size_t size) = 0;
    // virtual ??? getUTF8StringFromChildData(const void* buffer, const size_t size)=0;
    //@}

    /**
     * \brief Tells this object it won't be used anymore
     *
     * Instances of this class can not be instanciated
     * another way than calling \c createReaderHelper. They
     * can not be deleted either because the destructor is.
     * protected. The library knows how to create and
     * delete an instance of this class... Calling
     * \c release will simply delete this instance and
     * handle necessary cleanings when needed.
     *
     * The current object is invalid after calling this
     * function. It can not be used anymore.
     */
    virtual void release() = 0;

protected:

    /**
     * \brief Virtual destructor - should be overloaded
     */
    virtual ~IReaderHelper() { }
};

/**
 * \brief Instanciation function for EBML reader helper objects
 * \return a pointer to the created instance on success.
 * \return \c NULL when something went wrong.
 */
extern EBML_API IReaderHelper* createReaderHelper();
}  // namespace EBML
