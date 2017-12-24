#pragma once

#include "test_helpers.h"
#include "winapi.h"
#include <string>
#include <fmt/format.h>


namespace test {

  std::string FuncFailed::msg(const char* func)
  {
    fmt::MemoryWriter msg;
    msg << func << "() failed";
    return msg.str();
  }

  std::string FuncFailed::msg(const char* func, unsigned long res)
  {
    fmt::MemoryWriter msg;
    msg << func << "() failed : result=" << res << " (0x" << fmt::hex(res) << ")";
    return msg.str();
  }

  std::string FuncFailed::msg(const char* func, const char* arg1, unsigned long res)
  {
    fmt::MemoryWriter msg;
    msg << func << "() failed : " << arg1 << ", result=" << res << " (0x" << fmt::hex(res) << ")";
    return msg.str();
  }

  std::string WinFuncFailed::msg(const char* func)
  {
    fmt::MemoryWriter msg;
    msg << func << "() failed : lastError=" << GetLastError();
    return msg.str();
  }

  std::string WinFuncFailed::msg(const char* func, unsigned long res)
  {
    fmt::MemoryWriter msg;
    msg << func << "() failed : result=" << res << " (0x" << fmt::hex(res) << "), lastError=" << GetLastError();
    return msg.str();
  }

  std::string WinFuncFailed::msg(const char* func, const char* arg1, unsigned long res)
  {
    fmt::MemoryWriter msg;
    msg << func << "() failed : " << arg1 << ", result=" << res << " (0x" << fmt::hex(res) << "), lastError=" << GetLastError();
    return msg.str();
  }

  path path_of_test_bin(path relative_) {
    path base(winapi::wide::getModuleFileName(nullptr));
    return base.parent_path() / relative_;
  }

  path path_of_test_temp(path relative_) {
    return path_of_test_bin().parent_path() / "temp" / relative_;
  }

  path path_of_usvfs_lib(path relative_) {
    return path_of_test_bin().parent_path().parent_path() / "lib" / relative_;
  }

  std::string platform_dependant_executable(const char* name_, const char* ext_, const char* platform_)
  {
    std::string res = name_;
    res += "_";
    if (platform_)
      res += platform_;
    else
#if _WIN64
      res += "x64";
#else
      res += "x86";
#endif
    res += ".";
    res += ext_;
    return res;
  }

};