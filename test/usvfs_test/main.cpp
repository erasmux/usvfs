/*
Userspace Virtual Filesystem

Copyright (C) 2015 Sebastian Herbord. All rights reserved.

This file is part of usvfs.

usvfs is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

usvfs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with usvfs. If not, see <http://www.gnu.org/licenses/>.
*/

#include <test_helpers.h>
#include <iostream>
#include <gtest/gtest.h>
#include <inject.h>
#include <windows_sane.h>
#include <stringutils.h>
#include <spdlog.h>
#include <hookcontext.h>
#include <unicodestring.h>
#include <stringcast.h>
#include <hooks/kernel32.h>
#include <hooks/ntdll.h>
#include <usvfs.h>
#include <logging.h>
#include <filesystem>

using std::experimental::filesystem::path;

namespace spd = spdlog;
namespace uhooks = usvfs::hooks;
namespace ush = usvfs::shared;

class USVFSFullTest : public testing::Test
{
public:
  void SetUp() {
    USVFSParameters params;
    USVFSInitParameters(&params, "usvfs_test_fixture", true, LogLevel::Debug, CrashDumpsType::None, "");
    ConnectVFS(&params);
    SHMLogger::create("usvfs");
  }

  void TearDown() {
    DisconnectVFS();

    std::array<char, 1024> buffer;
    while (SHMLogger::instance().tryGet(buffer.data(), buffer.size())) {
      std::cout << buffer.data() << std::endl;
    }
    SHMLogger::free();
  }

private:
};

static LPCWSTR REAL_FILEW = L"C:/windows/notepad.exe";
static LPCWSTR REAL_DIRW = L"C:/windows/Logs";

TEST_F(USVFSFullTest, CannotCreateLinkToFileInNonexistantDirectory)
{
  EXPECT_EQ(FALSE, VirtualLinkFile(REAL_FILEW, L"c:/this_directory_shouldnt_exist/np.exe", FALSE));
}

TEST_F(USVFSFullTest, CanCreateMultipleLinks)
{
  static LPCWSTR outFile = LR"(C:\np.exe)";
  static LPCWSTR outDir  = LR"(C:\logs)";
  static LPCWSTR outDirCanonizeTest = LR"(C:\.\not/../logs\.\a\.\b\.\c\..\.\..\.\..\)";
  EXPECT_EQ(TRUE, VirtualLinkFile(REAL_FILEW, outFile, 0));
  EXPECT_EQ(TRUE, VirtualLinkDirectoryStatic(REAL_DIRW, outDir, 0));

  // both file and dir exist and have the correct type
  EXPECT_NE(INVALID_FILE_ATTRIBUTES, uhooks::GetFileAttributesW(outFile));
  EXPECT_NE(INVALID_FILE_ATTRIBUTES, uhooks::GetFileAttributesW(outDir));
  EXPECT_EQ(0UL, uhooks::GetFileAttributesW(outFile) & FILE_ATTRIBUTE_DIRECTORY);
  EXPECT_NE(0UL, uhooks::GetFileAttributesW(outDir)  & FILE_ATTRIBUTE_DIRECTORY);
  EXPECT_NE(0UL, uhooks::GetFileAttributesW(outDirCanonizeTest) & FILE_ATTRIBUTE_DIRECTORY);
}

#include <type_traits>
#include <iomanip>
template<int base, class value_t>
size_t count_digits_as_unsigned(value_t value)
{
  using unsigned_value_t = std::make_unsigned_t<value_t>;
  unsigned_value_t val = value;
  unsigned_value_t cmp = base;
  size_t res = 1;
  while (val >= cmp) {
    ++res;
    if (cmp <= std::numeric_limits<unsigned_value_t>::max() / base)
      cmp *= base;
    else
      break;
  }
  return res;
}

template<class value_t>
void count_digits(value_t val)
{
  if (val < 0)
    val = -val;
  std::cout << std::dec << (int) val << " has " << count_digits_as_unsigned<10>(val) << " digits" << std::endl;
  std::cout << std::hex << (int) val << " has " << count_digits_as_unsigned<16>(val) << " digits" << std::endl;
}

int main(int argc, char **argv) {
  using namespace test;

  using test_type = short;
  count_digits(static_cast<test_type>(0));
  count_digits(static_cast<test_type>(1));
  count_digits(static_cast<test_type>(9));
  count_digits(static_cast<test_type>(10));
  count_digits(static_cast<test_type>(15));
  count_digits(static_cast<test_type>(16));
  count_digits(static_cast<test_type>(25));
  count_digits(static_cast<test_type>(26));
  count_digits(static_cast<test_type>(99));
  count_digits(static_cast<test_type>(100));
  count_digits(static_cast<test_type>(127));
  count_digits(static_cast<test_type>(128));
  count_digits(static_cast<test_type>(255));
  count_digits(static_cast<test_type>(256));
  count_digits(static_cast<test_type>(999));
  count_digits(static_cast<test_type>(1000));
  count_digits(static_cast<test_type>(4095));
  count_digits(static_cast<test_type>(4096));
  count_digits(static_cast<test_type>(9999));
  count_digits(static_cast<test_type>(10000));
  count_digits(static_cast<test_type>(32767));
  count_digits(static_cast<test_type>(-32767));

  count_digits(static_cast<test_type>(-10000));
  count_digits(static_cast<test_type>(-9999));
  count_digits(static_cast<test_type>(-4096));
  count_digits(static_cast<test_type>(-4095));
  count_digits(static_cast<test_type>(-1000));
  count_digits(static_cast<test_type>(-999));
  count_digits(static_cast<test_type>(-256));
  count_digits(static_cast<test_type>(-255));

  count_digits(static_cast<test_type>(-128));
  count_digits(static_cast<test_type>(-127));
  count_digits(static_cast<test_type>(-100));
  count_digits(static_cast<test_type>(-99));
  count_digits(static_cast<test_type>(-26));
  count_digits(static_cast<test_type>(-25));
  count_digits(static_cast<test_type>(-16));
  count_digits(static_cast<test_type>(-15));
  count_digits(static_cast<test_type>(-10));
  count_digits(static_cast<test_type>(-9));
  count_digits(static_cast<test_type>(-1));
  count_digits(static_cast<test_type>(0));
  //count_digits(static_cast<test_type>(254));
  //count_digits(static_cast<test_type>(255));

  auto dllPath = path_of_usvfs_lib(platform_dependant_executable("usvfs", "dll"));
  HMODULE loadDll = LoadLibrary(dllPath.c_str());
  if (!loadDll) {
    std::wcerr << L"failed to load usvfs dll: " << dllPath.c_str() << L", " << GetLastError() << std::endl;
    return 1;
  }

  // note: this makes the logger available only to functions statically linked to the test binary, not those
  // called in the dll
  auto logger = spdlog::stdout_logger_mt("usvfs");
  logger->set_level(spdlog::level::warn);
  testing::InitGoogleTest(&argc, argv);
  int res = RUN_ALL_TESTS();

  FreeLibrary(loadDll);
  return res;
}
