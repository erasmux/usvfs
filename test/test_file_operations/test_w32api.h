#pragma once

#include "test_filesystem.h"

class TestW32Api : public TestFileSystem
{
public:
  TestW32Api(FILE* output) : TestFileSystem(output) {}

  path real_path(const char* abs_or_rel_path) override;

  FileInfoList list_directory(const path& directory_path) override;

  void create_path(const path& directory_path) override;

  void read_file(const path& file_path) override;

  void write_file(const path& file_path, const void* data, std::size_t size, bool overwrite) override;

  const char* id() override;

private:
  class SafeHandle;
  class SafeFindHandle;
};
