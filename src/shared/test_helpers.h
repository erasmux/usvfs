#pragma once

#include <stdexcept>
#include <filesystem>

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

  path path_of_test_bin(const path& relative_ = path());
  path path_of_test_temp(const path& relative_ = path());
  path path_of_usvfs_lib(const path& relative_ = path());

  std::string platform_dependant_executable(const char* name_, const char* ext_ = "exe", const char* platform_ = nullptr);

  // Recursively deletes the given path and all the files and directories under it
  // Use with care!!!
  void delete_directory_tree(const path& dpath);
};