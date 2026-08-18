#pragma once

#include "IWriter.h"

namespace EBML {
/**
 * \class IWriterHelper
 * \author Yann Renard (INRIA/IRISA)
 * \date 2006-08-07
 * \brief Helper to write basic EBML types
 *
 * This class may be used by the user in order to correctly
 * format simple types defined in the EBML description such
 * as integers, floats, strings etc... It directly uses the
 * EBML::IWriter connected instance so one could simply
 * use the helper in order to write his EBML stream.
 *
 * A similar class exists to help parsing process...
 * See EBML::IReaderHelper for more details.
 *
 * Be sure to look at http://ebml.sourceforge.net/specs/ in
 * order to understand what EBML is and how it should be used.
 *
 * \todo long double formating implementation
 * \todo date formating implementation
 * \todo utf8 string formating implementation
 */
class EBML_API IWriterHelper
{
public:

    /** \name Writer connection */
    //@{
    /**
     * \brief Connects an EBML writer to this helper
     * \param writer [in] : The writer to connect
     * \return \e true on success.
     * \return \e false on error.
     *
     * This function gives the helper a handle to the writer
     * to use in order to forward requests. Thus, when the
     * user calls a helper function, the call is forwarded
     * to the correct writer that effictively does the work.
     * The aim of this helper is simply to transform standard
     * EBML types into bytes buffers.
     *
     * Once a writer is connected, it could be disconnected
     * thanks to the \c disconnect function. It must be done
     * before calling \c connect again.
     */
    virtual bool connect(IWriter* writer) = 0;
    /**
     * \brief Disconnects the currently connected EBML writer
     * \return \e true on success.
     * \return \e false on error.
     *
     * This function should be called to release the EBML
     * writer handle. The helper instance may then be used
     * with another EBML writer instance calling \c connect
     * again.
     */
    virtual bool disconnect() = 0;
    //@}

    /** \name Writer binding functions */
    //@{
    /**
     * \brief Child opening binding
     * \param identifier [in] : The identifier of the new child node
     * \return \e true on success.
     * \return \e false on error.
     *
     * This function simply forwards the call to the
     * corresponding EBML::IWriter function. See
     * EBML::IWriter::openChild for more details.
     */
    virtual bool openChild(const CIdentifier& identifier) = 0;
    /**
     * \brief Child closing binding
     * \return \e true on success.
     * \return \e false on error.
     *
     * This function simply forwards the call to the
     * corresponding EBML::IWriter function. See
     * EBML::IWriter::closeChild for more details.
     */
    virtual bool closeChild() = 0;
    //@}

    /**
     * \name Standard EBML formating
     * \brief EBML::IWriter::setChildData replacement
     * \return \e true on success.
     * \return \e false on error.
     *
     * Those functions should be used in place of the
     * basic EBML::IWriter::setChildData function. They
     * format standard EBML types into corresponding
     * buffers and then send those built buffers to
     * the writer using the EBML::IWriter::setChildData
     * function.
     */
    //@{
    /**
     * \brief Sets a signed integer as child data
     * \param value [in] : The integer value to set
     */
    virtual bool setInt(const int64_t value) = 0;
    /**
     * \brief Sets an unsigned integer as child data
     * \param value [in] : The integer value to set
     */
    virtual bool setUInt(const uint64_t value) = 0;
    /**
     * \brief Sets a 32 bits float value as child data
     * \param value [in] : The 32 bits float value to set
     */
    virtual bool setFloat(const float value) = 0;
    /**
     * \brief Sets a 64 bits float value as child data
     * \param value [in] : The 64 bits float value to set
     */
    virtual bool setDouble(const double value) = 0;
    // virtual bool setFloat80AsChildData( ??? value)=0;
    // virtual bool setDateAsChildData( ??? value)=0;
    /**
     * \brief Sets a buffer as child data
     * \param buffer [in] : The buffer to send to the writer
     * \param size [in] : The buffer size in bytes
     * \note This function simply calls the basic
     *       EBML::IWriter::setChildData function with the
     *       same two parameters.
     */
    virtual bool setBinary(const void* buffer, const size_t size) = 0;
    /**
     * \brief Sets an ASCII string as child data
     * \param value [in] : The ASCII string value to set
     */
    virtual bool setStr(const char* value) = 0;
    // virtual bool setUTF8StringAsChildData( ??? value)=0;
    //@}

    /**
     * \brief Tells this object it won't be used anymore
     *
     * Instances of this class can not be instanciated
     * another way than calling \c createWriterHelper. They
     * can not be deleted either because the destructor is.
     * protected. The library knows how to create and
     * delete an instance of this class... Calling
     * \c release will simply delete this instance and
     * handle necessary cleanings when needed.
     *
     * The current object is invalid after calling this
     * function. It can not be used anymore.
     *
     * \warning Releasing this obbject does not release the
     *          connected writer instance !
     */
    virtual void release() = 0;

protected:

    /**
     * \brief Virtual destructor - should be overloaded
     */
    virtual ~IWriterHelper() { }
};

/**
 * \brief Instanciation function for EBML writer helper objects
 * \return a pointer to the created instance on success.
 * \return \c NULL when something went wrong.
 */
extern EBML_API IWriterHelper* createWriterHelper();
}  // namespace EBML
