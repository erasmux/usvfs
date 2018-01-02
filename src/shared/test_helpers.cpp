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

  path path_of_test_bin(const path& relative) {
    path base(winapi::wide::getModuleFileName(nullptr));
    return base.parent_path() / relative;
  }

  path path_of_test_temp(const path& relative) {
    return path_of_test_bin().parent_path() / "temp" / relative;
  }

  path path_of_usvfs_lib(const path& relative) {
    return path_of_test_bin().parent_path().parent_path() / "lib" / relative;
  }

  std::string platform_dependant_executable(const char* name, const char* ext, const char* platform)
  {
    std::string res = name;
    res += "_";
    if (platform)
      res += platform;
    else
#if _WIN64
      res += "x64";
#else
      res += "x86";
#endif
    res += ".";
    res += ext;
    return res;
  }

  void recursive_delete_files(const path& dpath)
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
        recursive_delete_files(dpath / r);
      if (!RemoveDirectoryW(dpath.c_str()))
        throw WinFuncFailed("RemoveDirectory", dpath.u8string().c_str());
    }
    if (winapi::ex::wide::fileExists(dpath.c_str()))
      throw FuncFailed("delete_directory_tree", dpath.u8string().c_str());
  }

  void recursive_copy_files(const path& src_path, const path& dest_path, bool overwrite)
  {
    bool srcIsDir = false, destIsDir = false;
    if (!winapi::ex::wide::fileExists(src_path.c_str(), &srcIsDir))
      throw FuncFailed("recursive_copy", ("source doesn't exist: " + src_path.u8string()).c_str());
    if (!winapi::ex::wide::fileExists(dest_path.c_str(), &destIsDir) && srcIsDir)
    {
      winapi::ex::wide::createPath(dest_path.c_str());
      destIsDir = true;
    }

    if (!srcIsDir)
      if (!destIsDir)
      {
        if (!CopyFileW(src_path.c_str(), dest_path.c_str(), overwrite))
          throw WinFuncFailed("CopyFile", (src_path.u8string() + " => " + dest_path.u8string()).c_str());
        return;
      }
      else
        throw FuncFailed("recursive_copy",
          ("source is a file but destination is a directory: " + src_path.u8string() + ", " + dest_path.u8string()).c_str());

    if (!destIsDir)
      throw FuncFailed("recursive_copy",
        ("source is a directory but destination is a file: " + src_path.u8string() + ", " + dest_path.u8string()).c_str());

    // source and destination are both directories:
    std::vector<std::wstring> recurse;
    for (auto f : winapi::ex::wide::quickFindFiles(src_path.c_str(), L"*"))
    {
      if (f.fileName == L"." || f.fileName == L"..")
        continue;
      if (f.attributes & FILE_ATTRIBUTE_DIRECTORY)
        recurse.push_back(f.fileName);
      else
        if (!CopyFileW((src_path / f.fileName).c_str(), (dest_path / f.fileName).c_str(), overwrite))
          throw WinFuncFailed("CopyFile",
            ((src_path / f.fileName).u8string() + " => " + (dest_path / f.fileName).u8string()).c_str());
    }
    for (auto r : recurse)
      recursive_copy_files(src_path / r, dest_path / r, overwrite);
  }

  ScopedLoadLibrary::ScopedLoadLibrary(const wchar_t* dll_path)
    : m_mod(LoadLibrary(dll_path))
  {
  }
  ScopedLoadLibrary::~ScopedLoadLibrary()
  {
    if (m_mod)
      FreeLibrary(m_mod);
  }

};