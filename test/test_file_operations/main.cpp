#include <cstdio>
#include <stdexcept>
#include <winapi.h>
#include <fmt/format.h>
#include <test_helpers.h>
#include "test_ntapi.h"
#include "test_w32api.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using usvfs::shared::string_cast;
using usvfs::shared::CodePage;

void print_usage(const char* myname) {
  using namespace std;
  fprintf(stderr, "usage: %s [<options>] <command> [<command params>] ...\n", myname);
  fprintf(stderr, "options and commands are parsed and executed in the order they appear.\n");
  fprintf(stderr, "\nsupported commands:\n");
  fprintf(stderr, " -list <dir>         : lists the given directory and outputs the results.\n");
  fprintf(stderr, " -listcontents <dir> : lists the given directory, reading all files and outputs the results.\n");
  fprintf(stderr, " -read <file>        : reads the given file and outputs the results.\n");
  fprintf(stderr, " -overwrite <file> <string> : overwrites the file at the given path with the given contents (creating directories if necessary).\n");
  fprintf(stderr, " -rewrite <file> <string> : rewrites the file at the given path with the given contents (fails if file doesn't exist; uses read/write access).\n");
  fprintf(stderr, " -debug              : shows a message box and wait for a debugger to connect.\n");
  fprintf(stderr, "\nsupported options:\n");
  fprintf(stderr, " -out <file>         : file to log output to (use \"-\" for the stdout; otherwise path to output should exist).\n");
  fprintf(stderr, " -cout <file>        : similar to -out but does not log PID and other info which may change between runs.\n");
  fprintf(stderr, " -r                  : recursively list directories.\n");
  fprintf(stderr, " -r-                 : don't recursively list directories.\n");
  fprintf(stderr, " -basedir <dir>      : any paths under the basedir will outputed in a relative manner.\n");
  fprintf(stderr, " -w32api             : use regular Win32 API for file access (default).\n");
  fprintf(stderr, " -ntapi              : use lower level ntdll functions for file access.\n");
}

class CommandExecuter
{
public:
  CommandExecuter()
    : m_output(stdout)
    , m_api(&w32api)
  {
    set_basedir(TestFileSystem::current_directory().u8string().c_str());
  }

  ~CommandExecuter()
  {
    if (m_output && m_output != stdout)
      fclose(m_output);
  }

  FILE* output() const
  {
    return m_output;
  }

  bool file_output() const
  {
    return m_output != stdout;
  }

  // Options:

  void set_output(const char* output_file, bool clean, const char* cmdline)
  {
    if (m_output && m_output != stdout) {
      fprintf(m_output, "# Output log closed.\n", output_file);
      fclose(m_output);
    }

    m_cleanoutput = clean;
    m_output = nullptr;
    errno_t err = fopen_s(&m_output, output_file, "wb");
    if (err || !m_output)
      throw test::WinFuncFailed("fopen_s", output_file, err);
    else {
      if (m_cleanoutput)
        fprintf(m_output, "# Output log openned for: %s\n", cmdline);
      else
        fprintf(m_output, "# Output log openned for (pid %d): %s\n", GetCurrentProcessId(), cmdline);
      w32api.set_output(m_output);
      ntapi.set_output(m_output);
    }
  }

  bool cleanoutput() const
  {
    return m_cleanoutput;
  }

  void set_recursive(bool recursive)
  {
    m_recursive = recursive;
  }

  void set_basedir(const char* basedir)
  {
    w32api.set_basepath(basedir);
    ntapi.set_basepath(basedir);
  }

  void set_ntapi(bool enable)
  {
    if (enable)
      m_api = &ntapi;
    else
      m_api = &w32api;
  }

  // Commands:

  void list(const char* dir, bool read_files)
  {
    list_impl(m_api->real_path(dir), read_files);
  }

  void read(const char* path)
  {
    m_api->read_file(m_api->real_path(path));
  }

  void overwrite(const char* path, const char* value)
  {
    auto real = m_api->real_path(path);
    try {
      m_api->create_path(real.parent_path());
    }
    catch (const std::exception& e) {
      fmt::MemoryWriter msg;
      msg << "Failed to create_path [" << m_api->relative_path(real.parent_path()).u8string() << "] : " << e.what();
      throw std::runtime_error(msg.str());
    }
    m_api->write_file(real, value, strlen(value), true);
  }

  void rewrite(const char* path, const char* value)
  {
    m_api->write_file(m_api->real_path(path), value, strlen(value), false);
  }

  void debug()
  {
    if (!IsDebuggerPresent())
      MessageBoxA(NULL, "Connect a debugger and press OK to trigger a breakpoint", "DEBUG", 0);
    if (IsDebuggerPresent())
      __debugbreak();
  }

  // Traversal:

  void list_impl(TestFileSystem::path real, bool read_files)
  {
    std::vector<TestFileSystem::path> recurse;
    {
      auto files = m_api->list_directory(real);
      fprintf(m_output, ">> Listing directory {%s}:\n", m_api->relative_path(real).u8string().c_str());
      for (auto f : files) {
        if (f.is_dir()) {
          fprintf(m_output, "[%s] DIR (attributes 0x%x)\n",
            string_cast<std::string>(f.name, CodePage::UTF8).c_str(), f.attributes);
          if (m_recursive && f.name != L"." && f.name != L"..")
            recurse.push_back(real / f.name);
        }
        else {
          fprintf(m_output, "[%s] FILE (attributes 0x%x, %lld bytes)\n",
            string_cast<std::string>(f.name, CodePage::UTF8).c_str(), f.attributes, f.size);
          if (read_files)
            m_api->read_file(real / f.name);
        }
      }
    }
    for (auto r : recurse)
      list_impl(r, read_files);
  }

private:
  FILE* m_output;
  bool m_cleanoutput = false;
  bool m_recursive = false;

  TestFileSystem* m_api;
  static TestW32Api w32api;
  static TestNtApi ntapi;
};

//static
TestW32Api CommandExecuter::w32api(stdout);
TestNtApi CommandExecuter::ntapi(stdout);


bool verify_args_exist(const char* flag, int params, int index, int count)
{
  if (index + params >= count) {
    fprintf(stderr, "%s requires %d arguments\n", flag, params);
    return false;
  }
  else
    return true;
}

const char* UntouchedCommandLineArguments()
{
  const char* cmd = GetCommandLineA();
  for (; *cmd && *cmd != ' '; ++cmd)
  {
    if (*cmd == '"') {
      int escaped = 0;
      for (++cmd; *cmd && (*cmd != '"' || escaped % 2 != 0); ++cmd)
        escaped = *cmd == '\\' ? escaped + 1 : 0;
    }
  }
  while (*cmd == ' ') ++cmd;
  return cmd;
}

int main(int argc, char *argv[])
{
  bool found_commands = false;
  CommandExecuter executer;

  TestFileSystem::path exe_path = argv[0];
  std::string exename = exe_path.filename().u8string();
  std::string cmdline = exename + " " + UntouchedCommandLineArguments();
  fprintf(stdout, "# process %d started with commandline: %s\n", GetCurrentProcessId(), cmdline.c_str());

  for (int ai = 1; ai < argc; ++ai)
  {
    try
    {
      SetLastError(0);

      // options:
      if (strcmp(argv[ai], "-out") == 0 && verify_args_exist("-out", 1, ai, argc)
        || strcmp(argv[ai], "-cout") == 0 && verify_args_exist("-cout", 1, ai, argc) )
      {
        executer.set_output(argv[ai + 1], argv[ai][1]=='c', cmdline.c_str());
        ++ai;
      }
      else if (strcmp(argv[ai], "-r") == 0)
        executer.set_recursive(true);
      else if (strcmp(argv[ai], "-r-") == 0)
        executer.set_recursive(false);
      else if (strcmp(argv[ai], "-basedir") == 0 && verify_args_exist("-basedir", 1, ai, argc)) {
        executer.set_basedir(argv[++ai]);
      }
      else if (strcmp(argv[ai], "-w32api") == 0)
        executer.set_ntapi(false);
      else if (strcmp(argv[ai], "-ntapi") == 0)
        executer.set_ntapi(true);
      // commands:
      else if ((strcmp(argv[ai], "-list") == 0
        || strcmp(argv[ai], "-listcontents") == 0)
        && verify_args_exist("-list", 1, ai, argc))
      {
        bool contents = strcmp(argv[ai], "-listcontents") == 0;
        executer.list(argv[++ai], contents);
        found_commands = true;
      }
      else if (strcmp(argv[ai], "-read") == 0 && verify_args_exist("-read", 1, ai, argc))
      {
        executer.read(argv[++ai]);
        found_commands = true;
      }
      else if (strcmp(argv[ai], "-overwrite") == 0 && verify_args_exist("-overwrite", 2, ai, argc)) {
        executer.overwrite(argv[ai + 1], argv[ai + 2]);
        ++++ai;
        found_commands = true;
      }
      else if (strcmp(argv[ai], "-rewrite") == 0 && verify_args_exist("-rewrite", 2, ai, argc)) {
        executer.rewrite(argv[ai + 1], argv[ai + 2]);
        ++++ai;
        found_commands = true;
      }
      else if (strcmp(argv[ai], "-debug") == 0) {
        executer.debug();
      }
      else {
        if (executer.file_output())
          fprintf(executer.output(), "ERROR: ignoring invalid argument {%s}\n", argv[ai]);
        fprintf(stderr, "ERROR: ignoring invalid argument {%s}\n", argv[ai]);
      }
    }
#if 1 // just a convient way to not catch exception when debugging
    catch (const std::exception& e)
    {
      if (executer.file_output())
        fprintf(executer.output(), "ERROR: %hs\n", e.what());
      fprintf(stderr, "ERROR: %hs\n", e.what());
      return 1;
    }
    catch (...)
    {
      if (executer.file_output())
        fprintf(executer.output(), "ERROR: unknown exception");
      fprintf(stderr, "ERROR: unknown exception\n");
      return 1;
    }
#else
    catch (bool) {}
#endif
  }

  if (!found_commands) {
    print_usage(exename.c_str());
    return 2;
  }

  if (executer.file_output())
    if (executer.cleanoutput())
      fprintf(executer.output(), "%s ended properly.\n", exename.c_str());
    else
      fprintf(executer.output(), "%s ended properly in process %d.\n", exename.c_str(), GetCurrentProcessId());
  fprintf(stdout, "%s ended properly in process %d.\n", exename.c_str(), GetCurrentProcessId());

  return 0;
}
