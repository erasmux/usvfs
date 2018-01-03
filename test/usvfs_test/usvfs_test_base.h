#pragma once

#include <test_helpers.h>
#include <filesystem>
#include <string>

class usvfs_test_options {
public:
  static constexpr auto DEFAULT_MAPPING = L"vfs_mappings.txt";
  static constexpr auto MOUNT_DIR = L"mount";
  static constexpr auto SOURCE_DIR = L"source";

  using path = test::path;

  // fills any values not set (or set to an empty value) to their default value
  void fill_defaults(const path& test_name, const std::wstring& scenario);

  void set_ops32(); // sets opsexe iff opsexe is empty
  void set_ops64(); // sets opsexe iff opsexe is empty
  void add_ops_options(const std::wstring& options);

  path opsexe;
  path fixture;
  path mapping;
  path temp;
  path mount;
  path source;
  path output;
  path usvfs_log;
  std::wstring ops_options;
  bool temp_cleanup = false;
  bool force_temp_cleanup = false;
};

class usvfs_test_base {
public:
  static constexpr auto MOUNT_DIR = usvfs_test_options::MOUNT_DIR;
  static constexpr auto SOURCE_DIR = usvfs_test_options::SOURCE_DIR;
  static constexpr auto MOUNT_LABEL = "mount:";
  static constexpr auto SOURCE_LABEL = "source:";

  using path = test::path;

  // options object should outlive this object.
  usvfs_test_base(const usvfs_test_options& options) : m_o(options) {}
  virtual ~usvfs_test_base() = default;

  int run();

  // function for override:

  virtual const char* scenario_name() = 0;
  virtual bool scenario_run() = 0;

  // helpers for derived scenarios:

  virtual void ops_list(const path& rel_path, bool recursive, bool with_contents);
  virtual void ops_read(const path& rel_path);
  virtual void ops_rewrite(const path& rel_path, const char* contents);
  virtual void ops_overwrite(const path& rel_path, const char* contents, bool recursive);

  virtual void verify_mount_file(const path& rel_path, const char* contents);
  virtual void verify_mount_non_existance(const path& rel_path);
  virtual void verify_source_file(const path& rel_path, const char* contents);
  virtual void verify_source_non_existance(const path& rel_path);

private:
  void cleanup_temp();
  void copy_fixture();
  bool postmortem_check();

  test::ScopedFILE output();
  void run_ops(std::wstring preargs, const path& rel_path, const std::wstring& postargs = std::wstring());
  bool verify_file(const path& file, const char* contents);

  const usvfs_test_options& m_o;
};
