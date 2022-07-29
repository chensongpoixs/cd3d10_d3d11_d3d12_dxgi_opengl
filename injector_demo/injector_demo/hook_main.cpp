/***********************************************************************************************
				   created: 		2022-07-27

				   author:			chensong

				   purpose:		    动态库启动后注入demo
************************************************************************************************/
 
#define _SILENCE_CXX17_C_HEADER_DEPRECATION_WARNING
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <Windows.h>
#include <cstdbool>
//
#include <stdio.h>
#include <Windows.h>
#include <sddl.h>
#include <AclAPI.h>
#include <TlHelp32.h>
#include <iostream>




typedef HANDLE(WINAPI* create_remote_thread_t)(HANDLE, LPSECURITY_ATTRIBUTES,
	SIZE_T, LPTHREAD_START_ROUTINE,
	LPVOID, DWORD, LPDWORD);
typedef BOOL(WINAPI* write_process_memory_t)(HANDLE, LPVOID, LPCVOID, SIZE_T,
	SIZE_T*);
typedef LPVOID(WINAPI* virtual_alloc_ex_t)(HANDLE, LPVOID, SIZE_T, DWORD,
	DWORD);
typedef BOOL(WINAPI* virtual_free_ex_t)(HANDLE, LPVOID, SIZE_T, DWORD);




#define LOWER_HALFBYTE(x) ((x)&0xF)
#define UPPER_HALFBYTE(x) (((x) >> 4) & 0xF)

static void deobfuscate_str(char* str, uint64_t val)
{
	uint8_t* dec_val = (uint8_t*)&val;
	int i = 0;

	while (*str != 0) {
		int pos = i / 2;
		bool bottom = (i % 2) == 0;
		uint8_t* ch = (uint8_t*)str;
		uint8_t xor1 = bottom ? LOWER_HALFBYTE(dec_val[pos]) : UPPER_HALFBYTE(dec_val[pos]);

		*ch ^=  xor1;

		if (++i == sizeof(uint64_t) * 2)
		{
			i = 0;
		}

		str++;
	}
}

void* ms_get_obfuscated_func(HMODULE module, const char* str, uint64_t val)
{
	char new_name[128];
	strcpy(new_name, str);
	deobfuscate_str(new_name, val);
	return GetProcAddress(module, new_name);
}



int inject_library_obf(HANDLE process, const wchar_t* dll,
	const char* create_remote_thread_obf, uint64_t obf1,
	const char* write_process_memory_obf, uint64_t obf2,
	const char* virtual_alloc_ex_obf, uint64_t obf3,
	const char* virtual_free_ex_obf, uint64_t obf4,
	const char* load_library_w_obf, uint64_t obf5)
{

	int ret = -4;
	size_t size = 0;
	DWORD last_error = 0;
	bool success = false;
	void* mem = NULL;
	HANDLE thread = NULL;
	size_t written_size;
	DWORD thread_id;

	HMODULE kernel32 = GetModuleHandleW(L"KERNEL32");

	create_remote_thread_t create_remote_thread;
	write_process_memory_t write_process_memory;
	virtual_alloc_ex_t virtual_alloc_ex;
	virtual_free_ex_t virtual_free_ex;
	FARPROC load_library_w;

	create_remote_thread = (create_remote_thread_t)ms_get_obfuscated_func(
		kernel32, create_remote_thread_obf, obf1);
	write_process_memory = (write_process_memory_t)ms_get_obfuscated_func(
		kernel32, write_process_memory_obf, obf2);
	virtual_alloc_ex = (virtual_alloc_ex_t)ms_get_obfuscated_func(
		kernel32, virtual_alloc_ex_obf, obf3);
	virtual_free_ex = (virtual_free_ex_t)ms_get_obfuscated_func(
		kernel32, virtual_free_ex_obf, obf4);
	load_library_w = (FARPROC)ms_get_obfuscated_func(
		kernel32, load_library_w_obf, obf5);



	/* -------------------------------- */

	size = (wcslen(dll) + 1) * sizeof(wchar_t);
	mem = virtual_alloc_ex(process, NULL, size, MEM_RESERVE | MEM_COMMIT,
		PAGE_READWRITE);
	if (!mem) {
		goto fail;
	}

	success = write_process_memory(process, mem, dll, size, &written_size);
	if (!success) {
		goto fail;
	}


	thread = create_remote_thread(process, NULL, 0,
		(LPTHREAD_START_ROUTINE)load_library_w,
		mem, 0, &thread_id);
	if (!thread) {
		goto fail;
	}

	if (WaitForSingleObject(thread, 4000) == WAIT_OBJECT_0) {
		DWORD code;
		GetExitCodeThread(thread, &code);
		ret = (code != 0) ? 0 : -1;

		SetLastError(0);
	}




fail:
	if (ret == -4) {
		last_error = GetLastError();
	}
	if (thread) {
		CloseHandle(thread);
	}
	if (mem) {
		virtual_free_ex(process, mem, 0, MEM_RELEASE);
	}
	if (last_error != 0) {
		SetLastError(last_error);
	}

	return ret;
}
static inline HANDLE open_process(DWORD desired_access, bool inherit_handle, DWORD process_id)
{
	typedef HANDLE(WINAPI * open_process_proc_t)(DWORD, BOOL, DWORD);
	open_process_proc_t open_process_proc = (open_process_proc_t)ms_get_obfuscated_func(GetModuleHandleW(L"KERNEL32"),
			"HxjcQrmkb|~", 0xc82efdf78201df87);

	return open_process_proc(desired_access, inherit_handle, process_id);
}
static inline int inject_library(HANDLE process, const wchar_t* dll)
{
	return inject_library_obf(process, dll, "E}mo|d[cefubWk~bgk",
		0x7c3371986918e8f6, "Rqbr`T{cnor{Bnlgwz",
		0x81bf81adc9456b35, "]`~wrl`KeghiCt",
		0xadc6a7b9acd73c9b, "Zh}{}agHzfd@{",
		0x57135138eb08ff1c, "DnafGhj}l~sX",
		0x350bfacdf81b2018);
}
static inline int inject_library_full(DWORD process_id, const wchar_t* dll)
{
	HANDLE process = open_process(PROCESS_ALL_ACCESS, false, process_id);
	int ret;

	if (process) {
		ret = inject_library(process, dll);
		CloseHandle(process);
	}
	else {
		ret = -2;
	}

	return ret;
}

int  main(int argc, char* argv[])
{
	WCHAR load_path[MAX_PATH] = L"";
	GetCurrentDirectoryW(MAX_PATH,  load_path);
	wcscat_s(load_path, L"\\");
	wcscat_s(load_path,   L"capture_hook64.dll");

	if (GetFileAttributesW( load_path) == INVALID_FILE_ATTRIBUTES)
	{
		wprintf(L"\nFailed to find hook_test64 at \"%s\"!\n",  load_path);
		return ERROR_FILE_NOT_FOUND;
	}

	inject_library_full( 27184, load_path);
	return 0;
}