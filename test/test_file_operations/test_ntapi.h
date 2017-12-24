#pragma once

#include "test_filesystem.h"

class TestNtApi : public TestFileSystem
{
public:
  TestNtApi(FILE* output) : TestFileSystem(output) {}

  path real_path(const char* abs_or_rel_path) override;

  FileInfoList list_directory(const path& directory_path) override;

  void create_path(const path& directory_path) override;

  void read_file(const path& file_path) override;

  void write_file(const path& file_path, const void* data, std::size_t size, bool overwrite) override;

  const char* id() override;

private:
  class SafeHandle;

  SafeHandle open_directory(const path& directory_path, bool create, bool allow_non_existence=false, long* pstatus=nullptr);
};
