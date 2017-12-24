#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <cstdio>

class TestFileSystem
{
public:
  static constexpr char* FILE_CONTENTS_PRINT_PREFIX = "== ";

  typedef std::experimental::filesystem::path path;
  typedef std::FILE FILE;

  static path current_directory();

  TestFileSystem(FILE* output);

  void set_output(FILE* output) { m_output = output; }

  // base path used to trim outputs (which important so we can compare tests ran at different base paths)
  void set_basepath(const char* path) { m_basepath = real_path(path); }

  // returns the path relative to the base path
  path relative_path(path full_path);

  virtual const char* id() = 0;

  virtual path real_path(const char* abs_or_rel_path) = 0;

  struct FileInformation {
    std::wstring name;
    uint32_t attributes;
    uint64_t size;

    FileInformation(const std::wstring& iname, uint32_t iattributes, uint64_t isize)
      : name(iname), attributes(iattributes), size(isize)
    {}

    bool is_dir() const;
    bool is_file() const;
  };
  typedef std::vector<FileInformation> FileInfoList;

  virtual FileInfoList list_directory(const path& directory_path) = 0;

  virtual void create_path(const path& directory_path) = 0;

  virtual void read_file(const path& file_path) = 0;

  virtual void write_file(const path& file_path, const void* data, std::size_t size, bool overwrite) = 0;

protected:
  void print_operation(const char* operation, path target);
  FILE* output() { return m_output; }

private:
  FILE* m_output;
  path m_basepath;
};
