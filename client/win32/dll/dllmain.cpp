// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <stdio.h> 
#include <process.h>
#include "entry.h"
#include "procCommClient.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
        injectInit();
        break;

	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
        break;

	case DLL_PROCESS_DETACH:
        injectFree();
		break;
	}

	return TRUE;
}
