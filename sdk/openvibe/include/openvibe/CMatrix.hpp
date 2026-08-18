///-------------------------------------------------------------------------------------------------
/// 
/// \file CMatrix.hpp
/// \brief Basic standalone OpenViBE matrix implementation.
/// \author  Yann Renard (INRIA/IRISA) & Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 21/11/2007.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include "ov_defines.h"
#include <vector>
#include <string>
#include <iostream> // cerr

namespace OpenViBE {

/// <summary> OpenViBE Matrix Class. </summary>
class OV_API CMatrix
{
public:
    //--------------------------------------------------
    //------------ Constructor / Destructor ------------
    //--------------------------------------------------
    /// <summary> Default constructor. </summary>
    CMatrix() { initVector(); }

    /// <summary> Constructor for 1D matrix with initial dimensions. </summary>
    /// <param name="dim"> The length of vector. </param>
    explicit CMatrix(const size_t dim) { resize(dim); }

    /// <summary> Constructor for 2D matrix with initial dimensions. </summary>
    /// <param name="dim1"> The first dimension size. </param>
    /// <param name="dim2"> The second dimension size. If it's <c>0</c>, the matrix is a Row vector. </param>
    explicit CMatrix(const size_t dim1, const size_t dim2) { resize(dim1, dim2); }

    /// <summary> Copy Constructor. </summary>
    /// <param name="m"> the matrix to copy. </param>
    CMatrix(const CMatrix& m) { copy(m); }

    /// <summary> Default destructor. </summary>
    ~CMatrix() { clear(); }

    //--------------------------------------------------
    //----------------- Getter/Setter ------------------
    //--------------------------------------------------
    /// <summary> Gets the number of dimension. </summary>
    /// <returns> THe number of dimension. </returns>
    size_t getDimensionCount() const { return m_dimSizes->size(); }

    /// <summary> Gets the size of the dimension <c>index</c>. </summary>
    /// <param name="index">The dimension. </param>
    /// <returns> the size of the selected dimension. </returns>
    size_t getDimensionSize(const size_t index) const { return (index >= m_dimSizes->size()) ? 0 : m_dimSizes->at(index); }

    /// <summary> Gets the label of the selected index in selected dimensions. </summary>
    /// <param name="dim"> The dimension concerned. </param>
    /// <param name="idx"> The index on this dimension. </param>
    /// <returns> The label. </returns>
    /// <remarks> It's a <c>const char*</c> object to keep previous compatibility with CString, intended to be replace by std::string. </remarks>
    const char* getDimensionLabel(const size_t dim, const size_t idx) const
    {
        return (dim >= m_dimSizes->size() || idx >= m_dimSizes->at(dim)) ? "" : m_dimLabels->at(dim)[idx].c_str();
    }

    /// <summary> Const Buffer Accessor. </summary>
    /// <returns> The buffer. </returns>
    const double* getBuffer() const { return m_buffer; }

    /// <summary> Buffer Accessor. </summary>
    /// <returns> The buffer. </returns>
    double* getBuffer()
    {
        if (!m_buffer) { initBuffer(); }    // Initialize buffer if needed
        return m_buffer;
    }

    /// <summary> Get the number of element in buffer. </summary>
    /// <returns> The number of element. </returns>
    size_t getSize() const
    {
        if (!m_buffer || m_size == 0) { initBuffer(); } // Initialize buffer if needed
        return m_size;
    }

    /// <summary> Get the number of element in buffer. </summary>
    /// <remarks> keep previous compatibility with heavy name. Avoid to used it, intended to be removed. </remarks>
    size_t getBufferElementCount() const { return getSize(); }


    /// <summary> Get the dimension's sizes vector. </summary>
    /// <returns> The member <see cref="m_dimSizes"/>. </returns>
    std::vector<size_t>* getSizes() const { return m_dimSizes; }

    /// <summary> Get the dimension's labels vector. </summary>
    /// <returns> The member <see cref="m_dimLabels"/>. </returns>
    std::vector<std::vector<std::string>>* getLabels() const { return m_dimLabels; }

    /// <summary> Set the number of dimensions. </summary>
    /// <param name="count">The number of dimensions. </param>
    void setDimensionCount(const size_t count) const;

    /// <summary> Set the size of the selected dimension. </summary>
    /// <param name="dim"> The selected dimension. </param>
    /// <param name="size"> The new size. </param>
    void setDimensionSize(const size_t dim, const size_t size) const;

    /// <summary> Set the label of the index in the selected dimension. </summary>
    /// <param name="dim"> The dimension concerned. </param>
    /// <param name="idx"> The index concerned. </param>
    /// <param name="label"> The Label to set. </param>
    void setDimensionLabel(const size_t dim, const size_t idx, const std::string& label) const;

    /// <summary> Set the label of the index in the selected dimension (keep previous compatibility with char* for CString). </summary>
    /// <remarks> keep previous compatibility with heavy name. Avoid to used it, intended to be removed. </remarks>
    void setDimensionLabel(const size_t dim, const size_t idx, const char* label) const { setDimensionLabel(dim, idx, std::string(label)); }

    /// <summary> Fill the matrix with the buffer. </summary>
    /// <param name="buffer"> The buffer to copy. </param>
    /// <param name="size"> The size of the buffer. </param>
    /// <remarks> The buffer can contain any numeric type. The CMatrix class stores them as double. </remarks>
    template <class T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
    void setBuffer(const T* buffer, const size_t size)
    {
        if (!m_buffer) { initBuffer(); }                // Initialize buffer if needed
        const auto s = size > m_size ? m_size : size;
        if (size > m_size) {
            std::cerr << "[ERROR] CMatrix::setBuffer: Provided buffer (size: " << size << ") is bigger than the local buffer (size: "
                    << m_size << "). Copy will be made up to " << m_size << " elements." << std::endl;
        }
        for (size_t i = 0; i < s; ++i) { m_buffer[i] = double(buffer[i]); }
    }

    /// <summary> Fill the matrix with the buffer. </summary>
    /// <param name="buffer"> The buffer to copy. </param>
    /// <remarks> The buffer can contain any numeric type. The CMatrix class stores them as double. </remarks>
    template <class T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
    void setBuffer(const std::vector<T>& buffer) { return setBuffer(buffer.data(), buffer.size()); }

    //--------------------------------------------------
    //------------------- Operators --------------------
    //--------------------------------------------------

    /// <summary> Copy Assignment Operator. </summary>
    /// <param name="m"> The matrix to copy. </param>
    /// <returns> Himself. </returns>
    CMatrix& operator=(const CMatrix& m)
    {
        if (this != &m) { copy(m); }
        return *this;
    }

    /// <summary>   Override the egal operator. </summary>
    /// <param name="obj">  The second object. </param>
    /// <returns>   True if the two <see cref="CMatrix"/> are equals. </returns>
    bool operator==(const CMatrix& obj) const { return isAlmostEqual(obj); }

    /// <summary> Overload of const operator []. </summary>
    /// <param name="index"> The index. </param>
    /// <returns> Const Reference of the object. </returns>
    const double& operator [](const size_t index) const { return this->getBuffer()[index]; }

    /// <summary> Overload of operator []. </summary>
    /// <param name="index"> The index. </param>
    /// <returns> Reference of the object. </returns>
    double& operator [](const size_t index) { return this->getBuffer()[index]; }

    //--------------------------------------------------
    //---------------------- Misc ----------------------
    //--------------------------------------------------

    /// <summary> Determines if the buffer contains <c>NaN</c> or <c>Ininity</c>. </summary>
    /// <param name="checkNaN"> Check if <c>NaN</c> is in buffer or not. </param>
    /// <param name="checkInf"> Check if <c>Ininity</c> is in buffer or not. </param>
    /// <returns> <c>true</c> if buffer is valid doesn't have <c>NaN</c> or <c>Ininity</c> value, <c>false</c> otherwise. </returns>
    bool isBufferValid(const bool checkNaN = true, const bool checkInf = true) const;

    /// <summary> Determines if the size and the labels of the matrix are equals. </summary>
    /// <param name="m"> The matrix to compare. </param>
    /// <param name="checkLabels"> The check labels or not. </param>
    /// <returns> <c>true</c> if description is equal, <c>false</c> otherwise. </returns>
    bool isDescriptionEqual(const CMatrix& m, const bool checkLabels = true) const;

    /// <summary> Determines if the buffer is exactly the same. </summary>
    /// <param name="m"> The matrix to compare. </param>
    /// <returns> <c>true</c> if buffer is equal, <c>false</c> otherwise. </returns>
    bool isBufferEqual(const CMatrix& m) const;

    /// <summary> Determines if the buffer is almost equals. </summary>
    /// <param name="m"> The matrix to compare. </param>
    /// <param name="epsilon"> Tolerance for the difference. </param>
    /// <returns> <c>true</c> if buffer is equal, <c>false</c> otherwise. </returns>
    /// <remarks> We use it to avoid double precision problems. </remarks>
    bool isBufferAlmostEqual(const CMatrix& m, const double epsilon = OV_EPSILON) const;

    /// <summary> Determines if the two <see cref="CMatrix"/> (Description & Buffer is exactly the same. </summary>
    /// <param name="m"> The matrix to compare. </param>
    /// <returns>   True if the two <see cref="CMatrix"/> are exactly the same. </returns>
    bool isEqual(const CMatrix& m) const { return isDescriptionEqual(m) && isBufferEqual(m); }

    /// <summary> Determines if the two <see cref="CMatrix"/> (Description & Buffer) is almost equal. </summary>
    /// <param name="m"> The matrix to compare. </param>
    /// <param name="epsilon"> Tolerance for the difference. </param>
    /// <returns>   True if the two <see cref="CMatrix"/> are exactly the same. </returns>
    /// <remarks> We use it to avo�d double precision problems. </remarks>
    bool isAlmostEqual(const CMatrix& m, const double epsilon = OV_EPSILON) const { return isDescriptionEqual(m) && isBufferAlmostEqual(m, epsilon); }


    /// <summary> Resize 1D matrix and set matrix value to 0. </summary>
    /// <param name="dim"> The length of vector. </param>
    void resize(const size_t dim) { resize(std::vector<size_t>{ dim }); }

    /// <summary> Resize 2D matrix and set matrix value to 0. </summary>
    /// <param name="dim1"> The first dimension. </param>
    /// <param name="dim2"> The second dimension. </param>
    void resize(const size_t dim1, const size_t dim2) { resize(std::vector<size_t>{ dim1, dim2 }); }

    /// <summary> Resize ND matrix with given sizes and set matrix value to 0. </summary>
    /// <param name="sizes"> The  sizes of dimensions. </param>
    void resize(const std::vector<size_t>& sizes);

    /// <summary> Delete all pointer and reset size to 0 and init vector pointer. </summary>
    void clean();

    /// <summary> Copy matrix to this instance. </summary>
    /// <param name="m"> The matrix to copy. </param>
    void copy(const CMatrix& m);

    /// <summary> Copy the size and the labels of matrix to this instance. </summary>
    /// <param name="m"> The matrix to copy. </param>
    void copyDescription(const CMatrix& m);

    /// <summary> Copy the content of matrix to this instance. </summary>
    /// <param name="m"> The matrix to copy. </param>
    void copyContent(const CMatrix& m) const;

    /// <summary> Set all element of the buffer to 0. </summary>
    void resetBuffer() const;

    /// <summary> Set all labels to "". </summary>
    void resetLabels() const { for (auto& dim : *m_dimLabels) { for (auto& l : dim) { l = ""; } } }

    /// <summary> Set all dimension labels are set from 1 to Dim Size. </summary>
    void setNumLabels() const;

    /// <summary> Save to the text file. </summary>
    /// <param name="filename"> The filename. </param>
    /// <returns> <c>true</c> if succeed, <c>false</c> otherwise. </returns>
    bool toTextFile(const std::string& filename) const;

    /// <summary> Load from the text file. </summary>
    /// <param name="filename"> The filename. </param>
    /// <returns> <c>true</c> if succeed, <c>false</c> otherwise. </returns>
    bool fromTextFile(const std::string& filename);

    /// <summary> Fill matrix from string. </summary>
    /// <param name="in">   The string with the matrix. </param>
    /// <param name="before">   caracter before the line (by default nothing). </param>
    /// <param name="start">    caracter when the line start (by default nothing). </param>
    /// <param name="sep">  separator between value (by default tabulation). </param>
    /// <param name="end">  caracter when the line finish (by default nothing). </param>
    /// <returns> <c>true</c> if succeed, <c>false</c> otherwise. </returns>
    bool bufferFromString(const std::string& in, const std::string& before = "", const std::string& start = "",
                          const std::string& sep                           = "\t", const std::string& end = "") const;

    /// <summary> Display the matrix. </summary>
    /// <param name="before">   caracter before the line (by default nothing). </param>
    /// <param name="start">    caracter when the line start (by default nothing). </param>
    /// <param name="sep">  separator between value (by default tabulation). </param>
    /// <param name="end">  caracter when the line finish (by default nothing). </param>
    /// <returns> the Matrix. </returns>
    /// <remarks> Display works for 2D and 1D matrix respectively in table format or a row. </remarks>
    std::string bufferToString(const std::string& before = "", const std::string& start = "", const std::string& sep = "\t", const std::string& end = "") const;

    /// <summary> Display the Labels. </summary>
    /// <param name="before">   caracter before the line (by default nothing). </param>
    /// <param name="start">    caracter when the line start (by default nothing). </param>
    /// <param name="sep">  separator between value (by default tabulation). </param>
    /// <param name="end">  caracter when the line finish (by default nothing). </param>
    /// <returns> the Labels. </returns>
    std::string labelsToString(const std::string& before = "", const std::string& start = "", const std::string& sep = "\t", const std::string& end = "") const;

    /// <summary> Override the ostream operator. </summary>
    /// <param name="os">   The ostream. </param>
    /// <param name="obj">  The object. </param>
    /// <returns> Return the modified ostream. </returns>
    friend std::ostream& operator<<(std::ostream& os, const CMatrix& obj)
    {
        os << obj.bufferToString();
        return os;
    }

private:
    /// <summary> Init Buffer and number of element. </summary>
    void initBuffer() const;

    /// <summary> Initialize vector pointer. </summary>
    void initVector();

    /// <summary> Delete Buffer pointer and reset size to 0. </summary>
    void clearBuffer() const;

    /// <summary> Delete vector pointer. </summary>
    void clearVector();

    /// <summary> Delete all pointer and reset size to 0. </summary>
    /// <remarks> Be carefull with this, must use a new constructor or resize function to allocate all pointers. </remarks>
    void clear();

    mutable double* m_buffer = nullptr; ///< The matrix buffer.
    mutable size_t m_size    = 0;       ///< The number of element.

    std::vector<size_t>* m_dimSizes                    = nullptr;   ///< Size of all dimensions (pointer to avoid export warning C4251)
    std::vector<std::vector<std::string>>* m_dimLabels = nullptr;   ///< Labels of all dimensions (pointer to avoid export warning C4251)
};

/// \deprecated Use the CMatrix class instead.
OV_Deprecated("Use the CMatrix class instead")
typedef CMatrix IMatrix;    ///< Keep previous compatibility. Avoid to used it, intended to be removed. 

}  // namespace OpenViBE
