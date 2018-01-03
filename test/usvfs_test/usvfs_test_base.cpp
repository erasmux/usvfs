
#include "usvfs_test_base.h"
#include <winapi.h>
#include <stringcast.h>
#include <usvfs.h>
#include <iostream>
#include <thread>
#include <future>
#include <chrono>

// usvfs_test_options class:

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
    mapping = fixture / DEFAULT_MAPPING;

  if (temp.empty())
    temp = path_of_test_temp(test_name / scenario);

  if (mount.empty()) {
    mount = temp / MOUNT_DIR;
    temp_cleanup = true;
  }

  if (source.empty()) {
    source = temp / SOURCE_DIR;
    temp_cleanup = true;
  }

  if (output.empty()) {
    output = temp / test_name;
    output += ".log";
  }

  if (output.empty()) {
    output = temp / test_name;
    output += ".log";
  }

  if (usvfs_log.empty()) {
    usvfs_log = temp / test_name;
    usvfs_log += "_usvfs.log";
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


// usvfs_connector helper class:

class usvfs_connector {
public:
  usvfs_connector(const usvfs_test_options& options)
    : m_exit_future(m_exit_signal.get_future())
  {
    errno_t err = _wfopen_s(m_usvfs_log, options.usvfs_log.c_str(), L"wt");
    if (err || !m_usvfs_log)
      throw_testWinFuncFailed("_wfopen_s", options.usvfs_log.u8string().c_str(), err);

    std::wcout << "Connecting VFS..." << std::endl;

    USVFSParameters params;
    USVFSInitParameters(&params, "usvfs_test", false, LogLevel::Debug, CrashDumpsType::None, "");
    InitLogging(false);
    CreateVFS(&params);

    m_log_thread = std::thread(&usvfs_connector::usvfs_logger, this);
  }

  ~usvfs_connector() {
    m_exit_signal.set_value();
    m_log_thread.join();
  }

  void usvfs_logger()
  {
    fprintf(m_usvfs_log, "usvfs log openned by usvfs_test");
    fflush(m_usvfs_log);

    constexpr size_t size = 1024;
    char buf[size + 1]{ 0 };
    int noLogCycles = 0;
    std::chrono::milliseconds wait_for;
    do {
      if (GetLogMessages(buf, size, false)) {
        fwrite(buf, 1, strlen(buf), m_usvfs_log);
        fwrite("\n", 1, 1, m_usvfs_log);
        fflush(m_usvfs_log);
        noLogCycles = 0;
      }
      else
        ++noLogCycles;
      if (noLogCycles)
        wait_for = std::chrono::milliseconds(std::min(40, noLogCycles) * 5);
      else
        wait_for = std::chrono::milliseconds(0);
    } while (m_exit_future.wait_for(wait_for) == std::future_status::timeout);
  }

private:
  test::ScopedFILE m_usvfs_log;
  std::thread m_log_thread;
  std::promise<void> m_exit_signal;
  std::shared_future<void> m_exit_future;
};


// usvfs_test_base class:

void usvfs_test_base::cleanup_temp()
{
  using namespace test;
  using namespace winapi::ex::wide;

  bool isDir = false;
  if (!m_o.temp_cleanup || !fileExists(m_o.temp.c_str(), &isDir))
    return;

  if (!isDir) {
    if (m_o.force_temp_cleanup)
      delete_file(m_o.temp);
    else
      throw FuncFailed("cleanup_temp", "temp exists but is a file", m_o.temp.u8string().c_str());
  }
  else {
    std::vector<std::wstring> cleanfiles;
    std::vector<std::wstring> cleandirs;
    bool other_dirs = false;
    for (auto f : quickFindFiles(m_o.temp.c_str(), L"*"))
      if (f.attributes & FILE_ATTRIBUTE_DIRECTORY == 0)
        cleanfiles.push_back(f.fileName);
      else if (f.fileName == SOURCE_DIR || f.fileName == MOUNT_DIR)
        cleandirs.push_back(f.fileName);
      else
        other_dirs = true;
    if (other_dirs || cleandirs.empty() && !cleanfiles.empty())
      throw FuncFailed("cleanup_temp", "temp exists but does not look like a temp dir (clean manually and rerun)", m_o.temp.u8string().c_str());
    std::wcout << "Cleaning previous temp dir: " << m_o.temp.c_str() << std::endl;
    Sleep(5000); // TODO: remove this after we verify this code works well
    for (auto f : cleanfiles)
      delete_file(m_o.temp / f);
    for (auto d : cleandirs)
      recursive_delete_files(m_o.temp / d);
  }
}

void usvfs_test_base::copy_fixture()
{
  using namespace test;
  using namespace winapi::ex::wide;

  path fmount = m_o.fixture / MOUNT_DIR;
  path fsource = m_o.fixture / SOURCE_DIR;

  bool isDir = false;
  if (!fileExists(fmount.c_str(), &isDir) || !isDir)
    throw FuncFailed("copy_fixture", "fixtures dir does not exist", fmount.u8string().c_str());
  if (!fileExists(fsource.c_str(), &isDir) || !isDir)
    throw FuncFailed("copy_fixture", "fixtures dir does not exist", fsource.u8string().c_str());
  if (fileExists(m_o.mount.c_str(), &isDir))
    throw FuncFailed("copy_fixture", "source dir already exists", m_o.mount.u8string().c_str());
  if (fileExists(m_o.source.c_str(), &isDir))
    throw FuncFailed("copy_fixture", "source dir already exists", m_o.source.u8string().c_str());

  std::wcout << "Copying fixture: " << m_o.fixture.c_str() << std::endl;
  recursive_copy_files(fmount, m_o.mount, false);
  recursive_copy_files(fsource, m_o.source, false);
}

bool usvfs_test_base::postmortem_check()
{
  // TODO: compare source with fixture source (no changes should be made to source)
  // TODO: compare mount with the postmortem mount
  return true;
}

int usvfs_test_base::run()
{
  using namespace usvfs::shared;
  using namespace std;

  try {
    cleanup_temp();
    copy_fixture();

    usvfs_connector usvfs(m_o);

    return postmortem_check() ? 0 : 7;
  }
#if 1 // just a convient way to not catch exception when debugging
  catch (const exception& e)
  {
    wcerr << "ERROR: " << string_cast<std::wstring>(e.what(), CodePage::UTF8).c_str() << endl;
  }
  catch (...)
  {
    wcerr << "ERROR: unknown exception" << endl;
  }
#else
  catch (bool) {}
#endif
  return 9; // exception
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
