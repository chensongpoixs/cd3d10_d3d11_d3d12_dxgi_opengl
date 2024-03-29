﻿
// 这个宏是非常重要的哈 否则会编译不过去的哈 ^_^
//#define COBJMACROS
#include <dxgi.h>
#include <d3d11.h>

#include <windows.h>
#include <psapi.h>
#include <inttypes.h>

#include <iostream>
#include <windows.h>
#include <psapi.h>
#include <inttypes.h>
#include "ccapture_hook.h"
#include "gl-capture.h"
#include "capture.h"
#include <chrono>
#include <string>
#include "ccloud_rendering_c.h"
#include "cd3dxx.h"
#include <ctime>
#include "include/detours.h"
#include <shellapi.h>
struct gl_read_video
{
	int ready;
	void* handler;
};
static struct gl_read_video  gl_video_data = { 0 };

static HINSTANCE dll_inst = NULL;
static volatile bool stop_loop = false;
static HANDLE capture_thread = NULL;
uint32_t g_run ;

//<<<<<<< HEAD
uint32_t g_gpu_index;
// ϵͳ��������
//=======
// 系统环境变量
//>>>>>>> b5a2a73c8d165c99ef70c41948300d7e8e9bf805
char system_path[MAX_PATH] = { 0 };
// 当前可执行程序名称
char process_name[MAX_PATH] = { 0 };
wchar_t keepalive_name[64] = { 0 };
HWND dummy_window = NULL;
//#pragma comment  (lib,"User32.lib")
//#pragma comment  (lib,"Gdi32.lib")
//#pragma comment  (lib,"d3d11.lib")
//#pragma comment  (lib,"d3dcompiler.lib")
//#pragma comment  (lib,"dxgi.lib")//d3dcompiler.lib


//struct ccapture
//{ 
//	HANDLE handle;
//	uint32_t width;
//	uint32_t height;
//	uint32_t count;
//};
//static struct ccapture capture = {0};

struct cframe_video
{
	unsigned char* capture_frame_ptr;
	uint32_t fmt;
	uint32_t width;
	uint32_t height;
	
	cframe_video()
		:capture_frame_ptr(NULL)
		, fmt(0)
		, width(0)
		, height(0)
		
	{
	}
};

struct ccapture
{
	cframe_video* cur_frame_ptr;
	cframe_video* old_frame_ptr;
	HANDLE handle;
	uint32_t fmt;
	uint32_t width;
	uint32_t height;
	uint32_t count;
	ccapture()
		: cur_frame_ptr(NULL)
		, old_frame_ptr(NULL)
		, handle(NULL)
		, fmt(0)
		, width(0)
		, height(0)
		, count(0)
	{
	}
};

static struct ccapture   g_capture_ptr;


 

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
		DEBUG_EX_LOG("Failed to get windows system path: %lu\n", GetLastError());
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

	if (!RegisterClass(&wc)) {
		DEBUG_EX_LOG("Failed to create temp D3D window class: %lu",
			GetLastError());
		return 0;
	}

	dummy_window = CreateWindowExW(0, dummy_window_class, L"Temp Window",
		DEF_FLAGS, 0, 0, 1, 1, NULL, NULL,
		dll_inst, NULL);
	if (!dummy_window) {
		DEBUG_EX_LOG("Failed to create temp D3D window: %lu", GetLastError());
		return 0;
	}

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	(void)unused;
	return 0;
}
 
static const std::string  g_ccapture_hook_file_name = "./capture_hook/" + std::to_string(::time(NULL)) + "_ccapture_hook.log";

static FILE* out_file_log_ptr = ::fopen(g_ccapture_hook_file_name.c_str(), "wb+");;
static inline void SHOW(const char* format, va_list args)
{

	if (!out_file_log_ptr)
	{
		return;
	}

	char message[10240] = {0};

	int num = _vsprintf_p(message, 1024, format, args);
	if (num > 0)
	{
		::fprintf(out_file_log_ptr, "%s\n", message);
		::fflush(out_file_log_ptr);
	}
}
void LOG(const char* format, ...)
{
	if (!out_file_log_ptr)
	{
		return;
	}
	
		va_list args;
		va_start(args, format);

		SHOW(format, args);
		va_end(args);
	
}
 

static inline void init_dummy_window_thread(void)
{
	 
	HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)dummy_window_thread, NULL, 0, NULL);
	if (!thread) 
	{
		DEBUG_EX_LOG("Failed to create temp D3D window thread: %lu",
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
		DEBUG_EX_LOG("capture_hook.dll loaded against process: %s", process_name);
		 
	}
	else {
		DEBUG_EX_LOG("capture_hook.dll loaded");
	}
	 
	DEBUG_EX_LOG("(half life scientist) everything..  seems to be in order");
}

//#include "C:\Work\mediaserver\webrtc_native\video-sdk-samples\Samples\AppEncode\AppEncD3D11/AppEncD3D11.h"
//C:\Work\mediaserver\webrtc_native\video-sdk-samples\Samples\x64.Release\AppEncD3D11.lib
static inline bool init_hook(HANDLE thread_handle)
{ 
	if (c_init())
	{
		g_run = 0;
		c_set_set_gpu_index_callback(&g_set_gpu_index_callback);
		c_startup();
	}
	init_dummy_window_thread();
	log_current_process();
	 
	return true;
}


bool open_shared_d3d11_texture(ID3D11Device* device, uintptr_t handler, ID3D11Texture2D* d3d11_texture)
{
	HRESULT hr;
	hr = device->OpenSharedResource((HANDLE)(uintptr_t)handler, __uuidof(ID3D11Texture2D), (void**)&d3d11_texture);
	//ID3D11Device_OpenSharedResource();
	if (FAILED(hr))
	{
		DEBUG_EX_LOG("open_shared_d3d11_texture: open shared handler  failed  !!!");
		return false;
	}
	return true;
}

void g_set_gpu_index_callback(uint32_t gpu_index)
{
	g_run = 10;
	g_gpu_index = gpu_index;
	DEBUG_EX_LOG("set gpu index callback --> gpu_index = %u", g_gpu_index);
}


//void* get_shared()
//{
//	gl_video_data.handler = capture.handle;
//	return &gl_video_data;
//}


void g_send_video_callback()
{
	//return;
	if (g_capture_ptr.count != 0)
	{
		{
			SYSTEMTIME t1;
			GetSystemTime(&t1);
			DEBUG_EX_LOG("start send cur = %u", t1.wMilliseconds);
		}
		if (0 != g_gpu_index /*|| (g_capture_ptr.cur_frame_ptr && g_capture_ptr.cur_frame_ptr->fmt == DXGI_FORMAT_R10G10B10A2_UNORM)*/)
		{
			if (g_capture_ptr.cur_frame_ptr && g_capture_ptr.cur_frame_ptr->capture_frame_ptr)
			{
				DEBUG_EX_LOG("cpu send frame  ok !!!");
				c_cpp_rtc_video(g_capture_ptr.cur_frame_ptr->capture_frame_ptr, g_capture_ptr.cur_frame_ptr->fmt, g_capture_ptr.cur_frame_ptr->width, g_capture_ptr.cur_frame_ptr->height);
			}
			else
			{
				DEBUG_EX_LOG("send frame  failed  !!!");
			}
		}
		else
		{
			DEBUG_EX_LOG("shared gpu send frame [g_capture_ptr.handle = %p][g_capture_ptr.fmt = %u][g_capture_ptr.width = %u][g_capture_ptr.height = %u] ok !!!", g_capture_ptr.handle, g_capture_ptr.fmt, g_capture_ptr.width, g_capture_ptr.height);
			cpp_rtc_texture(g_capture_ptr.handle, g_capture_ptr.fmt, g_capture_ptr.width, g_capture_ptr.height);
		}

		{
			SYSTEMTIME t1;
			GetSystemTime(&t1);
			DEBUG_EX_LOG("cur = %u", t1.wMilliseconds);
		}

	}
	 

	
	
	
}




static inline bool d3d9_hookable(void)
{
	return !!g_graphics_offsets->d3d9.present &&
		!!g_graphics_offsets->d3d9.present_ex &&
		!!g_graphics_offsets->d3d9.present_swap;
}
static inline bool dxgi_hookable(void)
{
	return !!g_graphics_offsets->dxgi.present &&
		!!g_graphics_offsets->dxgi.resize;
}
static inline bool attempt_hook(void)
{
	
	c_cpp_graphics_offsets((void **)&g_graphics_offsets);
	if (!g_graphics_offsets)
	{
		WARNING_EX_LOG("not graphics_offsets get failed !!!");
		return false;
	}
	static bool gl_hooked = false;
	static bool d3d9_hooked = false;
	static bool d3d12_hooked = false;
	static bool dxgi_hooked = false;
	//DEBUG_EX_LOG("", g_graphics_offsets->dxgi.present);
	//DEBUG_EX_LOG("[d3d8]\n");
	//DEBUG_EX_LOG("present=0x%" PRIx32 "\n", g_graphics_offsets->d3d8.present);
	//DEBUG_EX_LOG("[d3d9]\n");
	//DEBUG_EX_LOG("present=0x%" PRIx32 "\n", g_graphics_offsets->d3d9.present);
	//DEBUG_EX_LOG("present_ex=0x%" PRIx32 "\n", g_graphics_offsets->d3d9.present_ex);
	//DEBUG_EX_LOG("present_swap=0x%" PRIx32 "\n", g_graphics_offsets->d3d9.present_swap);
	//DEBUG_EX_LOG("d3d9_clsoff=0x%" PRIx32 "\n", g_graphics_offsets->d3d9.d3d9_clsoff);
	//DEBUG_EX_LOG("is_d3d9ex_clsoff=0x%" PRIx32 "\n", g_graphics_offsets->d3d9.is_d3d9ex_clsoff);
	//DEBUG_EX_LOG("[dxgi]\n");
	//DEBUG_EX_LOG("present=0x%" PRIx32 "\n", g_graphics_offsets->dxgi.present);
	//DEBUG_EX_LOG("present1=0x%" PRIx32 "\n", g_graphics_offsets->dxgi.present1);
	//DEBUG_EX_LOG("resize=0x%" PRIx32 "\n", g_graphics_offsets->dxgi.resize);
	//DEBUG_EX_LOG("release=0x%" PRIx32 "\n", g_graphics_offsets->dxgi2.release);
	/*if (!d3d9_hooked)
	{
		if (!d3d9_hookable())
		{
			WARNING_EX_LOG("  no D3D9 hook address found!\n");
			d3d9_hooked = true;
		}
		else
		{
			d3d9_hooked = hook_d3d9();
			if (d3d9_hooked)
			{
				return true;
			}
		}
	}*/
	if (!d3d12_hooked) 
	{
		d3d12_hooked = hook_d3d12();
	}
	if (!dxgi_hooked)
	{
		if (!dxgi_hookable())
		{
			WARNING_EX_LOG("  no DXGI hook address found!\n");
			dxgi_hooked = true;
		}
		else
		{
			dxgi_hooked = hook_dxgi();
			if (dxgi_hooked)
			{
				return true;
			}
		}
	}
	 if ( !gl_hooked)
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
	//Sleep(4);;
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

		ERROR_EX_LOG(  "[%s][%d][capture] Failed to init hook\n", __FUNCTION__, __LINE__);
		return 0;
	}

	while (g_run == 0)
	{
		Sleep(5);
		DEBUG_EX_LOG("sleep [g_run = %d][g_gpu_index = %u] ...", g_run, g_gpu_index);
	}
	DEBUG_EX_LOG("[g_run = %d][g_gpu_index = %u]", g_run, g_gpu_index);

	// 2. 开始抓取当前主程序的窗口
	capture_loop();
	return 0;
}

static  HMODULE  userdll32_ptr = NULL;

typedef BOOL(WINAPI* PFN_EnumDisplaySettingsA)(_In_opt_ LPCSTR lpszDeviceName, _In_ DWORD iModeNum, _Inout_ DEVMODEA* lpDevMode);
PFN_EnumDisplaySettingsA RealEnumDisplaySettingsA;
typedef BOOL(WINAPI* PFN_EnumDisplaySettingsW)(_In_opt_ LPCSTR lpszDeviceName, _In_ DWORD iModeNum, _Inout_ DEVMODEA* lpDevMode);
PFN_EnumDisplaySettingsW RealEnumDisplaySettingsW;

std::string WcharTochar(const std::wstring& wp, size_t m_encode = CP_ACP)
{
	std::string str;
	int32_t len = WideCharToMultiByte(m_encode, 0, wp.c_str(), wp.size(), NULL, 0, NULL, NULL);
	str.resize(len);
	WideCharToMultiByte(m_encode, 0, wp.c_str(), wp.size(), (LPSTR)(str.data()), len, NULL, NULL);
	return str;
}
static BOOL hook_EnumDisplaySettingsA(_In_opt_ LPCSTR lpszDeviceName, _In_ DWORD iModeNum, _Inout_ DEVMODEA* lpDevMode)
{
	 
	BOOL ret = RealEnumDisplaySettingsA(lpszDeviceName, iModeNum, lpDevMode);
	if (ret)
	{
		//::GetCommandLineA();
		LPWSTR* szArglist;
		int nArgs;
		szArglist = ::CommandLineToArgvW(GetCommandLineW(), &nArgs);
		int32_t width = 1920;
		int32_t height = 1080;
		if (nArgs > 14)
		{
			// 0 0 1920 1040
			// 11 12 13 14
			width = ::atoi((const char*)(WcharTochar(szArglist[13]).c_str()));
			height = ::atoi((const char*)(WcharTochar(szArglist[14]).c_str()));
			//NORMAL_EX_LOG("width  = %u, height = %u", width, height);
			lpDevMode->dmPelsWidth = width;
			lpDevMode->dmPelsHeight = height;
		}

	}
	return ret;
}
static BOOL hook_EnumDisplaySettingsW(_In_opt_ LPCSTR lpszDeviceName, _In_ DWORD iModeNum, _Inout_ DEVMODEA* lpDevMode)
{
	 
	BOOL ret = RealEnumDisplaySettingsW(lpszDeviceName, iModeNum, lpDevMode);
	if (ret)
	{
		LPWSTR* szArglist;
		int nArgs;
		szArglist = ::CommandLineToArgvW(GetCommandLineW(), &nArgs);
		int32_t width = 1920;
		int32_t height = 1080;

		if (nArgs > 14)
		{
			// 0 0 1920 1040
			// 11 12 13 14
			width = ::atoi((const char*)(WcharTochar(szArglist[13]).c_str()));
			height = ::atoi((const char*)(WcharTochar(szArglist[14]).c_str()));
			//NORMAL_EX_LOG("width  = %u, height = %u", width, height);
			lpDevMode->dmPelsWidth = width;
			lpDevMode->dmPelsHeight = height;
		}

	}
	return ret;
}
void load_seecen()
{
	if (userdll32_ptr)
	{
		return;
	}
	userdll32_ptr = get_system_module("user32.dll");
	if (!userdll32_ptr)
	{
		return;
	}
	void* EnumDisplaySettingsA_proc = GetProcAddress(userdll32_ptr, "EnumDisplaySettingsA");
	void* EnumDisplaySettingsW_proc = GetProcAddress(userdll32_ptr, "EnumDisplaySettingsW");
	{
	//	SYSTEM_LOG("    input device  begin ... ");
		DetourTransactionBegin();

		if (EnumDisplaySettingsA_proc)
		{
			RealEnumDisplaySettingsA = (PFN_EnumDisplaySettingsA)EnumDisplaySettingsA_proc;
			DetourAttach((PVOID*)&RealEnumDisplaySettingsA,
				hook_EnumDisplaySettingsA);
		}
		if (EnumDisplaySettingsW_proc)
		{
			RealEnumDisplaySettingsW = (PFN_EnumDisplaySettingsW)EnumDisplaySettingsW_proc;
			DetourAttach((PVOID*)&RealEnumDisplaySettingsW,
				hook_EnumDisplaySettingsW);
		} 
		const LONG error = DetourTransactionCommit();
		const bool success = error == NO_ERROR;
	}
}
/// <summary>
/// 动态库的入口
/// </summary>
/// <param name="hinst"></param>
/// <param name="reason"></param>
/// <param name="unused1"></param>
/// <returns></returns>
/// 
/// 
/// 
 
__declspec(dllexport)
BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD reason, LPVOID unused1)
{
	
	 
 
	 
	if (reason == DLL_PROCESS_ATTACH) 
	{
 
		//load_seecen();
		wchar_t name[MAX_PATH];
		 
		dll_inst = hinst;

		HANDLE cur_thread;
		bool success = DuplicateHandle(GetCurrentProcess(),
			GetCurrentThread(),
			GetCurrentProcess(), &cur_thread,
			SYNCHRONIZE, false, 0);

		if (!success)
		{
			DEBUG_EX_LOG("[OBS] Failed to get current thread handle\n");
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
			WaitForSingleObject(capture_thread, 200);
			CloseHandle(capture_thread);
			 
		}
		 
		//free_hook();
	}
	 
	(void)unused1;
	return true;
}

void capture_count(uint32_t count)
{
	DEBUG_EX_LOG("[count = %u]", count);
	 g_capture_ptr.count = count;
	 
}
void capture_init_shtex(HWND window, uint32_t cx, uint32_t cy, uint32_t format, HANDLE handle)
{
	if (handle && (cx < 100 || cy < 100))
	{
		WARNING_EX_LOG("[width = %u][height = %u][format = %u][handle = %p]", cx, cy, format, handle);
		return;
	}
	DEBUG_EX_LOG("[width = %u][height = %u][format = %u][handle = %p]", cx, cy, format, handle);
	capture_count(0);
	if (window)
	{
		cpp_set_main_window(window);
	}
	
	g_capture_ptr.handle =  handle;
	g_capture_ptr.fmt = format;
	g_capture_ptr.width = cx;
	g_capture_ptr.height = cy;
}

void d3d11_capture_frame(unsigned char* rgba_ptr, uint32_t fmt, uint32_t row_pitch, uint32_t depth_pitch, uint32_t width, uint32_t heigth)
{
	if (!g_capture_ptr.old_frame_ptr)
	{
		g_capture_ptr.old_frame_ptr = new cframe_video();
		if (!g_capture_ptr.old_frame_ptr)
		{
			WARNING_EX_LOG("alloc struct frame data failed !!!");
			return;
		}
		g_capture_ptr.old_frame_ptr->width=  width;
		g_capture_ptr.old_frame_ptr->height = heigth;
		g_capture_ptr.old_frame_ptr->capture_frame_ptr = new unsigned char[sizeof(unsigned char ) * width * heigth * 4];
		if (!g_capture_ptr.old_frame_ptr->capture_frame_ptr)
		{
			delete g_capture_ptr.old_frame_ptr;
			g_capture_ptr.old_frame_ptr = NULL;
			WARNING_EX_LOG("alloc failed   frame data failed !!!");
			return;
		}
	}
	else if (g_capture_ptr.old_frame_ptr->width != width || heigth != g_capture_ptr.old_frame_ptr->height)
	{
		if (g_capture_ptr.old_frame_ptr->capture_frame_ptr)
		{
			delete[] g_capture_ptr.old_frame_ptr->capture_frame_ptr;
			g_capture_ptr.old_frame_ptr->capture_frame_ptr = NULL;
		}
		g_capture_ptr.old_frame_ptr->width = width;
		g_capture_ptr.old_frame_ptr->height = heigth;
		g_capture_ptr.old_frame_ptr->capture_frame_ptr = new unsigned char[sizeof(unsigned char) * width * heigth * 4];
		if (!g_capture_ptr.old_frame_ptr->capture_frame_ptr)
		{
			delete g_capture_ptr.old_frame_ptr;
			g_capture_ptr.old_frame_ptr = NULL;
			WARNING_EX_LOG("reset size alloc failed   frame data failed !!!");
			return;
		}
	}
	g_capture_ptr.old_frame_ptr->fmt = fmt;

	if (false /*DXGI_FORMAT_R10G10B10A2_UNORM == fmt*/)
	{
		//DXGI_FORMAT_R10G10B10A2_UNORM ===>DXGI_FORMAT_B8G8R8A8_UNORM [A8R8G8B8]
		//[1111 1111] [1122 2222][2222 3333][3333 3344]
		// 
		//[1111 1111] [2222 2222][3333 3333][4444 4444]
		// ///
		//[4444 4444] [1111 1111][2222 2222][3333 3333]
		for (int y = 0; y < heigth; ++y)
		{
			for (int x = 0; x <  width; ++x)
			{
				const size_t host_data_index = y * (width *4 ) + x * 4;
				const size_t frame_data_index = y * (width  * 4)/*data.frame->linesize[0]*/ + x * 4;

				g_capture_ptr.old_frame_ptr->capture_frame_ptr[frame_data_index + 1] = (static_cast<const uint8_t*>(rgba_ptr)[host_data_index + 0]);
				g_capture_ptr.old_frame_ptr->capture_frame_ptr[frame_data_index + 2] = (static_cast<const uint8_t*>(rgba_ptr)[host_data_index + 1]>>2);
				g_capture_ptr.old_frame_ptr->capture_frame_ptr[frame_data_index + 3] = (static_cast<const uint8_t*>(rgba_ptr)[host_data_index + 2]>>4);
				g_capture_ptr.old_frame_ptr->capture_frame_ptr[frame_data_index + 0] = ((static_cast<const uint8_t*>(rgba_ptr)[host_data_index + 3] >> 4)<<4);
			}
		}
	}
	else
	{
		memcpy(g_capture_ptr.old_frame_ptr->capture_frame_ptr, rgba_ptr, static_cast<size_t>(sizeof(unsigned char) * width * heigth * 4));
	}
	
	void* ptr = g_capture_ptr.cur_frame_ptr;
	g_capture_ptr.cur_frame_ptr = g_capture_ptr.old_frame_ptr;
	g_capture_ptr.old_frame_ptr = (struct cframe_video*)ptr;
	DEBUG_EX_LOG("frame copy ok !!!");
	//std::swap(g_d3d11_capture_ptr.cur_frame_ptr, g_d3d11_capture_ptr.old_frame_ptr);
}

struct graphics_offsets* g_graphics_offsets = NULL;