
#include "test_filesystem.h"
#include <test_helpers.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

bool TestFileSystem::FileInformation::is_dir() const
{
  return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool TestFileSystem::FileInformation::is_file() const
{
  return (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

TestFileSystem::TestFileSystem(FILE* output)
  : m_output(output)
{}

TestFileSystem::path TestFileSystem::current_directory()
{
  DWORD res = GetCurrentDirectoryW(0, NULL);
  if (!res)
    throw test::WinFuncFailed("GetCurrentDirectory", res);
  std::wstring buf(res + 1,'\0');
  res = GetCurrentDirectoryW(buf.length(), &buf[0]);
  if (!res || res >= buf.length())
    throw test::WinFuncFailed("GetCurrentDirectory", res);
  buf.resize(res);
  return buf;
}

TestFileSystem::path TestFileSystem::relative_path(path full_path)
{
  auto rel_begin = full_path.begin();
  auto base_iter = m_basepath.begin();
  while (rel_begin != full_path.end() && base_iter != m_basepath.end() && *rel_begin == *base_iter) {
    ++rel_begin;
    ++base_iter;
  }

  if (base_iter != m_basepath.end()) // full_path is not a sub-folder of m_basepath
    return full_path;

  if (rel_begin == full_path.end())  // full_path == m_basepath
      return path(L".");

  // full_path is a sub-folder of m_basepath so take only relative path
  path result;
  for (; rel_begin != full_path.end(); ++rel_begin)
    result /= *rel_begin;
  return result;
}

void TestFileSystem::print_operation(const char* operation, path target)
{
  if (m_output)
    fprintf(m_output, "# (%s) %s {%s}\n", id(), operation, relative_path(target).u8string().c_str());
}
