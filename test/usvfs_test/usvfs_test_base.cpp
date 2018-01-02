
#include "usvfs_test_base.h"
#include <winapi.h>
#include <usvfs.h>

void usvfs_test_options::fill_defaults(const path& test_name, const std::wstring& scenario)
{
  using namespace test;

#ifdef _WIN64
  set_ops64();
#else
  set_ops32();
#endif

  if (fixture.empty())
    fixture = path_of_test_bin(test_name / scenario);

  if (mapping.empty())
    mapping = fixture / L"vfs_mappings.txt";

  if (temp.empty())
    temp = path_of_test_temp(test_name / scenario);

  if (mount.empty()) {
    mount = temp / L"mount";
    temp_cleanup = true;
  }

  if (source.empty()) {
    source = temp / L"source";
    temp_cleanup = true;
  }

  if (output.empty()) {
    output = temp / test_name;
    output += ".txt";
  }
}

void usvfs_test_options::set_ops32()
{
  if (opsexe.empty())
    opsexe = test::path_of_test_bin(L"test_file_operations_x86.exe");
}

void usvfs_test_options::set_ops64()
{
  if (opsexe.empty())
    opsexe = test::path_of_test_bin(L"test_file_operations_x64.exe");
}

void usvfs_test_options::add_ops_options(const std::wstring& options)
{
  if (!ops_options.empty())
    ops_options += L" ";
  ops_options += options;
}

bool usvfs_test_base::cleanup_temp()
{
  using winapi::ex::wide::fileExists;

  bool isDir = false;
  if (!m_o.temp_cleanup || !fileExists(m_o.temp.c_str(), &isDir))
    return true;

  if (!isDir) {
    if (m_o.force_temp_cleanup) {
      // TODO
    }
  }

  // TODO
}

bool usvfs_test_base::copy_fixture()
{
  using namespace test;

  // TODO
}

bool usvfs_test_base::postmortem_check()
{
  // TODO: compare source with fixture source (no changes should be made to source)
  // TODO: compare mount with the postmortem mount
  return true;
}

int usvfs_test_base::run()
{
  return 0;
}

void usvfs_test_base::ops_list(path rel_path, bool recursive, bool with_contents)
{
}

void usvfs_test_base::ops_read(path rel_path)
{
}

void usvfs_test_base::ops_rewrite(path rel_path, const wchar_t* contents)
{
}

void usvfs_test_base::ops_overwrite(path rel_path, const wchar_t* contents, bool recursive)
{
}

void usvfs_test_base::verify_mount_file(path rel_path, const wchar_t* contents)
{
}

void usvfs_test_base::verify_mount_non_existance(path rel_path)
{
}

void usvfs_test_base::verify_source_file(path rel_path, const wchar_t* contents)
{
}

void usvfs_test_base::verify_source_non_existance(path rel_path)
{
}

void usvfs_test_base::run_ops(std::wstring args)
{
}

void usvfs_test_base::verify_file(path file, const wchar_t* contents)
{
}

void usvfs_test_base::verify_non_existance(path apath)
{
}
