
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
    throw_testWinFuncFailed("GetCurrentDirectory", res);
  std::wstring buf(res + 1,'\0');
  res = GetCurrentDirectoryW(buf.length(), &buf[0]);
  if (!res || res >= buf.length())
    throw_testWinFuncFailed("GetCurrentDirectory", res);
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

//static
const char* TestFileSystem::write_operation_name(write_mode mode)
{
  switch (mode) {
  case write_mode::manual_truncate:
    return "Writing file (by open & truncate)";
  case write_mode::truncate:
    return "Truncating file";
  case write_mode::create:
    return "Creating file";
  case write_mode::overwrite:
    return "Overwriting file";
  case write_mode::append:
    return "Appending file";
  }
  return "Unknown write operation?!";
}

void TestFileSystem::print_operation(const char* operation, path target)
{
  if (m_output)
    fprintf(m_output, "# (%s) %s {%s}\n", id(), operation, relative_path(target).u8string().c_str());
}

static inline void print_op_with_result(FILE* output, const char* prefix, const char* operation, uint32_t result, DWORD* last_error, const char* opt_arg)
{
  if (output) {
    fprintf(output, "%s%s", prefix, operation);
    if (opt_arg)
      fprintf(output, " %s", opt_arg);
    fprintf(output, " returned %u (0x%x)", result, result);
    if (last_error)
      fprintf(output, " last error %u (0x%x)", *last_error, *last_error);
    fprintf(output, "\n");
  }
}

void TestFileSystem::print_result(const char* operation, uint32_t result, bool with_last_error, const char* opt_arg)
{
  if (m_output)
  {
    DWORD last_error = with_last_error ? GetLastError() : 0;
    std::string prefix = "# ("; prefix += id(); prefix += ")   ";
    print_op_with_result(m_output, prefix.c_str(), operation, result, with_last_error ? &last_error : nullptr, opt_arg);
  }
}

void TestFileSystem::print_error(const char* operation, uint32_t result, bool with_last_error, const char* opt_arg)
{
  DWORD last_error = with_last_error ? GetLastError() : 0;
  print_op_with_result(stderr, "ERROR: ", operation, result, with_last_error ? &last_error : nullptr, opt_arg);
  if (m_output && m_output != stdout)
    print_op_with_result(m_output, "ERROR: ", operation, result, with_last_error ? &last_error : nullptr, opt_arg);
}

void TestFileSystem::print_write_success(const void* data, std::size_t size, std::size_t written)
{
  if (m_output)
  {
    fprintf(m_output, "# Successfully written %u bytes ", static_cast<unsigned>(written));
    // heuristics to print nicer one liners:
    if (size == 1 && reinterpret_cast<const char*>(data)[0] == '\n')
      fprintf(m_output, "<newline>");
    else {
      fprintf(m_output, "{");
      if (size && reinterpret_cast<const char*>(data)[size - 1] == '\n')
        --size;
      fwrite(data, 1, size, m_output);
      fprintf(m_output, "}");
    }
    fprintf(output(), "\n");
  }
}
