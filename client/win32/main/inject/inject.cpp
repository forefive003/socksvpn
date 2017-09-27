#include <stdio.h> 
#include "stdafx.h"
#include <windows.h>
#include <TlHelp32.h> 
#include <io.h>
#include <tchar.h>

#include "common.h"
#include "inject.h"
#include "injectDll32.h"
#include "injectDll64.h"
#include "wow64ext.h"
#include "proxyConfig.h"

typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);

static LPFN_ISWOW64PROCESS g_fnIsWow64Process = NULL;
static bool g_is_system_64 = false; /*如果进程运行在wow64上,说明系统是64位的,因为本程序是32位的*/


bool is_system_VistaOrLater()
{
    OSVERSIONINFO osvi;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

    if( osvi.dwMajorVersion >= 6 )
    {
        return TRUE;
    }

    return FALSE;
}

bool is_system_64()
{
	BOOL bIsWow64 = FALSE;

	if (NULL != g_fnIsWow64Process)
	{
		if (!g_fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			_LOG_ERROR(_T("call IsWow64Process failed, error %d"), GetLastError());
			return false;
		}
	}

	return (bIsWow64 == TRUE);
}

bool is_process_64(HANDLE hProcess)
{
	BOOL bIsWow64 = FALSE;

	if (g_is_system_64 == false)
	{
		/*本身是32位系统*/
		return false;
	}

	if (NULL != g_fnIsWow64Process)
	{
		if (!g_fnIsWow64Process(hProcess, &bIsWow64))
		{
			_LOG_ERROR(_T("call IsWow64Process failed, error %d"), GetLastError());
			return false;
		}
		if (bIsWow64)
		{
			/*说明运行在wow64上,是一个32位的进程*/
			return false;
		}

		/*不是运行在wow64上, 进程本身是64位的*/
		return true;
	}

	return false;
}

int get_proc_id_by_name(char* lpProcessName, uint64_t *pid_arr)
{
	int count = 0;

	DWORD dwRet = 0;
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapShot == INVALID_HANDLE_VALUE)
	{
		_LOG_ERROR(_T("CreateToolhelp32Snapshot failed when get pid, %d"), GetLastError());
		return dwRet;
	}

	PROCESSENTRY32 pe32;//声明进程入口对象 
	pe32.dwSize = sizeof(PROCESSENTRY32);//填充进程入口对象大小 
	BOOL bMORE = ::Process32First(hSnapShot, &pe32);
	while (bMORE)
	{
		if (!lstrcmp(pe32.szExeFile, lpProcessName))//查找指定进程名的PID 
		{
			/*线程个数为0，等待; 否则可能被注入进程刚启动,提示无法打开设备/文件/路径*/
			if (pe32.cntThreads == 0)
			{
				continue;
			}

			pid_arr[count] = pe32.th32ProcessID;
			count++;

			if (count >= PROXY_CFG_MAX_PROC)
			{
				break;
			}
		}
		bMORE = ::Process32Next(hSnapShot, &pe32);
	}

	::CloseHandle(hSnapShot);
	return count;
}

bool is_proc_exist(const char *proc_name, uint64_t proc_id)
{
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapShot == INVALID_HANDLE_VALUE)
	{
		_LOG_ERROR(_T("CreateToolhelp32Snapshot failed, %d"), GetLastError());
		return false;
	}

	PROCESSENTRY32 pe32;//声明进程入口对象 
	pe32.dwSize = sizeof(PROCESSENTRY32);//填充进程入口对象大小 
	Process32First(hSnapShot, &pe32);//遍历进程列表 
	do
	{
		if (!lstrcmp(pe32.szExeFile, proc_name)
			&& (pe32.th32ProcessID == proc_id))//查找指定进程名的PID 
		{
			::CloseHandle(hSnapShot);
			return true;
		}
	} while (Process32Next(hSnapShot, &pe32));

	::CloseHandle(hSnapShot);
	return false;
}

BOOL  EnableDebugPrivilege()
{
	HANDLE hToken;
	BOOL fOk = FALSE;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) //Get Token
	{
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid))//Get Luid
			printf("Can't lookup privilege value.\n");

		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;//这一句很关键，修改其属性为SE_PRIVILEGE_ENABLED
		if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL))//Adjust Token
			printf("Can't adjust privilege value.\n");

		fOk = (GetLastError() == ERROR_SUCCESS);
		CloseHandle(hToken);
	}

	return fOk;
}

int inject_dll(const char *proc_name, uint64_t proc_id)
{
#if 0
	typedef long(__fastcall *pfnRtlAdjustPrivilege64)(ULONG, ULONG, ULONG, PVOID);
	pfnRtlAdjustPrivilege64 RtlAdjustPrivilege;

	DWORD                  dwRetVal = 0;
	RtlAdjustPrivilege = (pfnRtlAdjustPrivilege64)::GetProcAddress((HMODULE)LoadLibrary(_T("ntdll.dll")), "RtlAdjustPrivilege");

	if (RtlAdjustPrivilege == NULL)
	{
		return -1;
	}
	/*
	.常量 SE_BACKUP_PRIVILEGE, "17", 公开
	.常量 SE_RESTORE_PRIVILEGE, "18", 公开
	.常量 SE_SHUTDOWN_PRIVILEGE, "19", 公开
	.常量 SE_DEBUG_PRIVILEGE, "20", 公开
	*/
	RtlAdjustPrivilege(20, 1, 0, &dwRetVal);  //19
#endif

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, (DWORD)proc_id);
	if (hProcess == NULL)
	{
		_LOG_ERROR(_T("OpenProcess %s failed, %d"), proc_name, GetLastError());
		return -1;
	}
	
	int ret = 0;
	char lpDllName[MAX_PATH + 1] = { 0 };
	bool need64 = false;

	if (g_is_system_64)
	{
		if (is_process_64(hProcess))
		{
			need64 = true;
		}
	}
	
	if (need64)
	{
		_snprintf_s(lpDllName, MAX_PATH, "%s%s", g_localModDir, DLL_PATH_NAME_64);

		// 指定 Dll 文件不存在  
		if (-1 == _access(lpDllName, 0))
		{
			::CloseHandle(hProcess);
			_LOG_ERROR(_T("dll(%s) not exist"), lpDllName);
			return -1;
		}

		ret = inject_dll_wow64(proc_name, proc_id, hProcess, lpDllName);
	}
	else
	{
		_snprintf_s(lpDllName, MAX_PATH, "%s%s", g_localModDir, DLL_PATH_NAME_32);

		// 指定 Dll 文件不存在  
		if (-1 == _access(lpDllName, 0))
		{
			::CloseHandle(hProcess);
			_LOG_ERROR(_T("dll(%s) not exist"), lpDllName);
			return -1;
		}

		ret = inject_dll_32(proc_name, proc_id, hProcess, lpDllName);
	}

	::CloseHandle(hProcess);
	return ret;
}

int unInject_dll(const char *proc_name, uint64_t proc_id)
{
	char lpDllName[MAX_PATH + 1] = { 0 };
	bool need64 = false;
	
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, (DWORD)proc_id);
	if (hProcess == NULL)
	{
		_LOG_ERROR(_T("OpenProcess %s failed, %d"), proc_name, GetLastError());
		return -1;
	}
	
	if (g_is_system_64)
	{
		if (is_process_64(hProcess))
		{
			need64 = true;
		}
	}

	int ret = 0;
	if (need64)
	{
		_snprintf_s(lpDllName, MAX_PATH, "%s%s", g_localModDir, DLL_PATH_NAME_64);
		ret = unInject_dll_wow64(proc_name, proc_id, hProcess, lpDllName);
	}
	else
	{
		_snprintf_s(lpDllName, MAX_PATH, "%s%s", g_localModDir, DLL_PATH_NAME_32);
		ret = unInject_dll_32(proc_name, proc_id, hProcess, lpDllName);
	}

	::CloseHandle(hProcess);
	return ret;
}

int is_dll_injected(const char *proc_name, uint64_t proc_id, bool *isInjected)
{
	char lpDllName[MAX_PATH + 1] = { 0 };
	bool need64 = false;

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, (DWORD)proc_id);
	if (hProcess == NULL)
	{
		_LOG_ERROR(_T("OpenProcess %s failed, %d"), proc_name, GetLastError());
		return -1;
	}
	
	if (g_is_system_64)
	{
		if (is_process_64(hProcess))
		{
			need64 = true;
		}
	}

	int ret = 0;
	if (need64)
	{
		_snprintf_s(lpDllName, MAX_PATH, "%s%s", g_localModDir, DLL_PATH_NAME_64);
		ret = is_dll_injected_wow64(proc_name, proc_id, hProcess, lpDllName, isInjected);
	}
	else
	{
		_snprintf_s(lpDllName, MAX_PATH, "%s%s", g_localModDir, DLL_PATH_NAME_32);
		ret = is_dll_injected_32(proc_name, proc_id, hProcess, lpDllName, isInjected);
	}
	
	::CloseHandle(hProcess);
	return ret;
}

int inject_init()
{
	EnableDebugPrivilege();
	g_fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(::GetModuleHandle(_T("Kernel32.dll")), "IsWow64Process");
	g_is_system_64 = is_system_64();
	if (g_is_system_64)
	{
		wow64ext_init();
	}

	return 0;
}
