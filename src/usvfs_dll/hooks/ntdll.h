#pragma once

#include <ntdll_declarations.h>
#include <windows_sane.h>
#include <sstream>
#include <vector>
#include <dllimport.h>
#include <loghelpers.h>

namespace usvfs
{

DLLEXPORT NTSTATUS WINAPI
hook_NtQueryFullAttributesFile(POBJECT_ATTRIBUTES ObjectAttributes,
                          PFILE_NETWORK_OPEN_INFORMATION FileInformation);

DLLEXPORT NTSTATUS WINAPI
hook_NtQueryAttributesFile(POBJECT_ATTRIBUTES      ObjectAttributes,
                      PFILE_BASIC_INFORMATION FileInformation);

DLLEXPORT NTSTATUS WINAPI
hook_NtQueryDirectoryFile(HANDLE FileHandle,
                     HANDLE Event,
                     PIO_APC_ROUTINE ApcRoutine,
                     PVOID ApcContext,
                     PIO_STATUS_BLOCK IoStatusBlock,
                     PVOID FileInformation,
                     ULONG Length,
                     FILE_INFORMATION_CLASS FileInformationClass,
                     BOOLEAN ReturnSingleEntry,
                     PUNICODE_STRING FileName,
                     BOOLEAN RestartScan);

DLLEXPORT NTSTATUS WINAPI hook_NtOpenFile(PHANDLE FileHandle,
                                     ACCESS_MASK DesiredAccess,
                                     POBJECT_ATTRIBUTES ObjectAttributes,
                                     PIO_STATUS_BLOCK IoStatusBlock,
                                     ULONG ShareAccess,
                                     ULONG OpenOptions);

DLLEXPORT NTSTATUS WINAPI hook_NtCreateFile(PHANDLE FileHandle,
                                       ACCESS_MASK DesiredAccess,
                                       POBJECT_ATTRIBUTES ObjectAttributes,
                                       PIO_STATUS_BLOCK IoStatusBlock,
                                       PLARGE_INTEGER AllocationSize,
                                       ULONG FileAttributes,
                                       ULONG ShareAccess,
                                       ULONG CreateDisposition,
                                       ULONG CreateOptions,
                                       PVOID EaBuffer,
                                       ULONG EaLength);

DLLEXPORT NTSTATUS WINAPI hook_NtClose(HANDLE Handle);

DLLEXPORT NTSTATUS WINAPI hook_NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus);

} // namespace usvfs
