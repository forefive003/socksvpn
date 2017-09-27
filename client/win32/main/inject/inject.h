#pragma once

#define DLL_PATH_NAME_32 "GoProxy.dll"
#define DLL_PATH_NAME_64 "GoProxy64.dll"

bool is_process_64(HANDLE hProcess);
bool is_system_VistaOrLater();

int inject_init();

int get_proc_id_by_name(char* lpProcessName, uint64_t *pid_arr);
bool is_proc_exist(const char *proc_name, uint64_t proc_id);

int inject_dll(const char *proc_name, uint64_t proc_id);
int unInject_dll(const char *proc_name, uint64_t proc_id);
int is_dll_injected(const char *proc_name, uint64_t proc_id, bool *isInjected);
