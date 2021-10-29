#include "core.h"


BOOL SafeCopy(PVOID dest, PVOID src, SIZE_T size)
{
	SIZE_T returnSize = 0;
	if (NT_SUCCESS(MmCopyVirtualMemory(PsGetCurrentProcess(), src, PsGetCurrentProcess(), dest, size, KernelMode, &returnSize)) && returnSize == size) 
	{
		return TRUE;
	}

	return FALSE;
}


NTSTATUS core::write_memory(int pid, void* addr, void* value, size_t size)
{
	PEPROCESS pe;
	SIZE_T bytes;
	NTSTATUS status = PsLookupProcessByProcessId((HANDLE)pid, &pe);
	if (NT_SUCCESS(status))
	{
		MmCopyVirtualMemory(PsGetCurrentProcess(), value, pe, addr, size, KernelMode, &bytes);
		ObfDereferenceObject(pe);
	}
	return status;
}

NTSTATUS core::read_memory(int pid, void* addr, void* value, size_t size)
{
	PEPROCESS pe;
	SIZE_T bytes;
	NTSTATUS status = PsLookupProcessByProcessId((HANDLE)pid, &pe);
	if (NT_SUCCESS(status))
	{
		ProbeForRead(addr, size, 1);
		MmCopyVirtualMemory(pe, addr, PsGetCurrentProcess(), value, size, KernelMode, &bytes);
		ObfDereferenceObject(pe);
	}
	return status;
}

NTSTATUS core::AllocMemory(int pid, void* addr, size_t size, DWORD protect)
{
	PEPROCESS process = NULL;
	NTSTATUS status = PsLookupProcessByProcessId((HANDLE)pid, &process);
	if (NT_SUCCESS(status))
	{
		PVOID address = NULL;
		SIZE_T sizet = size;

		KeAttachProcess(process);
		ZwAllocateVirtualMemory(NtCurrentProcess(), &address, 0, &sizet, MEM_COMMIT | MEM_RESERVE, protect);
		KeDetachProcess();

		SafeCopy(addr, &address, sizeof(address));

		ObDereferenceObject(process);
	}
	return status;
}

NTSTATUS core::FreeMemory(int pid, void* addr)
{
	PEPROCESS process = NULL;
	NTSTATUS status = PsLookupProcessByProcessId((HANDLE)pid, &process);
	if (NT_SUCCESS(status))
	{
		SIZE_T size = 0;

		KeAttachProcess(process);
		ZwFreeVirtualMemory(NtCurrentProcess(), &addr, &size, MEM_RELEASE);
		KeDetachProcess();

		ObDereferenceObject(process);
	}
	return status;
}
