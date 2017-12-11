#pragma once

#include "dllimport.h"
#include "../usvfs_helper/usvfsparameters.h"
#include <atomic>

extern "C" {

/// Creates a dump file with current process executable and pid in the dumpPath
/// @return zero on successs
DLLEXPORT int WINAPI CrashCollectionCreateMiniDump(PEXCEPTION_POINTERS exceptionPtrs, CrashDumpsType type, const wchar_t* dumpPath);

/// Initialize the collection of crash dumps on unhandled exceptions.
/// Can also be called on an initialized CrashCollection module to update the parameters.
DLLEXPORT void WINAPI CrashCollectionInitialize(CrashDumpsType dumpType, const wchar_t* dumpPath);

/// Stop collecting crash dumps
DLLEXPORT void WINAPI CrashCollectionCleanup();

/// true iff this module is handling unhandled exceptions
DLLEXPORT bool WINAPI CrashCollectionRegisteredUnhandledExceptionFilter();

/// Sets an unhandled expection handler that this module will call (after collecting the crash dump).
/// @return If a previous handler was register returns the last one registered
DLLEXPORT LPTOP_LEVEL_EXCEPTION_FILTER WINAPI CrashCollectionSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER uehandler);

}