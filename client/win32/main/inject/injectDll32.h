#pragma once



typedef DWORD (WINAPI *PFNTCREATETHREADEX)
( 
    PHANDLE                 ThreadHandle,   
    ACCESS_MASK             DesiredAccess,  
    LPVOID                  ObjectAttributes,   
    HANDLE                  ProcessHandle,  
    LPTHREAD_START_ROUTINE  lpStartAddress, 
    LPVOID                  lpParameter,    
    BOOL                    CreateSuspended,    
    DWORD                   dwStackSize,    
    DWORD                   dw1, 
    DWORD                   dw2, 
    LPVOID                  Unknown 
); 

int inject_dll_32(const char *proc_name, uint64_t proc_id, HANDLE hProcess, char *dll_name);
int unInject_dll_32(const char *proc_name, uint64_t proc_id, HANDLE hProcess, char *dll_name);
int is_dll_injected_32(const char *proc_name, uint64_t proc_id, HANDLE hProcess, char *dll_name, bool *isInjected);
