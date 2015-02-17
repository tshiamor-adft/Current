/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2014 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#include <algorithm>
#include <string>
#include <vector>

#include "file.h"

#include "../dflags/dflags.h"
#include "../3party/gtest/gtest-main-with-dflags.h"

using bricks::FileSystem;
using bricks::FileException;

DEFINE_string(file_test_tmpdir, ".noshit", "Local path for the test to create temporary files in.");

TEST(File, JoinPath) {
  EXPECT_EQ("foo/bar", FileSystem::JoinPath("foo", "bar"));
  EXPECT_EQ("foo/bar", FileSystem::JoinPath("foo/", "bar"));
  EXPECT_EQ("/foo/bar", FileSystem::JoinPath("/foo", "bar"));
  EXPECT_EQ("/foo/bar", FileSystem::JoinPath("/foo/", "bar"));
  EXPECT_EQ("/bar", FileSystem::JoinPath("foo", "/bar"));
  EXPECT_EQ("/bar", FileSystem::JoinPath("/foo", "/bar"));
  EXPECT_EQ("/bar", FileSystem::JoinPath("/", "bar"));
  EXPECT_EQ("/bar", FileSystem::JoinPath("/", "/bar"));
  EXPECT_EQ("bar", FileSystem::JoinPath("", "bar"));
  ASSERT_THROW(FileSystem::JoinPath("/foo/", ""), FileException);
}

TEST(File, GetFileExtension) {
  EXPECT_EQ("", FileSystem::GetFileExtension(""));
  EXPECT_EQ("", FileSystem::GetFileExtension("a"));
  EXPECT_EQ("b", FileSystem::GetFileExtension("a.b"));
  EXPECT_EQ("c", FileSystem::GetFileExtension("a.b.c"));
  EXPECT_EQ("a", FileSystem::GetFileExtension(".a"));
  EXPECT_EQ("a", FileSystem::GetFileExtension("..a"));
  EXPECT_EQ("", FileSystem::GetFileExtension("a/b"));
  EXPECT_EQ("b", FileSystem::GetFileExtension("a/.b"));
  EXPECT_EQ("", FileSystem::GetFileExtension("a/b/c"));
  EXPECT_EQ("c", FileSystem::GetFileExtension("a/b/.c"));
  EXPECT_EQ("a", FileSystem::GetFileExtension(".a"));
  EXPECT_EQ("", FileSystem::GetFileExtension("./a"));
  EXPECT_EQ("", FileSystem::GetFileExtension("../a"));
  EXPECT_EQ("a", FileSystem::GetFileExtension("../.a"));
  EXPECT_EQ("long_extension", FileSystem::GetFileExtension("long_name.long_extension"));
}

TEST(File, FileStringOperations) {
  const std::string fn = FileSystem::JoinPath(FLAGS_file_test_tmpdir, "tmp");

  FileSystem::RmFile(fn, FileSystem::RmFileParameters::Silent);
  ASSERT_THROW(FileSystem::RmFile(fn), FileException);
  ASSERT_THROW(FileSystem::ReadFileAsString(fn), FileException);

  FileSystem::WriteStringToFile(fn.c_str(), "PASSED");
  EXPECT_EQ("PASSED", FileSystem::ReadFileAsString(fn));

  FileSystem::WriteStringToFile(fn.c_str(), "ANOTHER");
  FileSystem::WriteStringToFile(fn.c_str(), " TEST", true);
  EXPECT_EQ("ANOTHER TEST", FileSystem::ReadFileAsString(fn));

  FileSystem::RmFile(fn);
  FileSystem::MkDir(fn);
  ASSERT_THROW(FileSystem::WriteStringToFile(fn.c_str(), "not so fast"), FileException);
  FileSystem::RmDir(fn);
}

TEST(File, GetFileSize) {
  const std::string fn = FileSystem::JoinPath(FLAGS_file_test_tmpdir, "size");

  FileSystem::RmFile(fn, FileSystem::RmFileParameters::Silent);
  ASSERT_THROW(FileSystem::GetFileSize(fn), FileException);

  FileSystem::WriteStringToFile(fn.c_str(), "four");
  EXPECT_EQ(4, FileSystem::GetFileSize(fn));
}

TEST(File, RenameFile) {
  const std::string fn1 = FileSystem::JoinPath(FLAGS_file_test_tmpdir, "one");
  const std::string fn2 = FileSystem::JoinPath(FLAGS_file_test_tmpdir, "two");

  FileSystem::RmFile(fn1, FileSystem::RmFileParameters::Silent);
  FileSystem::RmFile(fn2, FileSystem::RmFileParameters::Silent);
  ASSERT_THROW(FileSystem::RenameFile(fn1, fn2), FileException);

  FileSystem::WriteStringToFile(fn1.c_str(), "data");

  EXPECT_EQ("data", FileSystem::ReadFileAsString(fn1));
  ASSERT_THROW(FileSystem::ReadFileAsString(fn2), FileException);

  FileSystem::RenameFile(fn1, fn2);

  ASSERT_THROW(FileSystem::ReadFileAsString(fn1), FileException);
  EXPECT_EQ("data", FileSystem::ReadFileAsString(fn2));

  ASSERT_THROW(FileSystem::RenameFile(fn1, fn2), FileException);
}

TEST(File, DirOperations) {
  const std::string& dir = FileSystem::JoinPath(FLAGS_file_test_tmpdir, "dir");
  const std::string fn = FileSystem::JoinPath(dir, "file");

  FileSystem::RmFile(fn, FileSystem::RmFileParameters::Silent);
  FileSystem::RmDir(dir, FileSystem::RmDirParameters::Silent);

  ASSERT_THROW(FileSystem::WriteStringToFile(fn.c_str(), "test"), FileException);

  FileSystem::MkDir(dir);
  ASSERT_THROW(FileSystem::MkDir(dir), FileException);
  FileSystem::MkDir(dir, FileSystem::MkDirParameters::Silent);

  FileSystem::WriteStringToFile(fn.c_str(), "test");
  EXPECT_EQ("test", FileSystem::ReadFileAsString(fn));

  ASSERT_THROW(FileSystem::GetFileSize(dir), FileException);

  ASSERT_THROW(FileSystem::RmDir(dir), FileException);
  FileSystem::RmFile(fn);
  FileSystem::RmDir(dir);

  ASSERT_THROW(FileSystem::WriteStringToFile(fn.c_str(), "dir does not exist"), FileException);
}

TEST(File, ScopedRmFile) {
  const std::string fn = FileSystem::JoinPath(FLAGS_file_test_tmpdir, "tmp");

  FileSystem::RmFile(fn, FileSystem::RmFileParameters::Silent);
  ASSERT_THROW(FileSystem::ReadFileAsString(fn), FileException);
  {
    const auto file_remover = FileSystem::ScopedRmFile(fn);
    FileSystem::WriteStringToFile(fn.c_str(), "PASSED");
    EXPECT_EQ("PASSED", FileSystem::ReadFileAsString(fn));
  }
  ASSERT_THROW(FileSystem::ReadFileAsString(fn), FileException);
}

TEST(File, ScanDir) {
  const std::string& dir = FileSystem::JoinPath(FLAGS_file_test_tmpdir, "dir_to_scan");
  const std::string fn1 = FileSystem::JoinPath(dir, "one");
  const std::string fn2 = FileSystem::JoinPath(dir, "two");

  FileSystem::RmFile(fn1, FileSystem::RmFileParameters::Silent);
  FileSystem::RmFile(fn2, FileSystem::RmFileParameters::Silent);
  FileSystem::RmDir(dir, FileSystem::RmDirParameters::Silent);

  struct Scanner {
    explicit Scanner(const std::string& dir, bool return_code = true) : dir_(dir), return_code_(return_code) {}
    Scanner(const Scanner&) = delete;  // Make sure ScanDir()/ScanDirUntil() don't copy the argument.
    bool operator()(const std::string& file_name) {
      files_.push_back(
          std::make_pair(file_name, FileSystem::ReadFileAsString(FileSystem::JoinPath(dir_, file_name))));
      return return_code_;
    }
    const std::string dir_;
    const bool return_code_;
    std::vector<std::pair<std::string, std::string>> files_;
  };

  Scanner scanner_before(dir);
  ASSERT_THROW(FileSystem::ScanDir(dir, scanner_before), FileException);
  ASSERT_THROW(FileSystem::ScanDirUntil(dir, scanner_before), FileException);

  FileSystem::MkDir(dir);
  FileSystem::WriteStringToFile(fn1.c_str(), "foo");
  FileSystem::WriteStringToFile(fn2.c_str(), "bar");

  Scanner scanner_after(dir);
  FileSystem::ScanDir(dir, scanner_after);
  ASSERT_EQ(2u, scanner_after.files_.size());
  std::sort(scanner_after.files_.begin(), scanner_after.files_.end());
  EXPECT_EQ("one", scanner_after.files_[0].first);
  EXPECT_EQ("foo", scanner_after.files_[0].second);
  EXPECT_EQ("two", scanner_after.files_[1].first);
  EXPECT_EQ("bar", scanner_after.files_[1].second);

  Scanner scanner_after_until(dir, false);
  FileSystem::ScanDirUntil(dir, scanner_after_until);
  ASSERT_EQ(1u, scanner_after_until.files_.size());
  EXPECT_TRUE(scanner_after_until.files_[0].first == "one" || scanner_after_until.files_[0].first == "two");
  EXPECT_EQ((scanner_after_until.files_[0].first == "one" ? "foo" : "bar"),
            scanner_after_until.files_[0].second);
}