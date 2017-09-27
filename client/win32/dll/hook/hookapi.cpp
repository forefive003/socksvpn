#include "stdafx.h"
#include <stdio.h> 
#include <Windows.h> 
#include <TlHelp32.h> 
#include <io.h>
#include "entry.h"
#include "hookapi.h"
#include "mhook-lib/mhook.h"

static PIMAGE_IMPORT_DESCRIPTOR LocationIAT(HMODULE hModule, LPCSTR szImportMod)
// 其中， hModule 为进程模块句柄； szImportMod 为输入库名称。
{
	// 检查是否为 DOS 程序，如是返回 NULL ，因 DOS 程序没有 IAT 。
	PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hModule;
	if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE) 
		return NULL;

	// 检查是否为 NT 标志，否则返回 NULL 。
	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((DWORD)pDOSHeader + (DWORD)(pDOSHeader->e_lfanew));
	if (pNTHeader->Signature != IMAGE_NT_SIGNATURE) 
		return NULL;

	// 没有 IAT 表则返回 NULL 。
	if (pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress == 0) 
		return NULL;

	// 定位第一个 IAT 位置。
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)pDOSHeader + (DWORD)(pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress));
	// 根据输入库名称循环检查所有的 IAT ，如匹配则返回该 IAT 地址，否则检测下一个 IAT 。
	while (pImportDesc->Name)
	{
		// 获取该 IAT 描述的输入库名称。
		PSTR szCurrMod = (PSTR)((DWORD)pDOSHeader + (DWORD)(pImportDesc->Name));
		if (_stricmp(szCurrMod, szImportMod) == 0) 
			break;

		pImportDesc++;
	}

	if (pImportDesc->Name == NULL) 
		return NULL;

	return pImportDesc;
}

static bool UnHookAPIByIAT(HMODULE hModule, LPHOOKAPI pHookApi)
{
	// 定位 szImportMod 输入库在输入数据段中的 IAT 地址。
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = LocationIAT(hModule, pHookApi->szDllName);
	if (pImportDesc == NULL) 
	{
		MessageBox(NULL,"fail to find import descriptor","Error",MB_OK);
		return false;
	}
	
	bool result = false;
	if (NULL == pHookApi->pOldProc)
	{
		pHookApi->pOldProc = GetProcAddress(GetModuleHandle(pHookApi->szDllName), pHookApi->szFuncName);
	}

	// 第一个 Thunk 地址。
	PIMAGE_THUNK_DATA pOrigThunk = (PIMAGE_THUNK_DATA)((DWORD)hModule + (DWORD)(pImportDesc->OriginalFirstThunk));
	// 第一个 IAT 项的 Thunk 地址。
	PIMAGE_THUNK_DATA pRealThunk = (PIMAGE_THUNK_DATA)((DWORD)hModule + (DWORD)(pImportDesc->FirstThunk));
	// 循环查找被截 API 函数的 IAT 项，并使用替代函数地址修改其值。
	while (pOrigThunk->u1.Function)
	{
		// 检测此 Thunk 是否为 IAT 项。
		if ((pOrigThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG) != IMAGE_ORDINAL_FLAG)
		{
			// 获取此 IAT 项所描述的函数名称。
			PIMAGE_IMPORT_BY_NAME pByName = (PIMAGE_IMPORT_BY_NAME)((DWORD)hModule + (DWORD)(pOrigThunk->u1.AddressOfData));
			if (pByName->Name[0] == '//0') 
			{
				return false;
			}

			// 检测是否为挡截函数。
			if (_strcmpi(pHookApi->szFuncName, (char*)pByName->Name) == 0)
			{
				MEMORY_BASIC_INFORMATION mbi_thunk;
				// 查询修改页的信息。
				VirtualQuery(pRealThunk, &mbi_thunk, sizeof(MEMORY_BASIC_INFORMATION));
				// 改变修改页保护属性为 PAGE_READWRITE 。
				VirtualProtect(mbi_thunk.BaseAddress, mbi_thunk.RegionSize, PAGE_READWRITE, &mbi_thunk.Protect);

				//修改API函数IAT项内容为替代函数地址。
				pRealThunk->u1.Function = (DWORD)pHookApi->pOldProc;

				//恢复修改页保护属性。
				DWORD dwOldProtect;
				VirtualProtect(mbi_thunk.BaseAddress, mbi_thunk.RegionSize, mbi_thunk.Protect, &dwOldProtect);
				
				result = true;
				break;
			}
		}
		else
		{
			// 检测此 Thunk 是否为 IAT 项。
			if (pRealThunk->u1.Function == (DWORD)pHookApi->pNewProc)
			{
				MEMORY_BASIC_INFORMATION mbi_thunk;
				// 查询修改页的信息。
				VirtualQuery(pRealThunk, &mbi_thunk, sizeof(MEMORY_BASIC_INFORMATION));
				// 改变修改页保护属性为 PAGE_READWRITE 。
				VirtualProtect(mbi_thunk.BaseAddress, mbi_thunk.RegionSize, PAGE_READWRITE, &mbi_thunk.Protect);

				//修改API函数IAT项内容为替代函数地址。
				pRealThunk->u1.Function = (DWORD)pHookApi->pOldProc;

				//恢复修改页保护属性。
				DWORD dwOldProtect;
				VirtualProtect(mbi_thunk.BaseAddress, mbi_thunk.RegionSize, mbi_thunk.Protect, &dwOldProtect);

				result = true;
				break;
			}
		}
		
		pOrigThunk++;
		pRealThunk++;
	}

	SetLastError(ERROR_SUCCESS); //设置错误为ERROR_SUCCESS，表示成功。
	return result;
}

static bool HookAPIByIAT(HMODULE hModule, LPHOOKAPI pHookApi)
// 其中， hModule 为进程模块句柄； szImportMod 为输入库名称； pHookAPI 为 HOOKAPI 结构指针。
{
	// 定位 szImportMod 输入库在输入数据段中的 IAT 地址。
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = LocationIAT(hModule, pHookApi->szDllName);
	if (pImportDesc == NULL) 
	{
		MessageBox(NULL,"fail to find import descriptor","Error",MB_OK);
		return false;
	}
	
	bool result = false;
	if (NULL == pHookApi->pOldProc)
	{
		pHookApi->pOldProc = GetProcAddress(GetModuleHandle(pHookApi->szDllName), pHookApi->szFuncName);
	}

	// 第一个 Thunk 地址。
	PIMAGE_THUNK_DATA pOrigThunk = (PIMAGE_THUNK_DATA)((DWORD)hModule + (DWORD)(pImportDesc->OriginalFirstThunk));
	// 第一个 IAT 项的 Thunk 地址。
	PIMAGE_THUNK_DATA pRealThunk = (PIMAGE_THUNK_DATA)((DWORD)hModule + (DWORD)(pImportDesc->FirstThunk));
	// 循环查找被截 API 函数的 IAT 项，并使用替代函数地址修改其值。
	while (pOrigThunk->u1.Function)
	{
		// 检测此 Thunk 是否为 IAT 项。
		if ((pOrigThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG) != IMAGE_ORDINAL_FLAG)
		{
			// 获取此 IAT 项所描述的函数名称。
			PIMAGE_IMPORT_BY_NAME pByName = (PIMAGE_IMPORT_BY_NAME)((DWORD)hModule + (DWORD)(pOrigThunk->u1.AddressOfData));
			if (pByName->Name[0] == '//0') 
			{
				return false;
			}

			// 检测是否为挡截函数。
			if (_strcmpi(pHookApi->szFuncName, (char*)pByName->Name) == 0)
			{
				MEMORY_BASIC_INFORMATION mbi_thunk;
				// 查询修改页的信息。
				VirtualQuery(pRealThunk, &mbi_thunk, sizeof(MEMORY_BASIC_INFORMATION));
				// 改变修改页保护属性为 PAGE_READWRITE 。
				VirtualProtect(mbi_thunk.BaseAddress, mbi_thunk.RegionSize, PAGE_READWRITE, &mbi_thunk.Protect);

				// 保存原来的 API 函数地址。
				if (pHookApi->pOldProc == NULL)
					pHookApi->pOldProc = (PROC)pRealThunk->u1.Function;

				//修改API函数IAT项内容为替代函数地址。
				pRealThunk->u1.Function = (DWORD)pHookApi->pNewProc;

				//恢复修改页保护属性。
				DWORD dwOldProtect;
				VirtualProtect(mbi_thunk.BaseAddress, mbi_thunk.RegionSize, mbi_thunk.Protect, &dwOldProtect);
				
				result = true;
				break;
			}
		}
		else
		{
			// 检测此 Thunk 是否为 IAT 项。
			if (pRealThunk->u1.Function == (DWORD)pHookApi->pOldProc)
			{
				MEMORY_BASIC_INFORMATION mbi_thunk;
				// 查询修改页的信息。
				VirtualQuery(pRealThunk, &mbi_thunk, sizeof(MEMORY_BASIC_INFORMATION));
				// 改变修改页保护属性为 PAGE_READWRITE 。
				VirtualProtect(mbi_thunk.BaseAddress, mbi_thunk.RegionSize, PAGE_READWRITE, &mbi_thunk.Protect);

				//修改API函数IAT项内容为替代函数地址。
				pRealThunk->u1.Function = (DWORD)pHookApi->pNewProc;

				//恢复修改页保护属性。
				DWORD dwOldProtect;
				VirtualProtect(mbi_thunk.BaseAddress, mbi_thunk.RegionSize, mbi_thunk.Protect, &dwOldProtect);
					
				pHookApi->result = true;
				result = true;
				break;
			}
		}
		
		pOrigThunk++;
		pRealThunk++;
	}

	SetLastError(ERROR_SUCCESS); //设置错误为ERROR_SUCCESS，表示成功。
	pHookApi->result = result;
	return result;
}

static bool UnHookAPIByJmp(HMODULE hModule, LPHOOKAPI pHookApi)
{
    DWORD dwOldProtect = 0;
    ::VirtualProtect((void*)pHookApi->pOldProc, pHookApi->codeLen, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	/*恢复原始函数*/
    if (!WriteProcessMemory(g_process_hdl, (void*)pHookApi->pOldProc,
					(void*)pHookApi->oldFirstCode, pHookApi->codeLen, NULL))
    {
        ::VirtualProtect((void*)pHookApi->pOldProc, pHookApi->codeLen, dwOldProtect, NULL);
        return false;
    }

    ::VirtualProtect((void*)pHookApi->pOldProc, pHookApi->codeLen, dwOldProtect, NULL);
    return true;
}

static bool HookAPIByJmp(HMODULE hModule, LPHOOKAPI pHookApi)
{
	#if 1
	pHookApi->newFirstCode[0] = 0xe9;//实际上0xe9就相当于jmp指令   
	pHookApi->newFirstCode[1] = 0x00;
	pHookApi->newFirstCode[2] = 0x00;
	pHookApi->newFirstCode[3] = 0x00;
	pHookApi->newFirstCode[4] = 0x00;
	pHookApi->newFirstCode[5] = 0x00;
	pHookApi->newFirstCode[6] = 0x00;
	pHookApi->newFirstCode[7] = 0x00;
	pHookApi->codeLen = 5;
	#else
	pHookApi->newFirstCode[0] = 0x68;//push xxxx; ret
	pHookApi->newFirstCode[1] = 0x00;
	pHookApi->newFirstCode[2] = 0x00;
	pHookApi->newFirstCode[3] = 0x00;
	pHookApi->newFirstCode[4] = 0x00;
	pHookApi->newFirstCode[5] = 0xc3;
	pHookApi->newFirstCode[6] = 0x00;
	pHookApi->newFirstCode[7] = 0x00;
	#endif

	if (NULL == pHookApi->pOldProc)
	{
		pHookApi->pOldProc = GetProcAddress(GetModuleHandle(pHookApi->szDllName), pHookApi->szFuncName);
	}

	DWORD dwOldProtect = 0;
	::VirtualProtect((LPVOID)pHookApi->pOldProc, pHookApi->codeLen, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	// 事先保存被破坏的指令
	// memcpy(pHookApi->oldFirstCode, pHookApi->pOldProc, 5); 
	BOOL bOk = false;
	bOk = ReadProcessMemory(g_process_hdl, (void*)pHookApi->pOldProc,
					(void*)pHookApi->oldFirstCode, pHookApi->codeLen, NULL);
	if (!bOk)
	{
		::VirtualProtect((LPVOID)pHookApi->pOldProc, pHookApi->codeLen, dwOldProtect, NULL);
		MessageBox(NULL,"ReadProcessMemory failed","Error",MB_OK);
		return false;
	}

	//被HOOK的函数头改为jmp
	*(DWORD*)(&pHookApi->newFirstCode[1]) = (DWORD)pHookApi->pNewProc - ((DWORD)pHookApi->pOldProc + 5);

	//写入新地址
	bOk = WriteProcessMemory(g_process_hdl, (void*)pHookApi->pOldProc,
					(void*)pHookApi->newFirstCode, pHookApi->codeLen, NULL);
	if (!bOk)
	{
		::VirtualProtect((LPVOID)pHookApi->pOldProc, pHookApi->codeLen, dwOldProtect, NULL);
		MessageBox(NULL,"WriteProcessMemory failed","Error",MB_OK);
		return false;
	}

	::VirtualProtect((LPVOID)pHookApi->pOldProc, pHookApi->codeLen, dwOldProtect, NULL);

	pHookApi->result = true;
	return true;
}

static bool HookAPIResumeByJmp(LPHOOKAPI pHookApi)
{
	if (pHookApi->codeLen == 0)
	{
		return true;
	}

    DWORD dwOldProtect = 0;
    ::VirtualProtect((void*)pHookApi->pOldProc, pHookApi->codeLen, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	/*恢复原始函数*/
    if (!WriteProcessMemory(g_process_hdl, (void*)pHookApi->pOldProc,
					(void*)pHookApi->oldFirstCode, pHookApi->codeLen, NULL))
    {
        ::VirtualProtect((void*)pHookApi->pOldProc, pHookApi->codeLen, dwOldProtect, NULL);
        return false;
    }

    ::VirtualProtect((void*)pHookApi->pOldProc, pHookApi->codeLen, dwOldProtect, NULL);
    return true;
}

static bool HookAPIRecoveryByJmp(LPHOOKAPI pHookApi)
{
	if (pHookApi->codeLen == 0)
	{
		return true;
	}

    DWORD dwOldProtect = 0;
    ::VirtualProtect((void*)pHookApi->pOldProc, pHookApi->codeLen, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	/*恢复原始函数*/
    if (!WriteProcessMemory(g_process_hdl, (void*)pHookApi->pOldProc,
					(void*)pHookApi->newFirstCode, pHookApi->codeLen, NULL))
    {
        ::VirtualProtect((void*)pHookApi->pOldProc, pHookApi->codeLen, dwOldProtect, NULL);
        return false;
    }

    ::VirtualProtect((void*)pHookApi->pOldProc, pHookApi->codeLen, dwOldProtect, NULL);
    return true;
}


static bool HookAPIByMhook(HMODULE hModule, LPHOOKAPI pHookApi)
{
	if (NULL == pHookApi->pOldProc)
	{
		pHookApi->pOldProc = GetProcAddress(GetModuleHandle(pHookApi->szDllName), pHookApi->szFuncName);
	}

	if (TRUE == Mhook_SetHook((PVOID*)&pHookApi->pOldProc, pHookApi->pNewProc))
	{
		pHookApi->result = true;
		return true;
	}

	return false;
}

static bool UnHookAPIByMhook(HMODULE hModule, LPHOOKAPI pHookApi)
{
	return TRUE == Mhook_Unhook((PVOID*)&pHookApi->pOldProc);
}


bool HookAPI(HMODULE hModule, LPHOOKAPI pHookApi)
{
	if (pHookApi->type == HOOK_MHOOK)
	{
		return HookAPIByMhook(hModule, pHookApi);
	}
	else if(pHookApi->type == HOOK_JMP)
	{
		return HookAPIByJmp(hModule, pHookApi);
	}
	return HookAPIByIAT(hModule, pHookApi);
}


bool UnHookAPI(HMODULE hModule, LPHOOKAPI pHookApi)
{
	if (pHookApi->type == HOOK_MHOOK)
	{
		return UnHookAPIByMhook(hModule, pHookApi);
	}
	else if (pHookApi->type == HOOK_JMP)
	{
		return UnHookAPIByJmp(hModule, pHookApi);
	}
	return UnHookAPIByIAT(hModule, pHookApi);
}

bool HookAPIRecovery(LPHOOKAPI pHookApi)
{
	if (pHookApi->type == HOOK_JMP)
	{
		return HookAPIRecoveryByJmp(pHookApi);
	}

	return true;
}

bool HookAPIResume(LPHOOKAPI pHookApi)
{
	if (pHookApi->type == HOOK_JMP)
	{
		return HookAPIResumeByJmp(pHookApi);
	}

	return true;
}
