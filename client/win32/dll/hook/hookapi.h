#pragma once

#include <Windows.h> 
#include <TlHelp32.h> 

enum
{
	HOOK_IAT=1,
	HOOK_JMP,
	HOOK_MHOOK
};

typedef struct tag_HOOKAPI
{
	LPCSTR szFuncName;// 被 HOOK 的 API 函数名称。
	LPCSTR szDllName;

	PROC pNewProc;// 替代函数地址。
	PROC pOldProc;// 原 API 函数地址。

	unsigned char oldFirstCode[8];
	unsigned char newFirstCode[8];
	unsigned int codeLen;

	int type;
	bool result; //is hook success
}HOOKAPI, *LPHOOKAPI;

bool HookAPI(HMODULE hModule, LPHOOKAPI pHookApi);
bool UnHookAPI(HMODULE hModule, LPHOOKAPI pHookApi);
bool HookAPIRecovery(LPHOOKAPI pHookApi);
bool HookAPIResume(LPHOOKAPI pHookApi);
