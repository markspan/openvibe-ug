#include <fs/Files.h>
#include <boost/filesystem.hpp>
#include <boost/version.hpp>

#include <gtest/gtest.h>

#define TEMP_OUTPUT_DIR TMP_DIR "/オッペﾝヴィベ"
#define TEMP_OUTPUT_DIR_COPY TMP_DIR "/オッペﾝヴィベ_Copy"
#define TEMP_OUTPUT_ASCII_FILE_PATH TEMP_OUTPUT_DIR "/file.txt"
#define TEMP_OUTPUT_UTF_FILE_PATH_COPY TEMP_OUTPUT_DIR_COPY "/日本語.txt"
#define TEMP_OUTPUT_ASCII_FILE_PATH_COPY TEMP_OUTPUT_DIR_COPY "/file.txt"


#define TEST_ASCII_FILE_PATH DATA_DIR "/オッペﾝヴィベ/file.txt"
#define TEST_UTF_FILE_PATH DATA_DIR "/オッペﾝヴィベ/日本語.txt"
#define TEST_ASCII_DIR DATA_DIR "/オッペﾝヴィベ"


TEST(FS_Files_Test_Directories_UTF, validateFileExists)
{
    ASSERT_TRUE(FS::Files::fileExists(TEST_ASCII_FILE_PATH));
    ASSERT_TRUE(FS::Files::fileExists(TEST_UTF_FILE_PATH));
    ASSERT_FALSE(FS::Files::fileExists(DATA_DIR "/オッペﾝヴィベ/file"));
    ASSERT_FALSE(FS::Files::fileExists(DATA_DIR "/オッペﾝヴィベ/日本語"));
}

TEST(FS_Files_Test_Directories_UTF, validateDirectoryExists)
{
    ASSERT_FALSE(FS::Files::directoryExists(DATA_DIR "/inexistent"));
    ASSERT_TRUE(FS::Files::directoryExists(DATA_DIR "/オッペﾝヴィベ"));
}

TEST(FS_Files_Test_Directories_UTF, validateGetParentPath)
{
    char parentPath[1024];
    ASSERT_TRUE(FS::Files::getParentPath(TEST_ASCII_FILE_PATH, parentPath));
    ASSERT_STREQ(DATA_DIR "/オッペﾝヴィベ", parentPath);
}

TEST(FS_Files_Test_Directories_UTF, validateGetFileName)
{
    char fileName[1024];
    FS::Files::getFilename(TEST_ASCII_FILE_PATH, fileName);
    ASSERT_STREQ("file.txt", fileName);
    FS::Files::getFilename(TEST_UTF_FILE_PATH, fileName);
    ASSERT_STREQ("日本語.txt", fileName);
}

TEST(FS_Files_Test_Directories_UTF, validateGetFileNameWithoutExtension)
{
    char fileName[1024];
    FS::Files::getFilenameWithoutExtension(TEST_ASCII_FILE_PATH, fileName);
    ASSERT_STREQ("file", fileName);
    FS::Files::getFilenameWithoutExtension(TEST_UTF_FILE_PATH, fileName);
    ASSERT_STREQ("日本語", fileName);
}

TEST(FS_Files_Test_Directories_UTF, validateGetFileNameExtension)
{
    char extension[1024];
    FS::Files::getFilenameExtension(TEST_ASCII_FILE_PATH, extension);
    ASSERT_STREQ(".txt", extension);
    FS::Files::getFilenameExtension(TEST_UTF_FILE_PATH, extension);
    ASSERT_STREQ(".txt", extension);
}

TEST(FS_Files_Test_Directories_UTF, validateCreatePath)
{
    FS::Files::removeAll(TEMP_OUTPUT_DIR);
    ASSERT_FALSE(FS::Files::directoryExists(TEMP_OUTPUT_DIR));
    ASSERT_TRUE(FS::Files::createPath(TEMP_OUTPUT_DIR));
    ASSERT_TRUE(FS::Files::directoryExists(TEMP_OUTPUT_DIR));
}

TEST(FS_Files_Test_Directories_UTF, validateCreateParentPath)
{
    FS::Files::removeAll(TEMP_OUTPUT_DIR);
    ASSERT_FALSE(FS::Files::directoryExists(TEMP_OUTPUT_DIR));
    ASSERT_TRUE(FS::Files::createParentPath(TEMP_OUTPUT_DIR "/file.txt"));
    ASSERT_TRUE(FS::Files::directoryExists(TEMP_OUTPUT_DIR));
}

#if BOOST_VERSION / 100 % 1000 >= 55
TEST(FS_Files_Test_Directories_UTF, validateCopyFile)
{
    FS::Files::removeAll(TEMP_OUTPUT_ASCII_FILE_PATH);
    FS::Files::createParentPath(TEMP_OUTPUT_ASCII_FILE_PATH);

    ASSERT_TRUE(FS::Files::copyFile(TEST_ASCII_FILE_PATH, TEMP_OUTPUT_ASCII_FILE_PATH));
    ASSERT_TRUE(FS::Files::fileExists(TEMP_OUTPUT_ASCII_FILE_PATH));
}

TEST(FS_Files_Test_Directories_UTF, validateCopyDirectory)
{
    ASSERT_TRUE(FS::Files::copyDirectory(TEST_ASCII_DIR, TEMP_OUTPUT_DIR_COPY));
    ASSERT_TRUE(FS::Files::fileExists(TEMP_OUTPUT_UTF_FILE_PATH_COPY));
    ASSERT_TRUE(FS::Files::fileExists(TEMP_OUTPUT_ASCII_FILE_PATH_COPY));
}

#endif

int uoFSFilesTestUTF(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);

    ::testing::GTEST_FLAG(filter) = "FS_Files_Test_Directories_UTF.*";
    return RUN_ALL_TESTS();
}
