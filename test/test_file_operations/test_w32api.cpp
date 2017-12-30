
#include "test_w32api.h"
#include <test_helpers.h>
#include <cstdio>
#include <cstring>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class TestW32Api::SafeHandle
{
public:
  SafeHandle(HANDLE handle = INVALID_HANDLE_VALUE) : m_handle(handle) {}
  SafeHandle(const SafeHandle&) = delete;
  SafeHandle(SafeHandle&& other) : m_handle(other.m_handle) { other.m_handle = INVALID_HANDLE_VALUE; }

  operator HANDLE() { return m_handle; }
  operator PHANDLE() { return &m_handle; }

  bool valid() const { return m_handle != INVALID_HANDLE_VALUE; }

  ~SafeHandle() {
    if (m_handle) {
      if (!CloseHandle(m_handle))
        std::fprintf(stderr, "CloseHandle failed : %d", GetLastError());
      m_handle = NULL;
    }
  }

private:
  HANDLE m_handle;
};

class TestW32Api::SafeFindHandle
{
public:
  SafeFindHandle(HANDLE handle = INVALID_HANDLE_VALUE) : m_handle(handle) {}
  SafeFindHandle(const SafeFindHandle&) = delete;
  SafeFindHandle(SafeFindHandle&& other) : m_handle(other.m_handle) { other.m_handle = INVALID_HANDLE_VALUE; }

  operator HANDLE() { return m_handle; }
  operator PHANDLE() { return &m_handle; }

  bool valid() const { return m_handle != INVALID_HANDLE_VALUE; }

  ~SafeFindHandle() {
    if (m_handle) {
      if (!FindClose(m_handle))
        std::fprintf(stderr, "FindClose failed : %d", GetLastError());
      m_handle = NULL;
    }
  }

private:
  HANDLE m_handle;
};

const char* TestW32Api::id()
{
  return "W32";
}

TestW32Api::path TestW32Api::real_path(const char* abs_or_rel_path)
{
  if (!abs_or_rel_path || !abs_or_rel_path[0])
    return path();

  char buf[1024];
  size_t res = GetFullPathNameA(abs_or_rel_path, _countof(buf), buf, NULL);
  if (!res || res >= _countof(buf))
    throw test::WinFuncFailed("GetFullPathName", res);
  return buf;
}

TestFileSystem::FileInfoList TestW32Api::list_directory(const path& directory_path)
{
  print_operation("Querying directory", directory_path);

  WIN32_FIND_DATA fd;
  SafeFindHandle find = FindFirstFileW((directory_path / L"*").c_str(), &fd);
  if (!find.valid())
    throw test::WinFuncFailed("FindFirstFileW");

  FileInfoList files;
  while (true)
  {
    files.push_back(FileInformation(fd.cFileName, fd.dwFileAttributes, fd.nFileSizeHigh*(MAXDWORD + 1) + fd.nFileSizeLow));
    if (!FindNextFileW(find, &fd))
      break;
  }

  return files;
}

void TestW32Api::create_path(const path& directory_path)
{
  // sanity and guaranteed recursion end:
  if (!directory_path.has_relative_path())
    throw std::runtime_error("Refusing to create non-existing top level path");

  print_operation("Checking directory", directory_path);

  DWORD attr = GetFileAttributesW(directory_path.c_str());
  if (attr != INVALID_FILE_ATTRIBUTES) {
    if (attr & FILE_ATTRIBUTE_DIRECTORY)
      return; // if directory already exists all is good
    else
      throw std::runtime_error("path exists but not a directory");
  }
  DWORD err = GetLastError();
  if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND)
    throw test::WinFuncFailed("GetFileAttributesW");

  print_operation("Creating directory", directory_path);

  if (err != ERROR_FILE_NOT_FOUND) // ERROR_FILE_NOT_FOUND means parent directory already exists
    create_path(directory_path.parent_path()); // otherwise create parent directory (recursively)

  if (!CreateDirectoryW(directory_path.c_str(), NULL))
    throw test::WinFuncFailed("CreateDirectoryW");
}

void TestW32Api::read_file(const path& file_path)
{
  print_operation("Reading file", file_path);

  SafeHandle file =
    CreateFile(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (!file.valid())
    throw test::WinFuncFailed("CreateFile");

  uint32_t total = 0;
  bool ends_with_newline = false;
  bool pending_prefix = true;
  while (true) {
    char buf[4096];

    DWORD read = 0;
    if (!ReadFile(file, buf, sizeof(buf), &read, NULL))
      throw test::WinFuncFailed("ReadFile");
    if (!read) // eof
      break;

    total += read;
    char* begin = buf;
    char* end = begin + read;
    while (begin != end) {
      if (pending_prefix) {
        if (output())
          fwrite(FILE_CONTENTS_PRINT_PREFIX, 1, strlen(FILE_CONTENTS_PRINT_PREFIX), output());
        pending_prefix = false;
      }
      char* print_end = reinterpret_cast<char*>(std::memchr(begin, '\n', end - begin));
      if (print_end) {
        pending_prefix = true;
        ++print_end; // also print the '\n'
      }
      else
        print_end = end;
      if (output())
        fwrite(begin, 1, print_end - begin, output());
      ends_with_newline = *(print_end - 1) == '\n';
      begin = print_end;
    }
  }
  if (output())
  {
    if (!ends_with_newline)
      fwrite("\n", 1, 1, output());
    fprintf(output(), "# Successfully read %u bytes.\n", total);
  }
}

void TestW32Api::write_file(const path& file_path, const void* data, std::size_t size, bool overwrite)
{
  print_operation(overwrite ? "Overwritting file" : "Rewriting file", file_path);

  ACCESS_MASK access = GENERIC_WRITE;
  if (overwrite) // Use read/write access when rewriting to "simulate" the harder case where it is not known if the file is going to actually be changed
    access |= GENERIC_READ;
  DWORD disposition = overwrite ? CREATE_ALWAYS : OPEN_EXISTING; // FILE_SUPERSEDE for the overwrite case should be relatively easy for usvfs to handle
                                                                 // as it leave no doubt we are replacing the old file (if such exists)

  SafeHandle file =
    CreateFile(file_path.c_str(), access, 0, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
  if (!file.valid())
    throw test::WinFuncFailed("CreateFile");

  if (!overwrite)
  {
    // in case we didn't overwrite the file, we need to truncate it:
    if (!SetEndOfFile(file))
      throw test::WinFuncFailed("SetEndOfFile");
  }

  // finally write the data:
  DWORD written = 0;
  if (!WriteFile(file, data, static_cast<DWORD>(size), &written, NULL))
    throw test::WinFuncFailed("WriteFile");

  if (output())
  {
    fprintf(output(), "# Successfully written %u bytes {", static_cast<unsigned>(written));
    fwrite(data, 1, size, output());
    fprintf(output(), "}\n");
  }
}
