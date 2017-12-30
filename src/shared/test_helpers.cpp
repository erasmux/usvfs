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

  std::string FuncFailed::msg(const char* func, const char* arg1)
  {
    fmt::MemoryWriter msg;
    msg << func << "() failed : " << arg1;
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

  std::string WinFuncFailed::msg(const char* func, const char* arg1)
  {
    fmt::MemoryWriter msg;
    msg << func << "() failed : " << arg1 << ", lastError=" << GetLastError();
    return msg.str();
  }

  std::string WinFuncFailed::msg(const char* func, const char* arg1, unsigned long res)
  {
    fmt::MemoryWriter msg;
    msg << func << "() failed : " << arg1 << ", result=" << res << " (0x" << fmt::hex(res) << "), lastError=" << GetLastError();
    return msg.str();
  }

  path path_of_test_bin(const path& relative_) {
    path base(winapi::wide::getModuleFileName(nullptr));
    return base.parent_path() / relative_;
  }

  path path_of_test_temp(const path& relative_) {
    return path_of_test_bin().parent_path() / "temp" / relative_;
  }

  path path_of_usvfs_lib(const path& relative_) {
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

  void clean_directory_tree(const path& dpath)
  {
    bool is_dir = false;
    if (!winapi::ex::wide::fileExists(dpath.c_str(), &is_dir))
      return;
    if (!is_dir) {
      if (!DeleteFile(dpath.c_str()))
        throw WinFuncFailed("DeleteFile", dpath.u8string().c_str());
    }
    else {
      // dpath exists and its a directory:
      std::vector<std::wstring> recurse;
      for (auto f : winapi::ex::wide::quickFindFiles(dpath.c_str(), L"*"))
      {
        if (f.fileName == L"." || f.fileName == L"..")
          continue;
        if (f.attributes & FILE_ATTRIBUTE_DIRECTORY)
          recurse.push_back(f.fileName);
        else
          if (!DeleteFile((dpath / f.fileName).c_str()))
            throw WinFuncFailed("DeleteFile", dpath.u8string().c_str());
      }
      for (auto r : recurse)
        clean_directory_tree(dpath / r);
    }
    if (winapi::ex::wide::fileExists(dpath.c_str()))
      throw FuncFailed("clean_directory_tree", dpath.u8string().c_str());
  }

};