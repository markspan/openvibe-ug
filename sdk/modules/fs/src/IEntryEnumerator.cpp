#include "IEntryEnumerator.h"

#include <string>
#include <cstring>
#include <stack>

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
 #include <glob.h>
 #include <sys/stat.h>
#elif defined TARGET_OS_Windows
#ifndef UNICODE
#define UNICODE
#endif
#include <Windows.h>
#include <codecvt>
#include <locale>
#else
#endif

#include <iostream>

namespace FS {
class CEntry final : public IEntryEnumerator::IEntry
{
public:

    explicit CEntry(const std::string& name) : m_Name(name) {}
    const char* getName() override { return m_Name.c_str(); }

    std::string m_Name;
};

class CAttributes final : public IEntryEnumerator::IAttributes
{
public:

    CAttributes() {}
    ~CAttributes() override {}
    bool isFile() override { return m_IsFile; }
    bool isDirectory() override { return m_IsDirectory; }
    bool isSymbolicLink() override { return m_IsSymbolicLink; }
    bool isArchive() override { return m_IsArchive; }
    bool isReadOnly() override { return m_IsReadOnly; }
    bool isHidden() override { return m_IsHidden; }
    bool isSystem() override { return m_IsSystem; }
    bool isExecutable() override { return m_IsExecutable; }
    size_t getSize() override { return m_Size; }

    bool m_IsFile         = false;
    bool m_IsDirectory    = false;
    bool m_IsSymbolicLink = false;
    bool m_IsArchive      = false;
    bool m_IsReadOnly     = false;
    bool m_IsHidden       = false;
    bool m_IsSystem       = false;
    bool m_IsExecutable   = false;
    size_t m_Size         = 0;
};

class CEntryEnumerator : public IEntryEnumerator
{
public:
    explicit CEntryEnumerator(IEntryEnumeratorCallBack& rEntryEnumeratorCallBack) : m_entryEnumeratorCB(rEntryEnumeratorCallBack) {}
    void release() override { delete this; }
protected:
    IEntryEnumeratorCallBack& m_entryEnumeratorCB;
};

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    class CEntryEnumeratorLinux final : public CEntryEnumerator
    {
    public:
        CEntryEnumeratorLinux(IEntryEnumeratorCallBack& rEntryEnumeratorCallBack) : CEntryEnumerator(rEntryEnumeratorCallBack) { }
        virtual bool enumerate(const char* sWildCard, bool bRecursive=false);
    };

#elif defined TARGET_OS_Windows
class CEntryEnumeratorWindows final : public CEntryEnumerator
{
public:
    explicit CEntryEnumeratorWindows(IEntryEnumeratorCallBack& rEntryEnumeratorCallBack) : CEntryEnumerator(rEntryEnumeratorCallBack) {}
    bool enumerate(const char* sWildCard, bool bRecursive = false) override;
};

#else
    class CEntryEnumeratorDummy : public CEntryEnumerator
    {
    public:
        explicit CEntryEnumeratorDummy(IEntryEnumeratorCallBack& rEntryEnumeratorCallBack) : CEntryEnumerator(rEntryEnumeratorCallBack) { }
        virtual bool enumerate(const char* sWildCard, bool bRecursive=false) { return !sWildCard ? false : true; }
    };

#endif

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS

bool CEntryEnumeratorLinux::enumerate(const char* sWildCard, bool bRecursive)
{
    if(!sWildCard) { return false; }

    glob_t globStruc;
    memset(&globStruc, GLOB_NOSORT, sizeof(globStruc));

    // Glob can retrn
    switch (glob(sWildCard, 0, nullptr, &globStruc))
    {
    case GLOB_NOSPACE:
    case GLOB_ABORTED:
        return false;
        break;
    case GLOB_NOMATCH:
        return true;
        break;
    default:
        break;
    }

    if(globStruc.gl_pathc<=0)
    {
        // Nothing found
        return true;
    }

    size_t i=0;
    bool finished=false;
    while(!finished)
    {
        if(i<globStruc.gl_pathc)
        {
            char* name=globStruc.gl_pathv[i];
            CEntry entry(name);
            CAttributes att;

            struct stat sL;
            struct stat s;
            if(!lstat(name, &sL) && !stat(name, &s))
            {
                att.m_IsDirectory    = S_ISDIR(s.st_mode) ? true : false;
                att.m_IsFile         = S_ISREG(s.st_mode) ? true : false;
                att.m_IsSymbolicLink = S_ISLNK(sL.st_mode) ? true : false;

                att.m_IsArchive    = false;
                att.m_IsReadOnly   = s.st_mode&S_IWUSR ? false : true;
                att.m_IsHidden     = false;
                att.m_IsSystem     = (S_ISBLK(s.st_mode)|S_ISFIFO(s.st_mode)|S_ISSOCK(s.st_mode)|S_ISCHR(s.st_mode)) ? true : false;
                att.m_IsExecutable = s.st_mode&S_IXUSR ? true : false;

                att.m_Size=s.st_size;

                // Sends to callback
                if(!m_entryEnumeratorCB.callback(entry, att)) { finished=true; }
            }
            i++;
        }
        else { finished = true; }
    }

    return true;
}

#elif defined TARGET_OS_Windows

bool CEntryEnumeratorWindows::enumerate(const char* sWildCard, bool bRecursive)
{
    if (!sWildCard || strlen(sWildCard) == 0) { return false; }

    wchar_t wildCardUtf16[1024];
    MultiByteToWideChar(CP_UTF8, 0, sWildCard, -1, wildCardUtf16, 1024);

    // $$$ TODO
    // $$$ Find better method to enumerate files
    // $$$ under windows including their initial path
    // $$$ (cFileName member of WIN32_FIND_DATA structure
    // $$$ loses the initial path !!)
    // $$$ TODO
    wchar_t extendedWildCard[1024];
    wchar_t* extendedWildCardFilename = nullptr;
    GetFullPathName(wildCardUtf16, 1024, extendedWildCard, &extendedWildCardFilename);
    std::wstring path(wildCardUtf16, wcslen(wildCardUtf16) - (extendedWildCardFilename ? wcslen(extendedWildCardFilename) : 0));

    std::stack<std::wstring> foldersToEnumerate;
    foldersToEnumerate.push(path);

    // if we need to recurse over subfolders, let's fetch all subfolders in foldersToEnumerate
    if (bRecursive)
    {
        std::stack<std::wstring> temporaryFolderSearchStack;
        temporaryFolderSearchStack.push(path);
        std::wstring currentSearchPath;
        while (! temporaryFolderSearchStack.empty())
        {
            currentSearchPath = temporaryFolderSearchStack.top();
            temporaryFolderSearchStack.pop();

            WIN32_FIND_DATA findData;
            HANDLE fileHandle;
            fileHandle = FindFirstFile((currentSearchPath + L"*").c_str(), &findData);
            if (fileHandle != INVALID_HANDLE_VALUE)
            {
                bool finished = false;
                while (!finished)
                {
                    if (std::wstring(findData.cFileName) != L"." && std::wstring(findData.cFileName) != L"..")
                    {
                        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                        {
                            foldersToEnumerate.push(currentSearchPath + findData.cFileName + L"/");
                            temporaryFolderSearchStack.push(currentSearchPath + findData.cFileName + L"/");
                        }
                    }

                    if (!FindNextFile(fileHandle, &findData)) { finished = true; }
                }
                FindClose(fileHandle);
            }
        }
    }
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    std::wstring currentPath;
    while (! foldersToEnumerate.empty())
    {
        currentPath = foldersToEnumerate.top();
        foldersToEnumerate.pop();

        WIN32_FIND_DATA findData;
        HANDLE fileHandle;
        fileHandle = FindFirstFile((currentPath + extendedWildCardFilename).c_str(), &findData);

        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            bool finished = false;
            while (!finished)
            {
                std::string entryName = converter.to_bytes(currentPath + findData.cFileName);
                CEntry entry(entryName);
                CAttributes attributes;

                attributes.m_IsDirectory    = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
                attributes.m_IsFile         = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? false : true;
                attributes.m_IsSymbolicLink = false;

                attributes.m_IsArchive    = (findData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) ? true : false;
                attributes.m_IsReadOnly   = (findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? true : false;
                attributes.m_IsHidden     = (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ? true : false;
                attributes.m_IsSystem     = (findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) ? true : false;
                attributes.m_IsExecutable = false; // TODO

                attributes.m_Size = (findData.nFileSizeHigh << 16) + findData.nFileSizeLow;

                // Sends to callback
                if (!m_entryEnumeratorCB.callback(entry, attributes)) { finished = true; }

                if (!FindNextFile(fileHandle, &findData)) { finished = true; }
            }
            FindClose(fileHandle);
        }
    }

    return true;
}

#endif

FS_API IEntryEnumerator* createEntryEnumerator(IEntryEnumeratorCallBack& rCallBack)
{
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    IEntryEnumerator* res = new CEntryEnumeratorLinux(rCallBack);
#elif defined TARGET_OS_Windows
    IEntryEnumerator* res = new CEntryEnumeratorWindows(rCallBack);
#else
    IEntryEnumerator* res = new CEntryEnumeratorDummy(rCallBack);
#endif
    return res;
}

}  // namespace FS
