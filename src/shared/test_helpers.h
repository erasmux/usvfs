#pragma once

#include <stdexcept>
#include <filesystem>
#include "windows_sane.h"

namespace test {

  class FuncFailed : public std::runtime_error
  {
  public:
    FuncFailed(const char* func)
      : std::runtime_error(msg(func)) {}
    FuncFailed(const char* func, unsigned long res)
      : std::runtime_error(msg(func, res)) {}
    FuncFailed(const char* func, const char* arg1)
      : std::runtime_error(msg(func, arg1)) {}
    FuncFailed(const char* func, const char* arg1, unsigned long res)
      : std::runtime_error(msg(func, arg1, res)) {}

  private:
    std::string msg(const char* func);
    std::string msg(const char* func, unsigned long res);
    std::string msg(const char* func, const char* arg1);
    std::string msg(const char* func, const char* arg1, unsigned long res);
  };

  class WinFuncFailed : public std::runtime_error
  {
  public:
    WinFuncFailed(const char* func)
      : std::runtime_error(msg(func)) {}
    WinFuncFailed(const char* func, unsigned long res)
      : std::runtime_error(msg(func, res)) {}
    WinFuncFailed(const char* func, const char* arg1)
      : std::runtime_error(msg(func, arg1)) {}
    WinFuncFailed(const char* func, const char* arg1, unsigned long res)
      : std::runtime_error(msg(func, arg1, res)) {}

  private:
    std::string msg(const char* func);
    std::string msg(const char* func, unsigned long res);
    std::string msg(const char* func, const char* arg1);
    std::string msg(const char* func, const char* arg1, unsigned long res);
  };

  using std::experimental::filesystem::path;

  // path functions assume they are called by a test executable
  // (calculate the requested path relative to the current executable path)

  path path_of_test_bin(const path& relative = path());
  path path_of_test_temp(const path& relative = path());
  path path_of_usvfs_lib(const path& relative = path());

  std::string platform_dependant_executable(const char* name, const char* ext = "exe", const char* platform = nullptr);

  // Recursively deletes the given path and all the files and directories under it
  // Use with care!!!
  void recursive_delete_files(const path& dpath);

  // Recursively copies all files and directories from srcPath to destPath
  void recursive_copy_files(const path& src_path, const path& dest_path, bool overwrite);

  class ScopedLoadLibrary {
  public:
    ScopedLoadLibrary(const wchar_t* dll_path);
    ~ScopedLoadLibrary();

    // returns zero if load library failed
    operator HMODULE() const { return m_mod; }

  private:
    HMODULE m_mod;
  };
};