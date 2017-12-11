#include "crashcollection.h"
#include <ttrampolinepool.h>
#include <winapi.h>
#include <atomic>
#include <Psapi.h>
#include <DbgHelp.h>

class CrashCollection {
public:
  static void Initialize(CrashDumpsType dumpType, const wchar_t* dumpPath)
  {
    if (!instance)
      instance = new CrashCollection(dumpType, dumpPath);
    else {
      // update parameter to existing instance taking case to preserve
      // the invariant that the UEFilter is registered iff dumpType != None
      bool was_registered = instance->registeredUE();
      instance->dmpType = dumpType;
      instance->dmpPath = dumpPath;
      if (was_registered && !instance->registeredUE())
        instance->unregisterUEFilter();
      if (!was_registered && instance->registeredUE())
        instance->registerUEFilter();
    }
  }

  static void Cleanup()
  {
    delete instance;
    instance = nullptr;
  }

  CrashCollection(CrashDumpsType dumpType, const wchar_t* dumpPath)
    : dmpType(dumpType), dmpPath(dumpPath)
  {
    if (registeredUE())
      registerUEFilter();
  }

  ~CrashCollection()
  {
    if (registeredUE())
      unregisterUEFilter();
  }

  bool registeredUE() {
    return dmpType != CrashDumpsType::None;
  }

  void registerUEFilter()
  {
    // assumes not already registerd!
    previousUnhandledExceptionHandler = ::SetUnhandledExceptionFilter(UEFilter);
  }

  void unregisterUEFilter()
  {
    // assumes already registed!
    ::SetUnhandledExceptionFilter(previousUnhandledExceptionHandler.exchange(nullptr));
  }

  LPTOP_LEVEL_EXCEPTION_FILTER WINAPI SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER uehandler) {
    return nextUnhandledExceptionHandler.exchange(uehandler);
  }

  static LONG WINAPI UEFilter(PEXCEPTION_POINTERS exceptionPtrs)
  {
    return instance ? instance->uefilter(exceptionPtrs) : EXCEPTION_CONTINUE_SEARCH;
  }

  LONG uefilter(PEXCEPTION_POINTERS exceptionPtrs)
  {
    // first we create the dump
    createDump(exceptionPtrs);

    // only then call the handlers registered after us:
    if (LPTOP_LEVEL_EXCEPTION_FILTER next = nextUnhandledExceptionHandler) {
      LONG res = next(exceptionPtrs);
      if (res != EXCEPTION_CONTINUE_SEARCH)
        return res;
    }

    // and finally be nice and call the handler we replaced:
    if (LPTOP_LEVEL_EXCEPTION_FILTER prev = previousUnhandledExceptionHandler)
      return prev(exceptionPtrs);
    else
      return EXCEPTION_CONTINUE_SEARCH;
  }

  void createDump(PEXCEPTION_POINTERS exceptionPtrs)
  {
    // Don't attempt to create a crash dump from a process more than once
    // (this should also protect us from recursively trying to generate
    // crash dumps when the CreateMiniDump is causing a fault)
    bool alreadyCrashed = false;
    if (!alreadyCrashDumped.compare_exchange_strong(alreadyCrashed, true))
      return;

    // disable our hooking mechanism to increase chances the dump writing won't crash
    HookLib::TrampolinePool& trampPool = HookLib::TrampolinePool::instance();
    if (&trampPool) {
      trampPool.forceUnlockBarrier();
      trampPool.setBlock(true);
    }

    CrashCollectionCreateMiniDump(exceptionPtrs, dmpType, dmpPath.c_str());

    // reenabled our hooking just in case the process continues to live
    if (&trampPool)
      trampPool.setBlock(false);
  }

  static CrashCollection* instance;

private:
  CrashDumpsType dmpType;
  std::wstring dmpPath;

  std::atomic<LPTOP_LEVEL_EXCEPTION_FILTER> previousUnhandledExceptionHandler{ nullptr };
  std::atomic<LPTOP_LEVEL_EXCEPTION_FILTER> nextUnhandledExceptionHandler{ nullptr };
  std::atomic<bool> alreadyCrashDumped{ false };
};

CrashCollection* CrashCollection::instance{ nullptr };

/// Collect crash dumps on unhandled exceptions
void WINAPI CrashCollectionInitialize(CrashDumpsType dumpType, const wchar_t* dumpPath)
{
  CrashCollection::Initialize(dumpType, dumpPath);
}

/// Stop collecting crash dumps
void WINAPI CrashCollectionCleanup()
{
  CrashCollection::Cleanup();
}

bool WINAPI CrashCollectionRegisteredUnhandledExceptionFilter()
{
  return CrashCollection::instance ? CrashCollection::instance->registeredUE() : false;
}

LPTOP_LEVEL_EXCEPTION_FILTER WINAPI CrashCollectionSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER uehandler)
{
  return CrashCollection::instance ? CrashCollection::instance->SetUnhandledExceptionFilter(uehandler) : nullptr;
}


// Mini Dump Generation

static std::wstring minidumpName(const wchar_t* dumpPath)
{
  DWORD pid = GetCurrentProcessId();
  wchar_t pname[100];
  if (GetModuleBaseNameW(GetCurrentProcess(), NULL, pname, _countof(pname)) == 0)
    return std::wstring();

  // find an available name:
  wchar_t dmpFile[MAX_PATH];
  int count = 0;
  _snwprintf_s(dmpFile, _TRUNCATE, L"%s\\%s-%lu.dmp", dumpPath, pname, pid);
  while (winapi::ex::wide::fileExists(dmpFile)) {
    if (++count > 99)
      return std::wstring();
    _snwprintf_s(dmpFile, _TRUNCATE, L"%s\\%s-%lu_%02d.dmp", dumpPath, pname, pid, count);
  }
  return dmpFile;
}

static int createMiniDumpImpl(PEXCEPTION_POINTERS exceptionPtrs, CrashDumpsType type, const wchar_t* dumpPath, HMODULE dbgDLL)
{
  typedef BOOL(WINAPI *FuncMiniDumpWriteDump)(HANDLE process, DWORD pid, HANDLE file, MINIDUMP_TYPE dumpType,
    const PMINIDUMP_EXCEPTION_INFORMATION exceptionParam,
    const PMINIDUMP_USER_STREAM_INFORMATION userStreamParam,
    const PMINIDUMP_CALLBACK_INFORMATION callbackParam);

  // Notice as this is called for unhandled exceptions we should keep the operations we
  // execute to a minimum to reduce the chance of either deadlocks or another fault
  // (which may cause this function be called again).
  // Hence we should avoid the usvfs logging system here.
  // Another reason not to use the usvfs logging is that this function may also be called
  // as a utility function (and the caller might use his own logging system).

  winapi::ex::wide::createPath(dumpPath);

  auto dmpName = minidumpName(dumpPath);
  if (dmpName.empty())
    return 4;

  FuncMiniDumpWriteDump funcDump = reinterpret_cast<FuncMiniDumpWriteDump>(GetProcAddress(dbgDLL, "MiniDumpWriteDump"));
  if (!funcDump)
    return 5;

  HANDLE dumpFile = winapi::wide::createFile(dmpName).createAlways().access(GENERIC_WRITE).share(FILE_SHARE_WRITE)();
  if (dumpFile != INVALID_HANDLE_VALUE) {
    DWORD dumpType = MiniDumpNormal | MiniDumpWithHandleData | MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData;
    if (type == CrashDumpsType::Data)
      dumpType |= MiniDumpWithDataSegs;
    if (type == CrashDumpsType::Full)
      dumpType |= MiniDumpWithFullMemory;

    _MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
    exceptionInfo.ThreadId = GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = exceptionPtrs;
    exceptionInfo.ClientPointers = FALSE;

    BOOL success =
      funcDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFile, static_cast<MINIDUMP_TYPE>(dumpType), &exceptionInfo, nullptr, nullptr);

    CloseHandle(dumpFile);

    return success ? 0 : 7;
  }
  else
    return 6;
}

int WINAPI CrashCollectionCreateMiniDump(PEXCEPTION_POINTERS exceptionPtrs, CrashDumpsType type, const wchar_t* dumpPath)
{
  if (type == CrashDumpsType::None)
    return 0;

  int res = 1;
  if (HMODULE dbgDLL = LoadLibraryW(L"dbghelp.dll"))
  {
    try {
      res = createMiniDumpImpl(exceptionPtrs, type, dumpPath, dbgDLL);
    }
    catch (...) {
      res = 2;
    }
    FreeLibrary(dbgDLL);
  }
  return res;
}
