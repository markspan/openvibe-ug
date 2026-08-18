#include "ovtkMatrix.h"

#include <fs/Files.h>

#include <cstring>
#include <cmath>
#include <cstdlib>
#include <cerrno>

// for save/load
#include <fstream>
#include <iostream>
#include <vector>
#include <locale> // std::isspace
#include <sstream>

namespace OpenViBE {
namespace Toolkit {
namespace Matrix {

enum class EParsingStatus { Nothing, ParsingHeader, ParsingHeaderDimension, ParsingHeaderLabel, ParsingBuffer, ParsingBufferValue };

// tokens in the ascii matrix format
const char CONSTANT_LEFT_SQUARE_BRACKET  = '[';
const char CONSTANT_RIGHT_SQUARE_BRACKET = ']';
const char CONSTANT_HASHTAG              = '#';
const char CONSTANT_DOUBLE_QUOTE         = '"';
const char CONSTANT_TAB                  = '\t';
const char CONSTANT_CARRIAGE_RETURN      = '\r';
const char CONSTANT_EOL                  = '\n';
const char CONSTANT_SPACE                = ' ';

bool fromString(CMatrix& matrix, const CString& str)
{
    std::stringstream buffer;

    buffer << str.toASCIIString();

    const std::locale locale("C");
    //current string to parse
    std::string what;
    //current parsing status
    EParsingStatus status = EParsingStatus::Nothing;
    //current element index (incremented every time a value is stored in matrix)
    size_t curElementIdx = 0;
    //current dimension index
    size_t curDimIdx = size_t(-1);
    //vector keeping track of dimension sizes
    std::vector<size_t> dimSize;
    //vector keeping track of number of values found in each dimension
    std::vector<size_t> nValue;
    // Dim labels
    std::vector<std::string> labels;
    //current quote-delimited string
    std::string curString;

    do {
        //read current line
        std::getline(buffer, what, CONSTANT_EOL);

        //is line empty?
        if (what.length() == 0) { continue; } //skip it

        //output line to be parsed in debug level
        // getLogManager() << Kernel::LogLevel_Debug << what << "\n";

        //remove ending carriage return (if any) for windows / linux compatibility
        if (what[what.length() - 1] == CONSTANT_CARRIAGE_RETURN) { what.erase(what.length() - 1, 1); }

        //start parsing current line
        auto it = what.begin();

        //parse current line
        while (it != what.end()) {
            switch (status) {
                    //initial parsing status
                case EParsingStatus::Nothing:

                    //comments starting
                    if (*it == CONSTANT_HASHTAG) { it = what.end() - 1; }                               // ignore rest of line by skipping to last character
                        //header starting
                    else if (*it == CONSTANT_LEFT_SQUARE_BRACKET) { status = EParsingStatus::ParsingHeader; }   // update status
                    else if (!std::isspace(*it, locale)) { return false; }
                    break;

                    //parse header
                case EParsingStatus::ParsingHeader:

                    //comments starting
                    if (*it == CONSTANT_HASHTAG) { it = what.end() - 1; }   //ignore rest of line by skipping to last character
                        //new dimension opened
                    else if (*it == CONSTANT_LEFT_SQUARE_BRACKET) {
                        dimSize.resize(dimSize.size() + 1);                 //increment dimension count
                        curDimIdx++;                                        //update current dimension index
                        status = EParsingStatus::ParsingHeaderDimension;    //update status
                    }
                        //finished parsing header
                    else if (*it == CONSTANT_RIGHT_SQUARE_BRACKET) {
                        if (dimSize.empty()) { return false; }  //ensure at least one dimension was found
                        matrix.resize(dimSize);                 //resize matrix
                        nValue.resize(matrix.getDimensionCount());

                        // set labels now that we know the matrix size
                        size_t idx = 0;
                        for (size_t i = 0; i < matrix.getDimensionCount(); ++i) {
                            for (size_t j = 0; j < matrix.getDimensionSize(i); ++j) { matrix.setDimensionLabel(i, j, labels[idx++].c_str()); }
                        }

                        //reset current dimension index
                        curDimIdx = size_t(-1);
                        //update status
                        status = EParsingStatus::ParsingBuffer;
                    }
                    else if (!std::isspace(*it, locale)) { return false; }
                    break;

                case EParsingStatus::ParsingHeaderDimension:

                    //comments starting
                    if (*it == CONSTANT_HASHTAG) { it = what.end() - 1; }   //ignore rest of line by skipping to last character
                        //new label found
                    else if (*it == CONSTANT_DOUBLE_QUOTE) {
                        //new element found in current dimension
                        dimSize[curDimIdx]++;
                        //update status
                        status = EParsingStatus::ParsingHeaderLabel;
                    }
                        //finished parsing current dimension header
                    else if (*it == CONSTANT_RIGHT_SQUARE_BRACKET) { status = EParsingStatus::ParsingHeader; }  //update status
                    else if (!std::isspace(*it, locale)) { return false; }
                    break;

                    //look for end of label (first '"' char not preceded by the '\' escape char)
                case EParsingStatus::ParsingHeaderLabel:

                    //found '"' char not preceded by escape char : end of label reached
                    if (*it == CONSTANT_DOUBLE_QUOTE && *(it - 1) != '\\') {
                        // We can only attach the label later after we know the size
                        labels.push_back(curString);

                        // std::cout << " lab " << curDimensionIdx << " " << dimSize[curDimensionIdx]-1 <<  " : " << curString << "\n";

                        //clear current string
                        curString.erase();

                        //update status
                        status = EParsingStatus::ParsingHeaderDimension;
                    }
                        //otherwise, keep parsing current label
                    else { curString.append(1, *it); }
                    break;

                case EParsingStatus::ParsingBuffer:

                    //comments starting
                    if (*it == CONSTANT_HASHTAG) { it = what.end() - 1; }   //ignore rest of line by skipping to last character
                        //going down one dimension
                    else if (*it == CONSTANT_LEFT_SQUARE_BRACKET) {
                        //update dimension index
                        curDimIdx++;
                        //ensure dimension count remains in allocated range
                        if (curDimIdx == matrix.getDimensionCount()) { return false; }
                        //ensure values count remains in allocated range
                        if (nValue[curDimIdx] == matrix.getDimensionSize(curDimIdx)) { return false; }
                        //increment values count for current dimension, if it is not the innermost
                        if (curDimIdx < matrix.getDimensionCount() - 1) { nValue[curDimIdx]++; }
                    }
                        //going up one dimension
                    else if (*it == CONSTANT_RIGHT_SQUARE_BRACKET) {
                        //if we are not in innermost dimension
                        if (curDimIdx < matrix.getDimensionCount() - 1) {
                            //ensure the right number of values was parsed in lower dimension
                            if (nValue[curDimIdx + 1] != matrix.getDimensionSize(curDimIdx + 1)) { return false; }
                            //reset values count of lower dimension to 0
                            nValue[curDimIdx + 1] = 0;
                        }
                            //ensure dimension count is correct
                        else if (curDimIdx == size_t(-1)) { return false; }

                        //go up one dimension
                        curDimIdx--;
                    }
                        //non whitespace character found
                    else if (!std::isspace(*it, locale)) {
                        //if we are in innermost dimension, assume a value is starting here
                        if (curDimIdx == matrix.getDimensionCount() - 1) {
                            //ensure values parsed so far in current dimension doesn't exceed current dimension size
                            if (nValue.back() == matrix.getDimensionSize(curDimIdx)) { return false; }

                            //increment values count found in innermost dimension
                            nValue[curDimIdx]++;

                            //append current character to current string
                            curString.append(1, *it);

                            //update status
                            status = EParsingStatus::ParsingBufferValue;
                        }
                        else { return false; }
                    }
                    break;

                    //look for end of value (first '"' char not preceded by the '\' escape char)
                case EParsingStatus::ParsingBufferValue:

                    //values end at first whitespace character or ']' character
                    if (std::isspace(*it, locale) == true || *it == CONSTANT_RIGHT_SQUARE_BRACKET) {
                        //if dimension closing bracket is found
                        if (*it == CONSTANT_RIGHT_SQUARE_BRACKET) {
                            //move back iterator by one character so that closing bracket is taken into account in EParsingStatus::ParsingBuffer case
                            --it;
                        }

                        //retrieve value
                        errno = 0;
                        char* end;
                        const double value = strtod(curString.c_str(), &end);
#if defined TARGET_OS_Windows
                        //string couldn't be converted to a double
                        if (errno == ERANGE) { return false; }
#endif
                        //store value in matrix
                        (matrix.getBuffer())[curElementIdx] = value;
                        //update element index
                        curElementIdx++;
                        //reset current string
                        curString.erase();
                        //update status
                        status = EParsingStatus::ParsingBuffer;
                    }
                        //otherwise, append current character to current string
                    else { curString.append(1, *it); }
                    break;
            } // switch(status)

            //increment iterator
            ++it;
        } // while(it != what.end()) (read each character of current line)
    } while (buffer.good()); //read each line in turn

    //If the file is empty or other (like directory)
    if (nValue.empty()) { return false; }
    //ensure the right number of values were parsed in first dimension
    if (nValue[0] != matrix.getDimensionSize(0)) { return false; }

    return true;
}

// A recursive helper function to spool matrix contents to a txt stringstream.
bool dumpMatrixBuffer(const CMatrix& matrix, std::stringstream& buffer, const size_t index1, size_t& index2)
{
    //are we in innermost dimension?
    if (index1 == matrix.getDimensionCount() - 1) {
        //dimension start
        for (size_t j = 0; j < index1; ++j) { buffer << CONSTANT_TAB; }
        buffer << CONSTANT_LEFT_SQUARE_BRACKET;

        //dump current cell contents
        for (size_t j = 0; j < matrix.getDimensionSize(index1); j++, index2++) { buffer << CONSTANT_SPACE << matrix.getBuffer()[index2]; }

        //dimension end
        buffer << CONSTANT_SPACE << CONSTANT_RIGHT_SQUARE_BRACKET << CONSTANT_EOL;
    }
    else {
        //dump all entries in current dimension
        for (size_t i = 0; i < matrix.getDimensionSize(index1); ++i) {
            //dimension start
            for (size_t j = 0; j < index1; ++j) { buffer << CONSTANT_TAB; }
            buffer << CONSTANT_LEFT_SQUARE_BRACKET << CONSTANT_EOL;

            dumpMatrixBuffer(matrix, buffer, index1 + 1, index2);

            //dimension end
            for (size_t j = 0; j < index1; ++j) { buffer << CONSTANT_TAB; }
            buffer << CONSTANT_RIGHT_SQUARE_BRACKET << CONSTANT_EOL;
        }
    }

    return true;
}

bool toString(const CMatrix& matrix, CString& str, const size_t precision /* = 6 */)
{
    std::stringstream buffer;

    buffer << std::scientific;
    buffer.precision(std::streamsize(precision));

    // Dump header

    //header start
    buffer << CONSTANT_LEFT_SQUARE_BRACKET << CONSTANT_EOL;

    //dump labels for each dimension
    for (size_t i = 0; i < matrix.getDimensionCount(); ++i) {
        buffer << CONSTANT_TAB << CONSTANT_LEFT_SQUARE_BRACKET;

        for (size_t j = 0; j < matrix.getDimensionSize(i); ++j) {
            buffer << CONSTANT_SPACE << CONSTANT_DOUBLE_QUOTE << matrix.getDimensionLabel(i, j) << CONSTANT_DOUBLE_QUOTE;
        }

        buffer << CONSTANT_SPACE << CONSTANT_RIGHT_SQUARE_BRACKET << CONSTANT_EOL;
    }

    //header end
    buffer << CONSTANT_RIGHT_SQUARE_BRACKET << CONSTANT_EOL;

    // Dump buffer using a recursive algorithm
    size_t idx = 0;
    dumpMatrixBuffer(matrix, buffer, 0, idx);

    str = buffer.str().c_str();

    return true;
}

bool loadFromTextFile(CMatrix& matrix, const CString& filename)
{
    std::ifstream dataFile;
    FS::Files::openIFStream(dataFile, filename.toASCIIString(), std::ios_base::in);
    if (!dataFile.is_open()) { return false; }

    std::stringstream buffer;

    buffer << dataFile.rdbuf();

    const bool res = fromString(matrix, CString(buffer.str().c_str()));

    dataFile.close();

    return res;
}

bool saveToTextFile(const CMatrix& matrix, const CString& filename, const size_t precision /* = 6 */)
{
    std::ofstream dataFile;
    FS::Files::openOFStream(dataFile, filename.toASCIIString(), std::ios_base::out | std::ios_base::trunc);
    if (!dataFile.is_open()) { return false; }

    CString str;

    if (!toString(matrix, str, precision)) { return false; }

    dataFile << str.toASCIIString();
    dataFile.close();

    return true;
}


bool copy(CMatrix& dst, const CMatrix& src)
{
    if (&dst == &src) { return true; }
    dst.copy(src);
    return true;
}

bool copyDescription(CMatrix& dst, const CMatrix& src)
{
    if (&dst == &src) { return true; }
    dst.copyDescription(src);
    return true;
}

bool copyContent(CMatrix& dst, const CMatrix& src)
{
    if (&dst == &src) { return true; }
    const size_t nElementIn  = src.getBufferElementCount();
    const size_t nElementOut = dst.getBufferElementCount();
    if (nElementOut != nElementIn) { return false; }
    dst.copyContent(src);
    return true;
}

bool clearContent(CMatrix& matrix)
{
    matrix.resetBuffer();
    return true;
}

bool isDescriptionSimilar(const CMatrix& src1, const CMatrix& src2, const bool checkLabels)
{
    if (&src1 == &src2) { return true; }
    return src1.isDescriptionEqual(src2, checkLabels);
}

bool isContentSimilar(const CMatrix& src1, const CMatrix& src2)
{
    if (&src1 == &src2) { return true; }
    return src1.isBufferEqual(src2);
}

bool isContentValid(const CMatrix& src, const bool checkNotANumber, const bool checkInfinity) { return src.isBufferValid(checkNotANumber, checkInfinity); }

}  // namespace Matrix

namespace MatrixManipulation {

bool copy(CMatrix& dst, const CMatrix& src)
{
    if (&dst == &src) { return true; }
    dst.copy(src);
    return true;
}

bool copyDescription(CMatrix& dst, const CMatrix& src)
{
    if (&dst == &src) { return true; }
    dst.copyDescription(src);
    return true;
}

bool copyContent(CMatrix& dst, const CMatrix& src)
{
    if (&dst == &src) { return true; }
    const size_t nElementIn  = src.getBufferElementCount();
    const size_t nElementOut = dst.getBufferElementCount();
    if (nElementOut != nElementIn) { return false; }
    dst.copyContent(src);
    return true;
}
bool clearContent(CMatrix& matrix)
{
    matrix.resetBuffer();
    return true;
}

}  // namespace MatrixManipulation
}  // namespace Toolkit
}  // namespace OpenViBE
