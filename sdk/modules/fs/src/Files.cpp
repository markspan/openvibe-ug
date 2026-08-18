#include "Files.h"

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
 #include <stdio.h>   // For fopen
 #include <unistd.h>  //For access().
 #include <sys/stat.h>
 #include <sys/types.h>  // For stat().
 #include <cstdlib>
#elif defined TARGET_OS_Windows
#include "m_ConverterUtf8.h"
#include <Windows.h>
#else
#endif

#include <boost/filesystem.hpp>
#include <boost/version.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <cstdio>
#include <cstring>

namespace FS {

//  * 2006-08-30 YRD - Portability note : using namespace FS confuses windows platform SDK because it defines itself a 'boolean' type. Thus the following define to force the use of FS::boolean !

#if BOOST_VERSION / 100 % 1000 >= 55
/**
 * \brief Makes a recursive copy of source folder to target folder.
 *        Operation can fail in several cases:
 *          - target path exists
 *          - bad permission rights 
 * \param source    the source folder path
 * \param target    the destination folder path
 * \return true if succeeded
 * \return false if failed
 */
bool recursiveCopy(const boost::filesystem::path& source, const boost::filesystem::path& target)
{
    if (exists(target)) { return false; }

    if (is_directory(source))
    {
        if (!create_directories(target)) { return false; }
        for (boost::filesystem::directory_entry& item : boost::filesystem::directory_iterator(source))
        {
            // boost::filesystem::path overlods '/' operator !
            if (!recursiveCopy(item.path(), target / item.path().filename())) { return false; }
        }
    }
    else if (is_regular_file(source))
    {
        try { copy(source, target); }
        catch (...) { return false; }
    }
    else { return false; }
    return true;
}
#endif


#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS


FILE* Files::open(const char* file, const char* mode)
{
    FILE* fHandle = fopen(file, mode);
    return fHandle;
}

FILE* Files::popen(const char* file, const char* mode)
{
    FILE* fHandle = ::popen(file, mode);
    return fHandle;
}

void Files::openOFStream(std::ofstream& stream, const char* file, std::ios_base::openmode mode) { stream.open(file, mode); }
void Files::openIFStream(std::ifstream& stream, const char* file, std::ios_base::openmode mode) { stream.open(file, mode); }
void Files::openFStream(std::fstream& stream, const char* file, std::ios_base::openmode mode) { stream.open(file, mode); }

#elif defined TARGET_OS_Windows


FILE* Files::open(const char* file, const char* mode)
{
    FILE* fHandle;

    try
    {
        const std::wstring utf16FileName = Common::Converter::Utf8ToUtf16(file);
        const std::wstring utf16Mode     = Common::Converter::Utf8ToUtf16(mode);

        fHandle = _wfopen(utf16FileName.c_str(), utf16Mode.c_str());
    }
    catch (const std::logic_error&) { fHandle = fopen(file, mode); }

    return fHandle;
}

FILE* Files::popen(const char* file, const char* mode)
{
    FILE* fHandle;

    try
    {
        const std::wstring utf16FileName = Common::Converter::Utf8ToUtf16(file);
        const std::wstring utf16Mode     = Common::Converter::Utf8ToUtf16(mode);

        fHandle = _wpopen(utf16FileName.c_str(), utf16Mode.c_str());
    }
    catch (const std::logic_error&) { fHandle = popen(file, mode); }

    return fHandle;
}

template <class T>
void openStream(T& stream, const char* file, std::ios_base::openmode mode)
{
    try
    {
        const std::wstring utf16FileName = Common::Converter::Utf8ToUtf16(file);
        stream.open(utf16FileName.c_str(), mode);
    }
    catch (const std::logic_error&) { stream.open(file, mode); }
}

void Files::openOFStream(std::ofstream& stream, const char* file, const std::ios_base::openmode mode) { openStream<std::ofstream>(stream, file, mode); }
void Files::openIFStream(std::ifstream& stream, const char* file, const std::ios_base::openmode mode) { openStream<std::ifstream>(stream, file, mode); }
void Files::openFStream(std::fstream& stream, const char* file, const std::ios_base::openmode mode) { openStream<std::fstream>(stream, file, mode); }

#endif

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS

bool Files::equals(const char* pFile1, const char* pFile2)
{
    bool res=true;
    if(pFile1 && pFile2)
    {
        struct stat stat1;
        struct stat stat2;
        bool bStat1=!stat(pFile1, &stat1);
        bool bStat2=!stat(pFile2, &stat2);

        if(!bStat1 && !bStat2) { res=true; }
        else if(bStat1 && bStat2)
        {
            res=
                (stat1.st_dev==stat2.st_dev)&&
                (stat1.st_ino==stat2.st_ino)&&
                (stat1.st_size==stat2.st_size)&&
                (stat1.st_mtime==stat2.st_mtime);
        }
        else { res=false; }
    }
    return res;
}

#elif defined TARGET_OS_Windows

bool Files::equals(const char* pFile1, const char* pFile2)
{
    bool res = true;
    if (pFile1 && pFile2)
    {
        const HANDLE handle1 = ::CreateFile(pFile1, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                            FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        const HANDLE handle2 = ::CreateFile(pFile2, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                            FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (handle1 && handle2)
        {
            BY_HANDLE_FILE_INFORMATION stat1;
            BY_HANDLE_FILE_INFORMATION stat2;
            const BOOL bStat1 = GetFileInformationByHandle(handle1, &stat1);
            const BOOL bStat2 = GetFileInformationByHandle(handle2, &stat2);
            if (!bStat1 && !bStat2) { res = true; }
            else if (bStat1 && bStat2)
            {
                res = (stat1.dwVolumeSerialNumber == stat2.dwVolumeSerialNumber) && (stat1.nFileIndexHigh == stat2.nFileIndexHigh)
                      && (stat1.nFileIndexLow == stat2.nFileIndexLow) && (stat1.nFileSizeHigh == stat2.nFileSizeHigh)
                      && (stat1.nFileSizeLow == stat2.nFileSizeLow) && (stat1.ftLastWriteTime.dwHighDateTime == stat2.ftLastWriteTime.dwHighDateTime)
                      && (stat1.ftLastWriteTime.dwLowDateTime == stat2.ftLastWriteTime.dwLowDateTime);
            }
            else { res = false; }
            CloseHandle(handle1);
            CloseHandle(handle2);
        }
        else
        {
            if (handle1) { CloseHandle(handle1); }
            if (handle2) { CloseHandle(handle2); }
        }
    }
    return res;
}

#else

#endif

bool Files::fileExists(const char* pathToCheck)
{
    if (!pathToCheck) { return false; }
    FILE* fp = open(pathToCheck, "r");
    if (!fp) { return false; }
    fclose(fp);
    return true;
}

bool Files::directoryExists(const char* pathToCheck)
{
    if (!pathToCheck) { return false; }
#if defined TARGET_OS_Windows
    const std::wstring pathUTF16 = Common::Converter::Utf8ToUtf16(pathToCheck);
    const DWORD ftyp             = GetFileAttributesW(pathUTF16.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES) { return false; }
    if (ftyp & FILE_ATTRIBUTE_DIRECTORY) { return true; }
#endif
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    if ( access( pathToCheck, 0 ) == 0 )
    {
        struct stat status;
        stat( pathToCheck, &status );

        if ( status.st_mode & S_IFDIR ) { return true; }
    } 
#endif
    return false;
}

bool Files::createPath(const char* path)
{
    if (strcmp(path, "") == 0) { return false; }
#if defined TARGET_OS_Windows
    std::wstring pathUTF16 = Common::Converter::Utf8ToUtf16(path);
    create_directories(boost::filesystem::wpath(pathUTF16));
    return is_directory(boost::filesystem::wpath(pathUTF16));
#else
    return boost::filesystem::create_directories(boost::filesystem::path(path));
#endif
}

bool Files::createParentPath(const char* path)
{
    if (strcmp(path, "") == 0) { return false; }
#if defined TARGET_OS_Windows
    std::wstring pathUTF16 = Common::Converter::Utf8ToUtf16(path);
    return create_directories(boost::filesystem::wpath(pathUTF16).parent_path());
#else
    return boost::filesystem::create_directories(boost::filesystem::path(path).parent_path());
#endif
}

bool Files::getParentPath(const char* path, char* parentPath)
{
    if (!path || !parentPath) { return false; }
    strcpy(parentPath, boost::filesystem::path(path).parent_path().string().c_str());
    return true;
}

bool Files::getParentPath(const char* path, char* parentPath, const size_t size)
{
    if (!path || !parentPath) { return false; }
    strncpy(parentPath, boost::filesystem::path(path).parent_path().string().c_str(), size);
    return true;
}

bool Files::getFilename(const char* path, char* filename)
{
    if (!path || !filename) { return false; }
    strcpy(filename, boost::filesystem::path(path).filename().string().c_str());
    return true;
}

bool Files::getFilename(const char* path, char* filename, const size_t size)
{
    if (!path || !filename) { return false; }
    strncpy(filename, boost::filesystem::path(path).filename().string().c_str(), size);
    return true;
}

bool Files::getFilenameWithoutExtension(const char* path, char* filename)
{
    if (!path || !filename) { return false; }
    strcpy(filename, boost::filesystem::path(path).filename().replace_extension("").string().c_str());
    return true;
}

bool Files::getFilenameWithoutExtension(const char* path, char* filename, const size_t size)
{
    if (!path || !filename) { return false; }
    strncpy(filename, boost::filesystem::path(path).filename().replace_extension("").string().c_str(), size);
    return true;
}

bool Files::getFilenameExtension(const char* path, char* fileNameExtension)
{
    if (!path || !fileNameExtension) { return false; }
    strcpy(fileNameExtension, boost::filesystem::path(path).extension().string().c_str());
    return true;
}

bool Files::getFilenameExtension(const char* path, char* extension, const size_t size)
{
    if (!path || !extension) { return false; }
    strncpy(extension, boost::filesystem::path(path).extension().string().c_str(), size);
    return true;
}

bool Files::remove(const char* path)
{
    if (fileExists(path) || directoryExists(path))
    {
#if defined TARGET_OS_Windows
        const std::wstring pathUTF16 = Common::Converter::Utf8ToUtf16(path);
        return boost::filesystem::remove(boost::filesystem::wpath(pathUTF16.c_str()));
#else
        return boost::filesystem::remove(boost::filesystem::path(path));
#endif
    }
    return true;
}

bool Files::removeAll(const char* path)
{
    if (fileExists(path) || directoryExists(path))
    {
#if defined TARGET_OS_Windows
        const std::wstring pathUTF16 = Common::Converter::Utf8ToUtf16(path);
        return (remove_all(boost::filesystem::wpath(pathUTF16.c_str())) != 0);
#else
        return (boost::filesystem::remove_all(boost::filesystem::path(path)) != 0);
#endif
    }
    return true;
}


// old boost compliance
// manage cases here

#if BOOST_VERSION / 100 % 1000 >= 55

bool Files::copyFile(const char* srcFile, const char* dstPath)
{
    if (!srcFile || !dstPath) { return false; }
#if defined TARGET_OS_Windows
    std::wstring pathSourceUTF16      = Common::Converter::Utf8ToUtf16(srcFile);
    std::wstring pathDestinationUTF16 = Common::Converter::Utf8ToUtf16(dstPath);
    boost::filesystem::copy_file(pathSourceUTF16, pathDestinationUTF16);
#else
    boost::filesystem::copy_file(srcFile, dstPath);
#endif
    return true;
}

bool Files::copyDirectory(const char* srcDir, const char* dstDir)
{
    if (!srcDir || !dstDir) { return false; }
#if defined TARGET_OS_Windows
    const std::wstring pathSourceUTF16   = Common::Converter::Utf8ToUtf16(srcDir);
    const std::wstring pathTargetUTF16   = Common::Converter::Utf8ToUtf16(dstDir);
    const boost::filesystem::path source = boost::filesystem::wpath(pathSourceUTF16.c_str());
    const boost::filesystem::path target = boost::filesystem::wpath(pathTargetUTF16.c_str());
#else
    boost::filesystem::path source = srcDir;
    boost::filesystem::path target = dstDir;
#endif
    return recursiveCopy(source, target);
}

#elif defined TARGET_OS_Windows

#error OpenViBE requires at least boost 1.55 to compile on Windows

#else
// ugly hack for old boost on linux ...
bool Files::copyFile(const char* srcFile, const char* dstPath)
{
    if(!srcFile || !dstPath || FS::Files::fileExists(dstPath)) { return false; }
    std::string command = std::string("cp '") + srcFile + "' '" + dstPath+"'";  
    return (std::system(command.c_str()) != -1);
}

bool Files::copyDirectory(const char* srcDir, const char* dstDir)
{

    if(!srcDir || !srcDir || FS::Files::directoryExists(dstDir)) { return false; }
    std::string command = std::string("cp -r '") + srcDir + "' '" + dstDir+"'";     
    return (std::system(command.c_str()) != -1);    
}
#endif
}  // namespace FS
