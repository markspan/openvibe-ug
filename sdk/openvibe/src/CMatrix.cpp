#include "CMatrix.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <cmath>
#include <fstream>
#include <iostream>

namespace OpenViBE {

//--------------------------------------------------------------------------------
static std::string trim(const std::string& in, const bool space = true, const bool comment = true)
{
    if (in.empty()) { return ""; }
    std::string res(in);
    if (comment) {
        const std::string comments("#");
        const size_t start = res.find_first_of(comments);
        if (start != std::string::npos) { res.erase(start); }   // delete from start until the end
    }
    if (space) {
        const std::string spaces(" \t\f\v\n\r");
        const size_t start = res.find_first_not_of(spaces);
        const size_t end   = res.find_last_not_of(spaces);
        if (start == std::string::npos) { return ""; }
        //if (end == std::string::npos) { return ""; }  // uselesss same as previous
        res = res.substr(start, end + 1);               // end + 1 to include the last non space caracter
    }
    return res;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
static bool getCleanLine(std::ifstream& file, std::string& line)
{
    if (std::getline(file, line)) {
        line = trim(line);
        return true;
    }
    return false;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------
//----------------- Getter/Setter ------------------
//--------------------------------------------------

//--------------------------------------------------------------------------------
void CMatrix::setDimensionCount(const size_t count) const
{
    if (count == 0) {   // If dimension number is 0, make nothing
        std::cerr << "[ERROR] CMatrix::setDimensionCount: Dimension count can't be set to 0." << std::endl;
        return;
    }
    clearBuffer();              // Reset Buffer pointer
    m_dimSizes->resize(count);
    m_dimLabels->resize(count);
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void CMatrix::setDimensionSize(const size_t dim, const size_t size) const
{
    if (dim >= m_dimSizes->size()) {    // If out of dimension make nothing
        std::cerr << "[ERROR] CMatrix::setDimensionSize: Cannot set dimension " << dim << " as the matrix contains only "
                << m_dimSizes->size() << " dimensions." << std::endl;
        return;
    }
    clearBuffer();                              // Reset Buffer pointer
    m_dimSizes->at(dim) = size;
    m_dimLabels->at(dim).resize(size);
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void CMatrix::setDimensionLabel(const size_t dim, const size_t idx, const std::string& label) const
{
    // If out of dimension make nothing
    if (dim >= m_dimLabels->size()) {
        std::cerr << "[ERROR] CMatrix::setDimensionLabel: Cannot set label in dimension " << dim << " as the matrix contains only "
                << m_dimSizes->size() << " dimensions." << std::endl;
        return;
    }

    if (idx >= m_dimLabels->at(dim).size()) {
        std::cerr << "[ERROR] CMatrix::setDimensionLabel: Cannot set label in index " << idx << " of dimension " << dim << " as the dimension contains only "
                << m_dimLabels->at(dim).size() << " index." << std::endl;
        return;
    }
    m_dimLabels->at(dim)[idx] = label;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------
//---------------------- Misc ----------------------
//--------------------------------------------------

//--------------------------------------------------------------------------------
bool CMatrix::isBufferValid(const bool checkNaN, const bool checkInf) const
{
    for (size_t i = 0; i < m_size; ++i) { if ((checkNaN && std::isnan(m_buffer[i])) || (checkInf && std::isinf(m_buffer[i]))) { return false; } }
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CMatrix::isDescriptionEqual(const CMatrix& m, const bool checkLabels) const
{
    if (getDimensionCount() != m.getDimensionCount()) { return false; }
    for (size_t i = 0; i < getDimensionCount(); ++i) { if (getDimensionSize(i) != m.getDimensionSize(i)) { return false; } }

    if (checkLabels) {
        for (size_t i = 0; i < getDimensionCount(); ++i) {
            for (size_t j = 0; j < getDimensionSize(i); ++j) {
                // I use directly vectors to avoid conversion in char* (conversion is here to keep previous compability)
                if (m_dimLabels->at(i)[j] != m.m_dimLabels->at(i)[j]) { return false; }
            }
        }
    }
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CMatrix::isBufferEqual(const CMatrix& m) const
{
    if (getSize() != m.getSize()) { return false; }
    return std::equal(m_buffer, m_buffer + m_size, m.getBuffer());
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CMatrix::isBufferAlmostEqual(const CMatrix& m, const double epsilon) const
{
    if (getSize() != m.getSize()) { return false; }
    const double *a = getBuffer(), *b = m.getBuffer();
    for (size_t i = 0; i < getSize(); ++i, ++a, ++b) { if (std::fabs(*a - *b) >= std::fabs(epsilon)) { return false; } }
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void CMatrix::resize(const std::vector<size_t>& sizes)
{
    clean();        // Delete all pointer and initialize vector pointer
    setDimensionCount(sizes.size());
    for (size_t i = 0; i < sizes.size(); ++i) { setDimensionSize(i, sizes[i]); }
    resetBuffer();  // Initialise buffer with 0 always (if size don't change too)
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void CMatrix::clean()
{
    clearBuffer();
    initVector();
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void CMatrix::copy(const CMatrix& m)
{
    if (this == &m) { return; }
    copyDescription(m);
    copyContent(m);
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void CMatrix::copyDescription(const CMatrix& m)
{
    if (this == &m) { return; }
    clear();
    m_dimSizes  = new std::vector<size_t>(*m.m_dimSizes);                       // Copy sizes
    m_dimLabels = new std::vector<std::vector<std::string>>(*m.m_dimLabels);    // Copy Labels
    resetBuffer();                                                              // Reset buffer
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void CMatrix::copyContent(const CMatrix& m) const
{
    if (this == &m) { return; }
    initBuffer();
    if (m_buffer && m.getSize() == getSize()) { std::copy_n(m.getBuffer(), getSize(), m_buffer); }
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void CMatrix::resetBuffer() const
{
    if (!m_buffer) { initBuffer(); }
    if (m_buffer) { std::fill_n(m_buffer, m_size, 0); }
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void CMatrix::setNumLabels() const { for (auto& dim : *m_dimLabels) { for (size_t i = 0; i < dim.size(); ++i) { dim[i] = std::to_string(i + 1); } } }
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CMatrix::toTextFile(const std::string& filename) const
{
    std::ofstream file;
    file.open(filename, std::ios_base::out | std::ios_base::trunc);
    if (!file.is_open()) { return false; }

    //----- Save -----
    // Labels
    // [
    //  [ "label 1 dim 1" "label 2 dim 1" ..."label m dim 1" ]
    //  [ "label 1 dim 2" "label 2 dim 2" ..."label m dim 2" ]
    //  ...
    //  [ "label 1 dim n" "label 2 dim n" ..."label m dim n" ]
    // ]
    file << "[" << std::endl;
    file << labelsToString("\t", "[ ", " ", "]");
    file << "]" << std::endl;

    // Matrix
    // 2D size (m,n)
    // [
    //  [ (0,0) (0,1) ... (0,n) ]
    //  [ (1,0) (1,1) ... (1,n) ]
    //  ...
    //  [ (m,0) (m,1) ... (m,n) ]
    // ]
    // 1D
    // [
    //  [ (0) (1) ... (n) ]
    // ]
    file << "[" << std::endl;
    file << bufferToString("\t", "[ ", " ", "]");
    file << "]" << std::endl;

    file.close();
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CMatrix::fromTextFile(const std::string& filename)
{
    //---------- Setup ----------
    std::ifstream file;
    file.open(filename);

    if (!file.is_open()) { return false; }

    // clean object
    clean();

    //---------- Labels ----------
    bool done = false;
    std::string line;
    while (!done && getCleanLine(file, line)) {
        if (line.empty() || line == "[") { continue; }
        if (line == "]") { done = true; }
        else {
            std::vector<std::string> labels;
            size_t n1 = line.find('\"');
            while (n1 != std::string::npos) {
                line            = line.substr(n1 + 1);          // We remove all until the "
                const size_t n2 = line.find('\"');              // Find the end of the label
                if (n2 == std::string::npos) { return false; }  // We found a " but not the second
                labels.push_back(line.substr(0, n2));           // We add the label to the list
                n1 = line.find('\"', n2 + 1);                   // We search after the " of the label.
            }
            if (labels.empty()) { return false; }
            m_dimLabels->push_back(labels);                     // We add a dimension with the labels
        }
    }

    // resize matrix (don't use resize functions to avoid delete actual labels
    m_dimSizes->resize(m_dimLabels->size());
    for (size_t i = 0; i < m_dimSizes->size(); ++i) { m_dimSizes->at(i) = m_dimLabels->at(i).size(); }
    initBuffer();                                               // allocate buffer

    //---------- Matrix ----------
    line = "";
    while (line.empty()) { getCleanLine(file, line); }          // remove comment and space before and after.
    if (line != "[") { return false; }                          // If next line isn't a mtrix opening

    std::stringstream ss;                                       // cumulate matrix to a stringstream
    while (getCleanLine(file, line) && line != "]") { if (!line.empty()) { ss << line << std::endl; } }
    return bufferFromString(ss.str(), "\t", "[ ", " ", "]");    // use stringstream to parse the matrix
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CMatrix::bufferFromString(const std::string& in, const std::string& before, const std::string& start, const std::string& sep, const std::string& end) const
{
    if (in.empty()) { return false; }
    initBuffer();                                                       // allocate buffer (in case of)
    const size_t startskip = trim(before + start).size();               // Size before first value for each line with remove space and comment
    const size_t endskip   = trim(sep + end).size();                    // size after last value for each line with remove space and comment
    size_t d               = 0, idx = 0;                                // Current Dimension and Current index
    size_t n1              = 0, n2  = in.find('\n');                    // Line delimiter

    while (n2 != std::string::npos && n1 + startskip <= n2 - endskip) {
        const size_t i = idx;                                           // keep actual idx
        std::istringstream ss(in.substr(n1 + startskip, n2 - endskip)); // We remove the begining and ending of the line
        double v = 0.0;
        while (ss >> v) {
            m_buffer[idx++] = v;                                        // Add to buffer and increment idx
            ss.ignore(std::streamsize(sep.size()));                     // Ignore separator between value
        }
        if (getDimensionCount() == 1 && idx != getDimensionSize(0)) { return false; }
        if (getDimensionCount() == 2 && idx - i != getDimensionSize(1)) { return false; }
        d++;                                                            // increment dimension
        n1 = n2 + 1;                                                    // Beginning of new line
        n2 = in.find('\n', n1);                                         // Ending of new line (we have allways a \n at the end)
    }
    // We check if we have completly fill the buffer
    return idx == m_size;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
std::string CMatrix::bufferToString(const std::string& before, const std::string& start, const std::string& sep, const std::string& end) const
{
    if (!m_buffer) { resetBuffer(); }
    std::stringstream ss;
    ss.precision(10);
    if (getDimensionCount() == 2) {
        size_t i = 0;
        for (size_t row = 0; row < getDimensionSize(0); ++row) {
            ss << before << start;
            for (size_t col = 0; col < getDimensionSize(1); ++col) { ss << m_buffer[i++] << sep; }
            ss << end << std::endl;
        }
    }
    else {
        ss << before << start;
        for (size_t i = 0; i < m_size; ++i) { ss << m_buffer[i] << sep; }
        ss << end << std::endl;
    }
    return ss.str();
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
std::string CMatrix::labelsToString(const std::string& before, const std::string& start, const std::string& sep, const std::string& end) const
{
    std::stringstream ss;
    for (const auto& dimension : *m_dimLabels) {
        ss << before << start;
        for (const auto& label : dimension) { ss << "\"" << label << "\"" << sep; }
        ss << end << std::endl;
    }
    return ss.str();
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void CMatrix::initBuffer() const
{
    if (m_buffer || m_dimSizes->empty()) { return; }

    m_size = 1;
    for (const auto& s : *m_dimSizes) { m_size *= s; }

    if (m_size != 0) { m_buffer = new double[m_size]; }
    if (!m_buffer) { m_size = 0; }
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void CMatrix::initVector()
{
    clearVector();
    m_dimSizes  = new std::vector<size_t>;
    m_dimLabels = new std::vector<std::vector<std::string>>;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void CMatrix::clearBuffer() const
{
    if (m_buffer) {
        delete[] m_buffer;
        m_buffer = nullptr;
    }
    m_size = 0;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void CMatrix::clearVector()
{
    if (m_dimSizes) {
        delete m_dimSizes;
        m_dimSizes = nullptr;
    }
    if (m_dimLabels) {
        delete m_dimLabels;
        m_dimLabels = nullptr;
    }
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void CMatrix::clear()
{
    clearBuffer();
    clearVector();
}
//--------------------------------------------------------------------------------

}  // namespace OpenViBE
