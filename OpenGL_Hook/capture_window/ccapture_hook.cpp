

#include <iostream>
#include <windows.h>
#include <psapi.h>
#include <inttypes.h>
#include "ccapture_hook.h"
#include "gl-capture.h"
#include "capture.h"

#include "C:\Work\cabroad_server\Server\Robot\ccloud_rendering_c.h"

static HINSTANCE dll_inst = NULL;
static volatile bool stop_loop = false;
static HANDLE capture_thread = NULL;

// 系统环境变量
char system_path[MAX_PATH] = { 0 };
// 当前可执行程序名称
char process_name[MAX_PATH] = { 0 };
wchar_t keepalive_name[64] = { 0 };
HWND dummy_window = NULL;
#pragma comment  (lib,"User32.lib")
#pragma comment  (lib,"Gdi32.lib")
#pragma comment  (lib,"d3d11.lib")
#pragma comment  (lib,"d3dcompiler.lib")
#pragma comment  (lib,"dxgi.lib")//d3dcompiler.lib

bool  startup_capture_send_video_thread()
{
	
	return true;
}
/// <summary>
///  获取当前系统环境变量
/// </summary>
/// <param name=""></param>
/// <returns></returns>
static inline bool init_system_path(void)
{
	 
	 
	UINT ret = GetSystemDirectoryA(system_path, MAX_PATH);
	if (!ret) {
		printf("Failed to get windows system path: %lu\n", GetLastError());
		return false;
	}

	return true;
}


#define DEF_FLAGS (WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS)

// 注册顶层 窗口消息事件
static DWORD WINAPI dummy_window_thread(LPVOID *unused)
{
	 
	static const wchar_t dummy_window_class[] = L"temp_d3d_window_4039785";
	WNDCLASSW wc;
	MSG msg;

	memset(&wc, 0, sizeof(wc));
	wc.style = CS_OWNDC;
	wc.hInstance = dll_inst;
	wc.lpfnWndProc = (WNDPROC)DefWindowProc;
	wc.lpszClassName = dummy_window_class;

	if (!RegisterClass( &wc)) {
		printf("Failed to create temp D3D window class: %lu",
			GetLastError());
		return 0;
	}

	dummy_window = CreateWindowExW(0, dummy_window_class, L"Temp Window",
		DEF_FLAGS, 0, 0, 1, 1, NULL, NULL,
		dll_inst, NULL);
	if (!dummy_window) {
		printf("Failed to create temp D3D window: %lu", GetLastError());
		return 0;
	}

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	(void)unused;
	return 0;
}


static inline void init_dummy_window_thread(void)
{
	 
	HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)dummy_window_thread, NULL, 0, NULL);
	if (!thread) 
	{
		printf("Failed to create temp D3D window thread: %lu",
			GetLastError());
		 
		return;
	}

	CloseHandle(thread);
}

static inline void log_current_process(void)
{
	 
	DWORD len = GetModuleBaseNameA(GetCurrentProcess(), NULL, process_name,
		MAX_PATH);
	if (len > 0) {
		process_name[len] = 0;
		printf("capture_hook.dll loaded against process: %s",
			process_name);
		 
	}
	else {
		printf("capture_hook.dll loaded");
	}
	 
	printf("(half life scientist) everything..  seems to be in order");
}


static inline bool init_hook(HANDLE thread_handle)
{ 
	if (c_init())
	{
		c_startup();
	}
	init_dummy_window_thread();
	log_current_process();
	 
	return true;
}



static inline bool attempt_hook(void)
{
	

	static bool gl_hooked = false;


	if (!gl_hooked) 
	{
		
		gl_hooked = hook_gl();
		if (gl_hooked) 
		{
			return true;
		}
		
	}

	return false;
}

static inline void capture_loop(void)
{
	Sleep(4);;
	while (!attempt_hook())
	{
		Sleep(40);
	}

	for (size_t n = 0; !stop_loop; ++n)
	{
		/* this causes it to check every 4 seconds, but still with
		 * a small sleep interval in case the thread needs to stop */
		if (n % 100 == 0)
		{
			attempt_hook();
		}
		Sleep(40);
	}
}
static DWORD WINAPI main_capture_thread(HANDLE thread_handle)
{
	// 1. 窗口事件检查
	if (!init_hook(thread_handle)) 
	{
		printf(  "[%s][%d][OBS] Failed to init hook\n", __FUNCTION__, __LINE__);
		 
		return 0;
	}
	// 2. 开始抓取当前主程序的窗口
	capture_loop();
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID unused1)
{
	

	 
	if (reason == DLL_PROCESS_ATTACH) {
		wchar_t name[MAX_PATH];

		dll_inst = hinst;


		HANDLE cur_thread;
		bool success = DuplicateHandle(GetCurrentProcess(),
			GetCurrentThread(),
			GetCurrentProcess(), &cur_thread,
			SYNCHRONIZE, false, 0);

		if (!success)
		{
			printf("[OBS] Failed to get current thread handle\n");
		}

		 
		if (!init_system_path()) {
			return false;
		}
		

		/* this prevents the library from being automatically unloaded
		 * by the next FreeLibrary call */
		GetModuleFileNameW(hinst, name, MAX_PATH);
		LoadLibraryW(name);
		//init();
		capture_thread = CreateThread(
			NULL, 0, (LPTHREAD_START_ROUTINE)main_capture_thread,
			(LPVOID)cur_thread, 0, 0);
		if (!capture_thread) {
			CloseHandle(cur_thread);
			return false;
		}


		

	}
	else if (reason == DLL_PROCESS_DETACH) {
		 

		if (capture_thread) {
			stop_loop = true;
			WaitForSingleObject(capture_thread, 300);
			CloseHandle(capture_thread);
			 
		}

		//free_hook();
	}

	(void)unused1;
	return true;
}