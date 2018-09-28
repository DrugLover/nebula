/* Copyright (c) 2018 - present, VE Software Inc. All rights reserved
 *
 * This source code is licensed under Apache 2.0 License
 *  (found in the LICENSE.Apache file in the root directory)
 */

#include "base/Base.h"
#include <gtest/gtest.h>
#include "fs/FileUtils.h"

using namespace vesoft::fs;


TEST(FileUtils, getExePath) {
    static const char* kExePath = "common/fs/test/_build/file_utils_test";
    static const uint32_t kExePathLen = strlen(kExePath);

    auto path = FileUtils::getExePath();
    size_t pos = path.size() > kExePathLen ? path.size() - kExePathLen : 0;
    EXPECT_STREQ(kExePath, path.substr(pos).c_str());
}


TEST(FileUtils, joinPath) {
    EXPECT_EQ(std::string("./test.log"), FileUtils::joinPath("", "test.log"));
    EXPECT_EQ(std::string("/test.log"), FileUtils::joinPath("/", "test.log"));
    EXPECT_EQ(std::string("./test.log"), FileUtils::joinPath("./", "test.log"));
    EXPECT_EQ(std::string("/bin/test.log"), FileUtils::joinPath("/bin", "test.log"));
    EXPECT_EQ(std::string("/bin/test.log"), FileUtils::joinPath("/bin/", "test.log"));

    EXPECT_STREQ("./test.log", FileUtils::joinPath("", "test.log").c_str());
    EXPECT_STREQ("/test.log", FileUtils::joinPath("/", "test.log").c_str());
    EXPECT_STREQ("./test.log", FileUtils::joinPath("./", "test.log").c_str());
    EXPECT_STREQ("/bin/test.log", FileUtils::joinPath("/bin", "test.log").c_str());
    EXPECT_STREQ("/bin/test.log", FileUtils::joinPath("/bin/", "test.log").c_str());
}


TEST(FileUtils, dividePath) {
    folly::StringPiece parent;
    folly::StringPiece child;

    FileUtils::dividePath("JustName", parent, child);
    EXPECT_EQ(folly::StringPiece(), parent);
    EXPECT_EQ(std::string("JustName"), child);

    FileUtils::dividePath("/RootFile", parent, child);
    EXPECT_EQ(std::string("/"), parent);
    EXPECT_EQ(std::string("RootFile"), child);

    FileUtils::dividePath("./LocalFile", parent, child);
    EXPECT_EQ(std::string("."), parent);
    EXPECT_EQ(std::string("LocalFile"), child);

    FileUtils::dividePath("/RootDir/", parent, child);
    EXPECT_EQ(std::string("/"), parent);
    EXPECT_EQ(std::string("RootDir"), child);

    FileUtils::dividePath("./LocalDir/", parent, child);
    EXPECT_EQ(std::string("."), parent);
    EXPECT_EQ(std::string("LocalDir"), child);

    FileUtils::dividePath("/a/b/c/FullPathFile", parent, child);
    EXPECT_EQ(std::string("/a/b/c"), parent);
    EXPECT_EQ(std::string("FullPathFile"), child);
}


TEST(FileUtils, fileType) {
    EXPECT_EQ(FileType::CHAR_DEV, FileUtils::fileType("/dev/null"));
    EXPECT_STREQ("SoftLink",
                 FileUtils::getFileTypeName(FileUtils::fileType("/dev/stdin")));
}


TEST(FileUtils, removeFile) {
    // Create a temporary file
    char fileTemp[] = "/tmp/FileUtilsTest-TempFile.XXXXXX";
    int fd = mkstemp(fileTemp);
    ASSERT_GT(fd, 0);
    ASSERT_EQ(close(fd), 0);

    // Make sure the temp file exists
    struct stat st;
    ASSERT_EQ(lstat(fileTemp, &st), 0);

    // Let's remove it
    EXPECT_TRUE(FileUtils::remove(fileTemp));

    // Verify the temp file is gone
    ASSERT_LT(lstat(fileTemp, &st), 0);
    EXPECT_EQ(errno, ENOENT);
}


TEST(FileUtils, removeDirNonrecursively) {
    // Create a temp directory
    char dirTemp[] = "/tmp/FileUtilTest-TempDir.XXXXXX";
    ASSERT_NE(mkdtemp(dirTemp), nullptr);

    // Create two temp files
    char fileTemp[64];
    snprintf(fileTemp, sizeof(fileTemp), "%s/tempFile.XXXXXX", dirTemp);
    int fd = mkstemp(fileTemp);
    ASSERT_EQ(close(fd), 0);

    // Non-recursively removal should fail
    EXPECT_FALSE(FileUtils::remove(dirTemp));

    // Now let's remove the temp file
    EXPECT_TRUE(FileUtils::remove(fileTemp));

    // This time non-recursively removal should succeed
    EXPECT_TRUE(FileUtils::remove(dirTemp));

    // Verify the directory is gone
    struct stat st;
    EXPECT_LT(lstat(dirTemp, &st), 0);
    EXPECT_EQ(errno, ENOENT);
}


TEST(FileUtils, removeDirRecursively) {
    // Create a temp directory
    char dirTemp[] = "/tmp/FileUtilTest-TempDir.XXXXXX";
    ASSERT_NE(mkdtemp(dirTemp), nullptr);

    // Create two temp files
    char fileTemp[64];
    snprintf(fileTemp, sizeof(fileTemp), "%s/tempFile.XXXXXX", dirTemp);
    int fd = mkstemp(fileTemp);
    ASSERT_EQ(close(fd), 0);

    snprintf(fileTemp, sizeof(fileTemp), "%s/tempFile.XXXXXX", dirTemp);
    fd = mkstemp(fileTemp);
    ASSERT_EQ(close(fd), 0);

    // Create sub-directory
    char subDirTemp[64];
    snprintf(subDirTemp, sizeof(subDirTemp), "%s/subDir.XXXXXX", dirTemp);
    ASSERT_NE(mkdtemp(subDirTemp), nullptr);

    // Create couple files in the sub-directory
    snprintf(fileTemp, sizeof(fileTemp), "%s/tempFile.XXXXXX", subDirTemp);
    fd = mkstemp(fileTemp);
    ASSERT_EQ(close(fd), 0);

    snprintf(fileTemp, sizeof(fileTemp), "%s/tempFile.XXXXXX", subDirTemp);
    fd = mkstemp(fileTemp);
    ASSERT_EQ(close(fd), 0);

    // Recursively removal shold succeed
    EXPECT_TRUE(FileUtils::remove(dirTemp, true));

    // Verify the directory is gone
    struct stat st;
    EXPECT_LT(lstat(dirTemp, &st), 0);
    EXPECT_EQ(errno, ENOENT);
}


TEST(FileUtils, makeDir) {
    // Create a temp directory
    char dirTemp[] = "/tmp/FileUtilTest-mkdir.XXXXXX";
    ASSERT_NE(mkdtemp(dirTemp), nullptr);

    // Create sub-directory
    char subDirTemp[64];
    snprintf(subDirTemp, sizeof(subDirTemp), "%s/subdir.XXXXXX", dirTemp);
    // LOG(INFO) << subDirTemp;
    ASSERT_NE(mkdtemp(subDirTemp), nullptr);

    // Recursively remove the parent directory
    EXPECT_TRUE(FileUtils::remove(dirTemp, true));
    // Make sure the parent directory has gone
    ASSERT_EQ(FileType::NOTEXIST, FileUtils::fileType(dirTemp));

    // Now let's remake the directory
    EXPECT_TRUE(FileUtils::makeDir(std::string(subDirTemp)));
    // Verify the directory now exists
    EXPECT_EQ(FileType::DIRECTORY, FileUtils::fileType(subDirTemp));

    // Change to the newly created directory
    EXPECT_EQ(0, chdir(dirTemp));

    // create dir in cwd
    const char* subDirWithoutSlash = "testDir";
    EXPECT_TRUE(FileUtils::makeDir(subDirWithoutSlash));
    EXPECT_EQ(FileType::DIRECTORY, FileUtils::fileType(subDirWithoutSlash));

    // clean up
    EXPECT_TRUE(FileUtils::remove(dirTemp, true));
    // Verify everything is cleaned up
    EXPECT_EQ(FileType::NOTEXIST, FileUtils::fileType(dirTemp));
}


TEST(FileUtils, listContentInDir) {
    // Create a temp directory
    char dirTemp[] = "/tmp/FileUtilTest-listContent.XXXXXX";
    ASSERT_NE(mkdtemp(dirTemp), nullptr);

    // Create three sub-directories
    std::string subDir1 = FileUtils::joinPath(dirTemp, "subdir1");
    ASSERT_TRUE(FileUtils::makeDir(subDir1));
    std::string subDir2 = FileUtils::joinPath(dirTemp, "subdir2");
    ASSERT_TRUE(FileUtils::makeDir(subDir2));
    std::string subDir3 = FileUtils::joinPath(dirTemp, "subdir3");
    ASSERT_TRUE(FileUtils::makeDir(subDir3));

    // Create two regular files
    std::string fn1 = FileUtils::joinPath(dirTemp, "file1");
    FILE* f1 = fopen(fn1.c_str(), "w");
    ASSERT_NE(nullptr, f1);
    fwrite("file1", 5, 1, f1);
    ASSERT_EQ(0, fclose(f1));

    std::string fn2 = FileUtils::joinPath(dirTemp, "file2");
    FILE* f2 = fopen(fn2.c_str(), "w");
    ASSERT_NE(nullptr, f2);
    fwrite("file2", 5, 1, f2);
    ASSERT_EQ(0, fclose(f2));

    // Create two symbolic links
    ASSERT_EQ(0, symlink(subDir2.c_str(),
                         FileUtils::joinPath(dirTemp, "symlink1").c_str()));
    ASSERT_EQ(0, symlink(fn1.c_str(),
                         FileUtils::joinPath(dirTemp, "symlink2").c_str()));

    // List sym links
    auto symlinks = FileUtils::listAllTypedEntitiesInDir(dirTemp,
                                                         FileType::SYM_LINK,
                                                         false,
                                                         nullptr);
    EXPECT_EQ(2, symlinks.size());
    EXPECT_NE(symlinks.end(),
              std::find(symlinks.begin(), symlinks.end(), "symlink1"));
    EXPECT_NE(symlinks.end(),
              std::find(symlinks.begin(), symlinks.end(), "symlink2"));

    // List files
    auto files = FileUtils::listAllFilesInDir(dirTemp, true, nullptr);
    EXPECT_EQ(2, files.size());
    EXPECT_NE(files.end(), std::find(files.begin(), files.end(), fn1));
    EXPECT_NE(files.end(), std::find(files.begin(), files.end(), fn2));

    // List sub-directories with pattern matching
    auto dirs = FileUtils::listAllDirsInDir(dirTemp, true, "*dir2");
    EXPECT_EQ(1, dirs.size());
    EXPECT_NE(dirs.end(), std::find(dirs.begin(), dirs.end(), subDir2));

    // clean up
    EXPECT_TRUE(FileUtils::remove(dirTemp, true));
    // Verify everything is cleaned up
    EXPECT_EQ(FileType::NOTEXIST, FileUtils::fileType(dirTemp));
}


int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);

    return RUN_ALL_TESTS();
}
