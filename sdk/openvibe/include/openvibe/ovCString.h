#pragma once

#include "ov_defines.h"

#include <cstddef>

namespace OpenViBE {
typedef struct SStringImpl SStringImpl;

/**
 * \class CString
 * \author Yann Renard (INRIA/IRISA)
 * \date 2006-08-10
 * \brief String class to avoid std::string in the interface
 * \ingroup Group_Base
 *
 * This class helps avoiding std::string being present in exposed
 * C++ interface, eventually resulting in compile/link errors when
 * dynamically loading modules.
 *
 * \note The implementation uses std::string of course :)
 */
class OV_API CString final
{
public:

    /** \name Constructor / Destructor */
    //@{

    /**
     * \brief Default constructor
     *
     * Initializes the string to an empty string.
     */
    CString();
    /**
     * \brief Copy constructor
     * \param str [in] : The string to copy
     *
     * Copies the content of \c str into the new string.
     */
    CString(const CString& str);
    /**
     * \brief Constructor based on ASCII strings
     * \param str [in] : The string to copy
     *
     * Copies the content of \c str into the new string.
     */
    CString(const char* str);
    /**
     * \brief Destructor
     *
     * The destructor releases the std::string implementation !
     */
    ~CString();

    //@}
    /** \name Operators */
    //@{

    /**
     * \brief ASCII string cast operator
     * \return The string formatted as standard ASCII string used in C.
     *
     * The implementation simply calls \c c_str().
     */
    operator const char*() const;
    /**
     * \brief Affectation operator (copy)
     * \param str [in] : The string to copy
     * \return This string.
     */
    CString& operator=(const CString& str);

    /**
     * \brief Addition assignment operator
     * \param str [in] : The string to append
     * \return This string.
     */
    CString& operator+=(const CString& str);

    /**
     * \brief Addition operator
     * \param str1 [in] : The first part of the resulting string
     * \param str2 [in] : The second part of the resulting string
     * \return The concatenation of \c str1 and \c str2.
     */
    friend OV_API CString operator+(const CString& str1, const CString& str2);
    /**
     * \brief Equality comparison operator
     * \param str1 [in] : The first part of the resulting string
     * \param str2 [in] : The second part of the resulting string
     * \return \e true is \c str1 is exactly str2.
     * \return \e false in other case.
     * \note This is case sensitive !
     */
    friend OV_API bool operator==(const CString& str1, const CString& str2);
    /**
     * \brief Inequality comparison operator
     * \param str1 [in] : The first part of the resulting string
     * \param str2 [in] : The second part of the resulting string
     * \return \e false is \c str1 is exactly str2.
     * \return \e true in other case.
     * \note This is case sensitive !
     */
    friend OV_API bool operator!=(const CString& str1, const CString& str2);

    /**
     * \brief Array subscription operator
     * \param idx [in] : Index in the array
     */
    char& operator[](size_t idx) const;

    /**
     * \brief Order comparison operator (necessary to use CString as a key in a stl map)
     * \param str1 [in] : The first part of the resulting string
     * \param str2 [in] : The second part of the resulting string
     * \return \e false is \c str1 is exactly str2.
     * \return \e true in other case.
     * \note This is case sensitive !
     */
    friend OV_API bool operator<(const CString& str1, const CString& str2);
    //@}

    /**
     * \brief Initializes this string from another OpenViBE string
     * \param str [in] : the OpenViBE string to initialize this string from
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    bool set(const CString& str) const;
    /**
     * \brief Initializes this string from an ANSI/ASCII string
     * \param str [in] : the ANSI/ASCII string to initialize this string from
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    bool set(const char* str) const;
    /**
     * \brief Converts this string to an ANSI/ASCII string
     * \return the ANSI/ASCII converted string.
     */
    const char* toASCIIString() const;

    /**
     * \brief Returns length of the string
     * \return Length of the string
     */
    size_t length() const;

protected:

    SStringImpl* m_impl = nullptr; ///< The string implementation
};
}  // namespace OpenViBE
