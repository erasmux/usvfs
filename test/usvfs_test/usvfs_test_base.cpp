
#include "usvfs_test_base.h"

void usvfs_test_options::set_ops_platform(const wchar_t* platform)
{
}

void usvfs_test_options::fill_defaults()
{
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
