
#include <stdio.h> 
#include "stdafx.h"
#include <windows.h>
#include <TlHelp32.h> 
#include <io.h>

#include <memory>
#include <string>
#include "wow64ext.h"
#include "common.h"
#include "injectDll64.h"

//#pragma comment(lib,"wow64ext.lib")

int inject_dll_wow64(const char *proc_name, uint64_t proc_id, HANDLE hProcess, char *dll_name)
{
	DWORD dwDesiredAccess = PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ;
	auto closeProcessHandle = [](HANDLE hProcess1) {
		if (hProcess1 != NULL) CloseHandle(hProcess1);
	};
	ScopeResource<HANDLE, decltype(closeProcessHandle)> targetProcessHandle(OpenProcess(dwDesiredAccess, FALSE, (DWORD)proc_id), closeProcessHandle);
	if (targetProcessHandle.get() == NULL) {
		return -1;
	}
	unsigned char injectCode[] = {
		0x48, 0x89, 0x4c, 0x24, 0x08,                               // mov       qword ptr [rsp+8],rcx
		0x57,                                                       // push      rdi
		0x48, 0x83, 0xec, 0x20,                                     // sub       rsp,20h
		0x48, 0x8b, 0xfc,                                           // mov       rdi,rsp
		0xb9, 0x08, 0x00, 0x00, 0x00,                               // mov       ecx,8
		0xb8, 0xcc, 0xcc, 0xcc, 0xcc,                               // mov       eac,0CCCCCCCCh
		0xf3, 0xab,                                                 // rep stos  dword ptr [rdi]
		0x48, 0x8b, 0x4c, 0x24, 0x30,                               // mov       rcx,qword ptr [__formal]
		0x49, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov       r9,0
		0x49, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov       r8,0
		0x48, 0xba, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov       rdx,0
		0x48, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov       rcx,0
		0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov       rax,0
		0xff, 0xd0,                                                 // call      rax
		0x48, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov       rcx,0
		0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov       rax,0
		0xff, 0xd0                                                  // call      rax
	};

	size_t parametersMemSize = sizeof(DWORD64) + sizeof(_UNICODE_STRING_T<DWORD64>) + (strlen(dll_name) + 1) * sizeof(wchar_t);
	auto freeInjectCodeMem = [&targetProcessHandle, &injectCode](DWORD64 address) {
		if (address != 0) VirtualFreeEx64(targetProcessHandle.get(), address, sizeof(injectCode), MEM_COMMIT | MEM_RESERVE);
	};
	ScopeResource<DWORD64, decltype(freeInjectCodeMem)> injectCodeMem(VirtualAllocEx64(targetProcessHandle.get(), NULL, sizeof(injectCode), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE), freeInjectCodeMem);
	auto freeParametersMem = [&targetProcessHandle, parametersMemSize](DWORD64 address) {
		if (address != 0) VirtualFreeEx64(targetProcessHandle.get(), address, parametersMemSize, MEM_COMMIT | MEM_RESERVE);
	};
	ScopeResource<DWORD64, decltype(freeParametersMem)> parametersMem(VirtualAllocEx64(targetProcessHandle.get(), NULL, parametersMemSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE), freeParametersMem);
	if (injectCodeMem.get() == 0 || parametersMem.get() == 0) {
		return -1;
	}
	DWORD64 ntdll64 = GetModuleHandle64(L"ntdll.dll");
	DWORD64 ntdll_LdrLoadDll = GetProcAddress64(ntdll64, "LdrLoadDll");
	DWORD64 ntdll_RtlExitUserThread = GetProcAddress64(ntdll64, "RtlExitUserThread");
	DWORD64 ntdll_RtlCreateUserThread = GetProcAddress64(ntdll64, "RtlCreateUserThread");
	if (ntdll_LdrLoadDll == 0 || ntdll_RtlExitUserThread == 0 || ntdll_RtlCreateUserThread == 0) {
		return -1;
	}
	std::unique_ptr<unsigned char[]> parameters(new unsigned char[parametersMemSize]);
	std::memset(parameters.get(), 0, parametersMemSize);
	_UNICODE_STRING_T<DWORD64>* upath = reinterpret_cast<_UNICODE_STRING_T<DWORD64>*>(parameters.get() + sizeof(DWORD64));
	upath->Length = (WORD)strlen(dll_name) * sizeof(wchar_t);
	upath->MaximumLength = (WORD)(strlen(dll_name) + 1) * sizeof(wchar_t);
	wchar_t* path = reinterpret_cast<wchar_t*>(parameters.get() + sizeof(DWORD64) + sizeof(_UNICODE_STRING_T<DWORD64>));
	//std::copy(filename.begin(), filename.end(), path);
	int bufSize = MultiByteToWideChar(CP_ACP, 0, dll_name, -1, NULL, 0);
	MultiByteToWideChar(CP_ACP, 0, dll_name, -1, path, bufSize);
	upath->Buffer = parametersMem.get() + sizeof(DWORD64) + sizeof(_UNICODE_STRING_T<DWORD64>);

	union {
		DWORD64 from;
		unsigned char to[8];
	} cvt;

	// r9
	cvt.from = parametersMem.get();
	std::memcpy(injectCode + 32, cvt.to, sizeof(cvt.to));

	// r8
	cvt.from = parametersMem.get() + sizeof(DWORD64);
	std::memcpy(injectCode + 42, cvt.to, sizeof(cvt.to));

	// rax = LdrLoadDll
	cvt.from = ntdll_LdrLoadDll;
	std::memcpy(injectCode + 72, cvt.to, sizeof(cvt.to));

	// rax = RtlExitUserThread
	cvt.from = ntdll_RtlExitUserThread;
	std::memcpy(injectCode + 94, cvt.to, sizeof(cvt.to));

	if (FALSE == WriteProcessMemory64(targetProcessHandle.get(), injectCodeMem.get(), injectCode, sizeof(injectCode), NULL)
		|| FALSE == WriteProcessMemory64(targetProcessHandle.get(), parametersMem.get(), parameters.get(), parametersMemSize, NULL)) {
		return -1;
	}

	DWORD64 hRemoteThread = 0;
	struct {
		DWORD64 UniqueProcess;
		DWORD64 UniqueThread;
	} client_id;

	X64Call(ntdll_RtlCreateUserThread, 10,
		(DWORD64)targetProcessHandle.get(), // ProcessHandle
		(DWORD64)NULL,                      // SecurityDescriptor
		(DWORD64)FALSE,                     // CreateSuspended
		(DWORD64)0,                         // StackZeroBits
		(DWORD64)NULL,                      // StackReserved
		(DWORD64)NULL,                      // StackCommit
		injectCodeMem.get(),                // StartAddress
		(DWORD64)NULL,                      // StartParameter
		(DWORD64)&hRemoteThread,            // ThreadHandle
		(DWORD64)&client_id);               // ClientID
	if (hRemoteThread != 0) {
		CloseHandle((HANDLE)hRemoteThread);
		return 0;
	}
	return -1;
}


int unInject_dll_wow64(const char *proc_name, uint64_t proc_id, HANDLE hProcess, char *dll_name)
{
	return 0;
}

int is_dll_injected_wow64(const char *proc_name, uint64_t proc_id, HANDLE hProcess, char *dll_name, bool *isInjected)
{
	*isInjected = false;
	return 0;
}
