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
#include "CMemoryBuffer.hpp"

#include <cstring> // memcpy

namespace OpenViBE {

///-------------------------------------------------------------------------------------------------
CMemoryBuffer::~CMemoryBuffer()
{
    if (m_buffer) {
        delete [] m_buffer;
        m_buffer = nullptr;
    }
}

///-------------------------------------------------------------------------------------------------
bool CMemoryBuffer::reserve(const size_t size)
{
    if (size > m_allocatedSize) {
        uint8_t* buffer = m_buffer;
        m_buffer        = new uint8_t[size_t(size + 1)]; // $$$
        if (!m_buffer) { return false; }
        memcpy(m_buffer, buffer, size_t(m_size)); // $$$

        delete [] buffer;
        m_allocatedSize           = size;
        m_buffer[m_allocatedSize] = 0;
    }
    return true;
}

///-------------------------------------------------------------------------------------------------
bool CMemoryBuffer::setSize(const size_t size, const bool discard)
{
    if (size > m_allocatedSize) {
        uint8_t* buffer = m_buffer;
        m_buffer        = new uint8_t[size_t(size + 1)]; // $$$
        if (!m_buffer) { return false; }
        if (!discard) { memcpy(m_buffer, buffer, size_t(m_size)); } // $$$
        delete [] buffer;
        m_allocatedSize           = size;
        m_buffer[m_allocatedSize] = 0;
    }
    m_size = size;
    return true;
}

///-------------------------------------------------------------------------------------------------
bool CMemoryBuffer::append(const uint8_t* buffer, const size_t size)
{
    if (size != 0) {
        const size_t bufferSizeBackup = m_size;
        if (!this->setSize(m_size + size, false)) { return false; }
        memcpy(m_buffer + bufferSizeBackup, buffer, size_t(size));
    }
    return true;
}

///-------------------------------------------------------------------------------------------------
void CMemoryBuffer::copy(const uint8_t* buffer, const size_t size)
{
    m_buffer = new uint8_t[size_t(size + 1)]; // $$$
    if (m_buffer) {
        m_size          = size;
        m_allocatedSize = size;
        if (buffer) {
            memcpy(m_buffer, buffer, size_t(m_size)); // $$$
        }
        m_buffer[m_allocatedSize] = 0;
    }
}

}  // namespace OpenViBE
