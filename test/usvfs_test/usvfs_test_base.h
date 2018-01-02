#pragma once

#include <test_helpers.h>
#include <filesystem>
#include <string>

class usvfs_test_options {
public:
  using path = test::path;

  // fills any values not set (or set to an empty value) to their default value
  void fill_defaults();

  void set_ops_platform(const wchar_t* platform);
  void set_ops32() { if (opsexe.empty()) set_ops_platform(L"x86"); }
  void set_ops64() { if (opsexe.empty()) set_ops_platform(L"x64"); }
  void add_ops_options(const std::wstring& options) { if (!ops_options.empty()) ops_options += L" "; ops_options += options; }

  path opsexe;
  path fixture;
  path mapping;
  path temp;
  path mount;
  path source;
  path output;
  std::wstring ops_options;
  bool force_temp_cleanup = false;
};

class usvfs_test_base {
public:
  using path = test::path;

  // options object should outlive this object.
  usvfs_test_base(const usvfs_test_options& options) : m_o(options) {}
  virtual ~usvfs_test_base() = default;

  int run();

  // function for override:

  virtual const char* scenario_name() = 0;
  virtual bool scenario_run() = 0;

  // helpers for derived scenarios:

  virtual void ops_list(path rel_path, bool recursive, bool with_contents);
  virtual void ops_read(path rel_path);
  virtual void ops_rewrite(path rel_path, const wchar_t* contents);
  virtual void ops_overwrite(path rel_path, const wchar_t* contents, bool recursive);

  virtual void verify_mount_file(path rel_path, const wchar_t* contents);
  virtual void verify_mount_non_existance(path rel_path);
  virtual void verify_source_file(path rel_path, const wchar_t* contents);
  virtual void verify_source_non_existance(path rel_path);

private:
  void run_ops(std::wstring args);
  void verify_file(path file, const wchar_t* contents);
  void verify_non_existance(path apath);

  const usvfs_test_options& m_o;
};
