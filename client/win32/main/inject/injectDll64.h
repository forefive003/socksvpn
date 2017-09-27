#pragma once


template<typename Res, typename Deleter>
class ScopeResource {
	Res res;
	Deleter deleter;
	ScopeResource(const ScopeResource&) {}
public:
	Res get() const {
		return this->res;
	}
	ScopeResource(Res res, Deleter deleter) : res(res), deleter(deleter) {}
	~ScopeResource() {
		this->deleter(this->res);
	}
};


int inject_dll_wow64(const char *proc_name, uint64_t proc_id, HANDLE hProcess, char *dll_name);
int unInject_dll_wow64(const char *proc_name, uint64_t proc_id, HANDLE hProcess, char *dll_name);
int is_dll_injected_wow64(const char *proc_name, uint64_t proc_id, HANDLE hProcess, char *dll_name, bool *isInjected);
