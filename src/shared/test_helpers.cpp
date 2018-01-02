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

  void delete_directory_tree(const path& dpath)
  {
    bool isDir = false;
    if (!winapi::ex::wide::fileExists(dpath.c_str(), &isDir))
      return;
    if (!isDir) {
      if (!DeleteFileW(dpath.c_str()))
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
          if (!DeleteFileW((dpath / f.fileName).c_str()))
            throw WinFuncFailed("DeleteFile", (dpath / f.fileName).u8string().c_str());
      }
      for (auto r : recurse)
        delete_directory_tree(dpath / r);
      if (!RemoveDirectoryW(dpath.c_str()))
        throw WinFuncFailed("RemoveDirectory", dpath.u8string().c_str());
    }
    if (winapi::ex::wide::fileExists(dpath.c_str()))
      throw FuncFailed("delete_directory_tree", dpath.u8string().c_str());
  }

  void recursive_copy_files(const path& srcPath, const path& destPath, bool overwrite)
  {
    bool srcIsDir = false, destIsDir = false;
    if (!winapi::ex::wide::fileExists(srcPath.c_str(), &srcIsDir))
      throw FuncFailed("recursive_copy", ("source doesn't exist: " + srcPath.u8string()).c_str());
    if (!winapi::ex::wide::fileExists(destPath.c_str(), &destIsDir) && srcIsDir)
    {
      winapi::ex::wide::createPath(destPath.c_str());
      destIsDir = true;
    }

    if (!srcIsDir)
      if (!destIsDir)
      {
        if (!CopyFileW(srcPath.c_str(), destPath.c_str(), overwrite))
          throw WinFuncFailed("CopyFile", (srcPath.u8string() + " => " + destPath.u8string()).c_str());
        return;
      }
      else
        throw FuncFailed("recursive_copy",
          ("source is a file but destination is a directory: " + srcPath.u8string() + ", " + destPath.u8string()).c_str());

    if (!destIsDir)
      throw FuncFailed("recursive_copy",
        ("source is a directory but destination is a file: " + srcPath.u8string() + ", " + destPath.u8string()).c_str());

    // source and destination are both directories:
    std::vector<std::wstring> recurse;
    for (auto f : winapi::ex::wide::quickFindFiles(srcPath.c_str(), L"*"))
    {
      if (f.fileName == L"." || f.fileName == L"..")
        continue;
      if (f.attributes & FILE_ATTRIBUTE_DIRECTORY)
        recurse.push_back(f.fileName);
      else
        if (!CopyFileW((srcPath / f.fileName).c_str(), (destPath / f.fileName).c_str(), overwrite))
          throw WinFuncFailed("CopyFile",
            ((srcPath / f.fileName).u8string() + " => " + (destPath / f.fileName).u8string()).c_str());
    }
    for (auto r : recurse)
      recursive_copy(srcPath / r, destPath / r, overwrite);
  }

  ScopedLoadLibrary::ScopedLoadLibrary(const wchar_t* dllPath)
    : m_mod(LoadLibrary(dllPath))
  {
  }
  ScopedLoadLibrary::~ScopedLoadLibrary()
  {
    if (m_mod)
      FreeLibrary(m_mod);
  }

};