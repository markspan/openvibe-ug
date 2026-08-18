///-------------------------------------------------------------------------------------------------
/// 
/// \file uoFilesTest.cpp
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

#include <string>

#include <fs/Files.h>

#include "ovtAssert.h"
#include <iostream>

int uoFilesTest(int /*argc*/, char* argv[])
{
    std::string outputDirectory = argv[1];

    OVT_ASSERT(FS::Files::directoryExists(outputDirectory.c_str()), "Failure to find test data directory");

    std::string testDir  = outputDirectory + "uoFilesTest";
    std::string testFile = testDir + "/uoFilesTest.txt";

    // test path creation

    FS::Files::createParentPath(testFile.c_str());
    OVT_ASSERT(FS::Files::directoryExists(testDir.c_str()), "Failure to create directory");
    OVT_ASSERT(!FS::Files::fileExists(testFile.c_str()), "Failure to create parent path only");

    // test path retrieval methods

    char parentPathFromFile[1024];
    FS::Files::getParentPath(testFile.c_str(), parentPathFromFile);
    OVT_ASSERT_STREQ(std::string(parentPathFromFile), testDir, "Failure to retrieve parent path from full path");

    char parentPathFromDir[1024];
    FS::Files::getParentPath(testDir.c_str(), parentPathFromDir);
    std::cout << parentPathFromDir << " " << outputDirectory << std::endl;
    OVT_ASSERT_STREQ((std::string(parentPathFromDir) + "/"), outputDirectory, "Failure to retrieve parent path from path with no slash");

    testDir += "/"; // adding a slash should now give testDir as parent
    FS::Files::getParentPath(testDir.c_str(), parentPathFromDir);
    OVT_ASSERT_STREQ((std::string(parentPathFromDir) + "/"), testDir, "Failure to retrieve parent path from path with slash");

    char filename[256];
    FS::Files::getFilename(testFile.c_str(), filename);
    OVT_ASSERT_STREQ(std::string(filename), std::string("uoFilesTest.txt"), "Failure to retrieve filename from full path");

    FS::Files::getFilenameWithoutExtension(testFile.c_str(), filename);
    OVT_ASSERT_STREQ(std::string(filename), std::string("uoFilesTest"), "Failure to retrieve filename with no extension from full path");

    // test file creation and opening

    std::ofstream ostream;
    FS::Files::openOFStream(ostream, testFile.c_str());
    OVT_ASSERT(FS::Files::fileExists(testFile.c_str()), "Failure to create file");
    OVT_ASSERT(ostream.is_open(), "Failure to open file");
    ostream.close();

    std::string testFileInMissingnDir = testDir + "/newDir/uoFilesTest.txt";
    FS::Files::openOFStream(ostream, testFileInMissingnDir.c_str());
    OVT_ASSERT(!FS::Files::fileExists(testFileInMissingnDir.c_str()), "Failure to check for non-existing file");

    std::ifstream istream;
    FS::Files::openIFStream(istream, testFile.c_str());
    OVT_ASSERT(istream.is_open(), "Failure to open file in an input stream");
    istream.close();

    std::fstream fstream;
    FS::Files::openFStream(fstream, testFile.c_str(), std::ios_base::out);
    OVT_ASSERT(fstream.is_open(), "Failure to open file in a generic stream");
    fstream.close();

    auto file = FS::Files::open(testFile.c_str(), "r");
    OVT_ASSERT(file != nullptr, "Failure to open file in a FILE object");

    file = FS::Files::open(testFileInMissingnDir.c_str(), "r");
    OVT_ASSERT(file == nullptr, "Failure to return NULL FILE object for non-existing file");

    testDir = outputDirectory + "uoFilesTest2/long spaced/path";
    FS::Files::createPath(testDir.c_str());
    OVT_ASSERT(FS::Files::directoryExists(testDir.c_str()), "Failure to create directory with path containing spaces");

    // test equality

    OVT_ASSERT(!FS::Files::equals(testFile.c_str(), testFileInMissingnDir.c_str()), "Failure to compare different files");
    OVT_ASSERT(FS::Files::equals(testFile.c_str(), testFile.c_str()), "Failure to compare same files");


    // test folder copy

    std::string testFile2       = outputDirectory + "uoFilesTest" + "/uoFilesTestChild/uoFilesTest.txt";
    std::string testTargetDir   = outputDirectory + "uoFilesTestCopy";
    std::string testTargetFile1 = testTargetDir + "/uoFilesTest.txt";
    std::string testTargetFile2 = testTargetDir + "/uoFilesTestChild/uoFilesTest.txt";
    // create a subfolder with file
    FS::Files::createParentPath(testFile2.c_str());
    std::ofstream ostream2;
    FS::Files::openOFStream(ostream2, testFile2.c_str());
    OVT_ASSERT(FS::Files::fileExists(testFile2.c_str()), "Failure to create file in subfolder");
    OVT_ASSERT(ostream2.is_open(), "Failure to open file");
    ostream2.close();
    // copy folder
    testDir = outputDirectory + "uoFilesTest";
    FS::Files::copyDirectory(testDir.c_str(), testTargetDir.c_str());
    OVT_ASSERT(FS::Files::directoryExists(testTargetDir.c_str()), "Failure in copying folder");
    OVT_ASSERT(FS::Files::fileExists(testTargetFile1.c_str()), "Failure in copying child files of the folder");
    OVT_ASSERT(FS::Files::fileExists(testTargetFile2.c_str()), "Failure in copying subfolder");
    // test folder copy on existing folder
    OVT_ASSERT(!FS::Files::copyDirectory(testDir.c_str(), testTargetDir.c_str()), "Failure: Copy should not have been done if folder exits");

    return EXIT_SUCCESS;
}
