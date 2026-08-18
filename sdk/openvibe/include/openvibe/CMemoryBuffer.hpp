///-------------------------------------------------------------------------------------------------
/// 
/// \author Yann Renard (INRIA/IRISA).
/// \version 1.0.
/// \date 21/11/2007.
/// \copyright (C) 2022 INRIA
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
/// See the GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program. If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------
#pragma once

#include <ov_common_defines.h>

namespace OpenViBE {

/// <summary> Basic standalone OpenViBE memory buffer implementation. </summary>
///
/// This class offers functionalities to basically manipulate a raw memory buffer.
/// It allows the buffer to be resized and manipulated easily with no care of allocation reallocation.
/// Implementations for this interface may provide optimisations for such operations.
///
/// \ingroup Group_Base
class OV_API CMemoryBuffer
{
public:
    /** \name Constructors */
    //@{

    /// <summary> Initializes a new instance of the <see cref="CMemoryBuffer"/> . </summary>
    ///
    /// This constructor builds the internal implementation of this memory buffer.
    CMemoryBuffer() = default;

    /// <summary> Initializes a new instance of the <see cref="CMemoryBuffer"/> . </summary>
    ///
    /// This constructor builds the internal implementation of this memory buffer and initializes it with the actual parameter of the constructor as a copy.
    /// <param name="buffer">The buffer.</param>
    CMemoryBuffer(const CMemoryBuffer& buffer) { copy(buffer); }

    /// <summary> Initializes a new instance of the <see cref="CMemoryBuffer"/> . </summary>
    ///
    /// This constructor builds the internal implementation of this memory buffer and initializes it with the actual parameter of the constructor as a copy.
    /// <param name="buffer"> The buffer. </param>
    /// <param name="size"> The size of the buffer. </param>
    CMemoryBuffer(const uint8_t* buffer, const size_t size) { copy(buffer, size); }

    /// <summary> Finalizes an instance of the <see cref="CMemoryBuffer"/> class. </summary>
    ///
    /// The internal implementation is released.
    ~CMemoryBuffer();

    //@}

    /** \name Management */
    //@{

    /// <summary> Reserves some memory for this memory buffer. </summary>
    ///
    /// This function reserves some memory space for later use.
    /// This does not affect the actual size of the buffer but allows later calls to <c>append</c> not to reallocate the whole buffer.
    /// <param name="size"> the amount of memory to reserve. </param>
    /// <returns> <c>true</c> in case of success, <c>false</c> otherwise. </returns>
    /// <remarks> if <c>size</c> is lower than the actual buffer size then <c>true</c> is returned and nothing is done. </remarks>
    bool reserve(const size_t size);

    /// <summary> Changes the size of this memory buffer. </summary>
    /// <param name="size"> the new size to give to the buffer. </param>
    /// <param name="discard"> Tells the reallocation process whether it should presever currently stored data or not. </param>
    /// <returns> <c>true</c> in case of success, <c>false</c> otherwise. </returns>
    /// <remarks> On error, the buffer is left unchanged.
    /// If the new size if lower than the current sizeand \c discard is true, the buffer is simply truncated to the \c size first bytes. </remarks>
    bool setSize(const size_t size, const bool discard);

    /// <summary> Gets the current size of this memory buffer. </summary>
    /// <returns> The current size of this memory buffer. </returns>
    size_t getSize() const { return m_size; }

    /// <summary> Gets a direct pointer to the byte array for read/write access. </summary>
    /// <returns> A direct pointer to the byte array for read/write access. </returns>
    uint8_t* getDirectPointer() { return m_buffer; }

    /// <summary> Gets a direct pointer to the byte array for read access. </summary>
    /// <returns> A direct pointer to the byte array for read access. </returns>
    const uint8_t* getDirectPointer() const { return m_buffer; }

    /// <summary> Appends data to this memory buffer. </summary>
    /// <param name="buffer"> the buffer containing data that should be appended.</param>
    /// <param name="size"> the buffer size that should be appended. </param>
    /// <returns> <c>true</c> in case of success, <c>false</c> otherwise. </returns>
    bool append(const uint8_t* buffer, const size_t size);

    /// <summary> Appends data to this memory buffer. </summary>
    /// <param name="buffer"> the buffer containing data that should be appended.</param>
    /// <returns> <c>true</c> in case of success, <c>false</c> otherwise. </returns>
    bool append(const CMemoryBuffer& buffer) { return append(buffer.getDirectPointer(), buffer.getSize()); }

    //@}

    /** \name Operators */
    //@{

    /// <summary> Copy Assignment Operator. </summary>
    /// <param name="buffer"> The buffer to copy. </param>
    /// <returns> Himself. </returns>
    CMemoryBuffer& operator=(const CMemoryBuffer& buffer)
    {
        if (this == &buffer) { return *this; }
        copy(buffer);
        return *this;
    }

    /// <summary> Overload of const operator []. </summary>
    /// <param name="index"> The index. </param>
    /// <returns> Const Reference of the object. </returns>
    const uint8_t& operator [](const size_t index) const { return this->getDirectPointer()[index]; }

    /// <summary> Overload of operator []. </summary>
    /// <param name="index"> The index. </param>
    /// <returns> Reference of the object. </returns>
    uint8_t& operator [](const size_t index) { return this->getDirectPointer()[index]; }

    //@}

protected:
    /// <summary> Copy Memory Buffer to this instance. </summary>
    /// <param name="buffer"> The buffer to copy. </param>
    void copy(const CMemoryBuffer& buffer) { copy(buffer.getDirectPointer(), buffer.getSize()); }

    /// <summary> Copy buffer to this instance. </summary>
    /// <param name="buffer"> The buffer to copy. </param>
    /// <param name="size"> The size of the buffer. </param>
    void copy(const uint8_t* buffer, const size_t size);


    uint8_t* m_buffer      = nullptr;   ///< Buffer
    size_t m_size          = 0;         ///< Size of Buffer
    size_t m_allocatedSize = 0;         ///< Size allocated of Buffer
};

/// \deprecated Use the CMemoryBuffer class instead
OV_Deprecated("Use the CMemoryBuffer class instead")
typedef CMemoryBuffer IMemoryBuffer;    ///< Keep previous compatibility. Avoid to used it, intended to be removed.
}  // namespace OpenViBE
