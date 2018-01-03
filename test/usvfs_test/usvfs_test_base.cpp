
#include "usvfs_test_base.h"
#include <winapi.h>
#include <stringcast.h>
#include <usvfs.h>
#include <vector>
#include <cctype>
#include <thread>
#include <future>
#include <chrono>
#include <iostream>

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
  using path = test::path;

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

  enum class map_type {
    none, // the mapping_reader uses this value but will never pass it to the usvfs_connector (which ignored such entries)
    dir,
    dircreate,
    file
  };

  struct mapping {
    mapping(map_type itype, std::wstring isource, std::wstring idestination)
      : type(itype), source(isource), destination(idestination) {}

    map_type type;
    path source;
    path destination;
  };

  using mappings_list = std::vector<mapping>;

  void updateMapping(const mappings_list& mappings, const usvfs_test_options& options, FILE* log)
  {
    using namespace std;
    using namespace usvfs::shared;

    fprintf(log, "Updating VFS mappings:\n");

    ClearVirtualMappings();

    for (const auto& map : mappings) {
      const string& source = usvfs_test_base::SOURCE_LABEL +
        test::path_as_relative(options.source, map.source).u8string();
      const string& destination = usvfs_test_base::MOUNT_LABEL +
        test::path_as_relative(options.mount, map.destination).u8string();
      switch (map.type)
      {
      case map_type::dir:
        fprintf(log, "  mapdir: %s => %s\n", source.c_str(), destination.c_str());
        VirtualLinkDirectoryStatic(map.source.c_str(), map.destination.c_str(), LINKFLAG_RECURSIVE);
        break;

      case map_type::dircreate:
        fprintf(log, "  mapdircreate: %s => %s\n", source.c_str(), destination.c_str());
        VirtualLinkDirectoryStatic(map.source.c_str(), map.destination.c_str(), LINKFLAG_CREATETARGET|LINKFLAG_RECURSIVE);
        break;

      case map_type::file:
        fprintf(log, "  mapfile: %s => %s\n", source.c_str(), destination.c_str());
        VirtualLinkFile(map.source.c_str(), map.destination.c_str(), LINKFLAG_RECURSIVE);
        break;
      }
    }

    fprintf(log, "\n");
  }

  static DWORD spawn(wchar_t* commandline)
  {
    using namespace usvfs::shared;

    STARTUPINFO si{ 0 };
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{ 0 };

    if (!CreateProcessHooked(NULL, commandline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
      throw_testWinFuncFailed("CreateProcessHooked", string_cast<std::string>(commandline, CodePage::UTF8).c_str());

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit = 99;
    if (!GetExitCodeProcess(pi.hProcess, &exit))
    {
      test::WinFuncFailedGenerator failed;
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
      throw failed("GetExitCodeProcess");
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exit;
  }

  void usvfs_logger()
  {
    fprintf(m_usvfs_log, "usvfs_test usvfs logger started:\n");
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

    fprintf(m_usvfs_log, "usvfs log closed.\n");
    m_usvfs_log.close();
  }

private:
  test::ScopedFILE m_usvfs_log;
  std::thread m_log_thread;
  std::promise<void> m_exit_signal;
  std::shared_future<void> m_exit_future;
};


// mappings_reader

class mappings_reader
{
public:
  using path = test::path;
  using string = std::string;
  using wstring = std::wstring;
  using map_type = usvfs_connector::map_type;
  using mapping = usvfs_connector::mapping;
  using mappings_list = usvfs_connector::mappings_list;

  mappings_reader(const path& mount_base, const path& source_base)
    : m_mount_base(mount_base), m_source_base(source_base)
  {
  }

  mappings_list read(const path& mapfile)
  {
    test::ScopedFILE map;
    errno_t err = _wfopen_s(map, mapfile.c_str(), L"rt");
    if (err || !map)
      throw_testWinFuncFailed("_wfopen_s", mapfile.u8string().c_str(), err);

    mappings_list mappings;

    char line[1024];
    while (!feof(map))
    {
      // read one line:
      if (!fgets(line, _countof(line), map))
        if (feof(map))
          break;
        else
          throw_testWinFuncFailed("fgets", "reading mappings");

      if (empty_line(line))
        continue;

      if (start_nesting(line, "mapdir"))
        m_nesting = map_type::dir;
      else if (start_nesting(line, "mapdircreate"))
        m_nesting = map_type::dircreate;
      else if (start_nesting(line, "mapfile"))
        m_nesting = map_type::file;
      else if (!isspace(*line)) // mapping sources should be indented and we already check all the mapping directives
        throw test::FuncFailed("mappings_reader::read", "invalid mappings file line", line);
      else {
        const auto& source_rel = trimmed_wide_string(line);
        mappings.push_back(mapping(m_nesting, (m_source_base / source_rel).wstring(), m_mount.wstring()));
      }
    }

    return mappings;
  }

  bool start_nesting(const char* line, const char* directive)
  {
    // check if line starts with directive and if so skip it:
    auto dlen = strlen(directive);
    auto after = line + dlen;
    if (strncmp(directive, line, dlen) == 0 && (!*after || isspace(*after)))
    {
      m_mount = m_mount_base;
      const auto& mount_rel = trimmed_wide_string(after);
      if (!mount_rel.empty())
        m_mount /= mount_rel;
      return true;
    }
    else
      return false;
  }

  static wstring trimmed_wide_string(const char* in)
  {
    while (std::isspace(*in)) ++in;
    auto end = in;
    end += strlen(end);
    while (end > in && std::isspace(*--end));
    return usvfs::shared::string_cast<wstring>(string(in, end), usvfs::shared::CodePage::UTF8);
  }

  static bool empty_line(const char* line) {
    for (; *line; ++line) {
      if (!std::isspace(*line))
        return false;
      if (*line == '#') // comment, ignore rest of line
        return true;
    }
    return true;
  }

private:
  path m_mount_base;
  path m_source_base;
  path m_mount;
  map_type m_nesting = map_type::none;
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

test::ScopedFILE usvfs_test_base::output()
{
  test::ScopedFILE log;
  errno_t err = _wfopen_s(log, m_o.output.c_str(), L"wt");
  if (err || !log)
    throw_testWinFuncFailed("_wfopen_s", m_o.output.u8string().c_str(), err);
  return log;
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
    // we read mappings first only because it is "non-destructive" but might raise an error if mappings invalid
    auto mappings = mappings_reader(m_o.mount, m_o.source).read(m_o.mapping);

    cleanup_temp();
    copy_fixture();

    usvfs_connector usvfs(m_o);
    {
      const auto& log = output();
      usvfs.updateMapping(mappings, m_o, log);

      fprintf(log, "running scenario %s:\n\n", scenario_name());
    }
    auto res = scenario_run();
    {
      const auto& log = output();
      if (res)
        fprintf(log, "\nscenario ended successfully!\n");
      else
        fprintf(log, "\nscenario failed miserably.\n");
    }

    if (!res)
      return 7;

    if (!postmortem_check())
      return 8;

    return 0;
  }
#if 1 // just a convient way to not catch exception when debugging
  catch (const exception& e)
  {
    try {
      wcerr << "ERROR: " << string_cast<std::wstring>(e.what(), CodePage::UTF8).c_str() << endl;
      fprintf(output(), "ERROR: %s\n", e.what());
    }
    catch (const exception& e) {
      wcerr << "ERROR^2: " << string_cast<std::wstring>(e.what(), CodePage::UTF8).c_str() << endl;
    }
    catch (...) {
      wcerr << "ERROR^2: unknown exception" << endl;
    }
  }
  catch (...)
  {
    try {
      wcerr << "ERROR: unknown exception" << endl;
      fprintf(output(), "ERROR: unknown exception\n");
    }
    catch (const exception& e) {
      wcerr << "ERROR^2: " << string_cast<std::wstring>(e.what(), CodePage::UTF8).c_str() << endl;
    }
    catch (...) {
      wcerr << "ERROR^2: unknown exception" << endl;
    }
  }
#else
  catch (bool) {}
#endif
  return 9; // exception
}

void usvfs_test_base::ops_list(const path& rel_path, bool recursive, bool with_contents)
{
  std::wstring cmd = recursive ? L"-r -list" : L"-list";
  if (with_contents)
    cmd += L"contents";
  run_ops(cmd, rel_path);
}

void usvfs_test_base::ops_read(const path& rel_path)
{
  run_ops(L"-read", rel_path);
}

void usvfs_test_base::ops_rewrite(const path& rel_path, const char* contents)
{
  using namespace usvfs::shared;
  run_ops(L"-rewrite", rel_path,
    L"\""+string_cast<std::wstring>(contents, CodePage::UTF8)+L"\"");
}

void usvfs_test_base::ops_overwrite(const path& rel_path, const char* contents, bool recursive)
{
  using namespace usvfs::shared;
  run_ops(recursive ? L"-r overwrite" : L"-overwrite", rel_path,
    L"\""+string_cast<std::wstring>(contents, CodePage::UTF8)+L"\"");
}

void usvfs_test_base::run_ops(std::wstring preargs, const path& rel_path, const std::wstring& postargs)
{
  using namespace usvfs::shared;
  using string = std::string;
  using wstring = std::wstring;

  string commandlog = test::path(m_o.opsexe).filename().u8string();
  wstring commandline = m_o.opsexe;
  if (commandline.find(' ') != wstring::npos && commandline.find('"') == wstring::npos) {
    commandline = L"\"" + commandline + L"\"";
    commandlog = "\"" + commandlog + "\"";
  }

  if (!m_o.ops_options.empty()) {
    commandline += L" ";
    commandline += m_o.ops_options;
    commandlog += " ";
    commandlog += string_cast<string>(m_o.ops_options, CodePage::UTF8);
  }

  commandline += L" -cout+ ";
  commandline += m_o.output;
  commandlog += " -cout+ ";
  commandlog += m_o.output.filename().u8string();

  if (!preargs.empty()) {
    commandline += L" ";
    commandline += preargs;
    commandlog += " ";
    commandlog += string_cast<string>(preargs, CodePage::UTF8);
  }

  if (!rel_path.empty()) {
    commandline += L" ";
    commandline += (m_o.mount / rel_path).wstring();
    commandlog += " ";
    commandlog += MOUNT_LABEL + rel_path.u8string();
  }

  if (!postargs.empty()) {
    commandline += L" ";
    commandline += postargs;
    commandlog += " ";
    commandlog += string_cast<string>(postargs, CodePage::UTF8);
  }

  fprintf(output(), "Spawning: %s", commandlog.c_str());
  auto res = usvfs_connector::spawn(&commandline[0]);

  if (res)
    throw test::FuncFailed("run_ops", commandlog.c_str(), res);
}

void usvfs_test_base::verify_mount_file(const path& rel_path, const char* contents)
{
  if (verify_file((m_o.mount / rel_path).c_str(), contents))
    throw test::FuncFailed("verify_mount_non_existance",
      (MOUNT_LABEL + rel_path.u8string()).c_str(), contents);
}

void usvfs_test_base::verify_source_file(const path& rel_path, const char* contents)
{
  if (verify_file((m_o.source / rel_path).c_str(), contents))
    throw test::FuncFailed("verify_source_file",
    (SOURCE_LABEL + rel_path.u8string()).c_str(), contents);
}

bool usvfs_test_base::verify_file(const path& file, const char* contents)
{
  // we allow difference in trailing whitespace (i.e. extra new line):

  size_t sz = strlen(contents);
  while (sz && isspace(contents[sz - 1])) --sz;

  const auto& real_contents = test::read_small_file(file);
  size_t real_sz = real_contents.size();
  while (real_sz && isspace(real_contents[real_sz - 1])) --real_sz;

  return sz == real_sz && memcmp(contents, real_contents.data(), sz);
}

void usvfs_test_base::verify_mount_non_existance(const path& rel_path)
{
  if (winapi::ex::wide::fileExists((m_o.mount / rel_path).c_str()))
    throw test::FuncFailed("verify_mount_non_existance",
      "path exists", (MOUNT_LABEL + rel_path.u8string()).c_str());
}

void usvfs_test_base::verify_source_non_existance(const path& rel_path)
{
  if (winapi::ex::wide::fileExists((m_o.source / rel_path).c_str()))
    throw test::FuncFailed("verify_source_non_existance",
      "path exists", (SOURCE_LABEL + rel_path.u8string()).c_str());
}
