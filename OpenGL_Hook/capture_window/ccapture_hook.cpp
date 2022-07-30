
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
#include "C:\Work\cabroad_server\Server\Robot\ccloud_rendering_c.h"
#include "cd3dxx.h"

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

uint32_t g_gpu_index;
// 系统环境变量
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


struct ccapture
{ 
	HANDLE handle;
	uint32_t width;
	uint32_t height;
	uint32_t count;
};
static struct ccapture capture = {0};

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

struct d3d11_capture
{
	cframe_video* cur_frame_ptr;
	cframe_video* old_frame_ptr;
};

static struct d3d11_capture   g_d3d11_capture_ptr = {0};


 

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

static FILE* out_file_log_ptr = ::fopen(g_ccapture_hook_file_name, "wb+");;
static inline void SHOW(const char* format, va_list args)
{

	if (!out_file_log_ptr)
	{
		return;
	}

	char message[1024] = {0};

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


void* get_shared()
{
	gl_video_data.handler = capture.handle;
	return &gl_video_data;
}


void g_send_video_callback()
{
	 
	{
		SYSTEMTIME t1;
		GetSystemTime(&t1);
		DEBUG_EX_LOG("start send cur = %u", t1.wMilliseconds);
	}
	if (g_d3d11_capture_ptr.cur_frame_ptr && g_d3d11_capture_ptr.cur_frame_ptr->capture_frame_ptr)
	{
		DEBUG_EX_LOG("send frame  ok !!!");
		c_cpp_rtc_video(g_d3d11_capture_ptr.cur_frame_ptr->capture_frame_ptr, g_d3d11_capture_ptr.cur_frame_ptr->width, g_d3d11_capture_ptr.cur_frame_ptr->height);
	}
	else
	{
		DEBUG_EX_LOG("send frame  failed  !!!");
	}
	 
	{
		SYSTEMTIME t1;
		GetSystemTime(&t1);
		DEBUG_EX_LOG("cur = %u", t1.wMilliseconds);
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
	/*if ( !gl_hooked)
	{

		gl_hooked = hook_gl();
		if (gl_hooked)
		{
			return true;
		}

	}*/
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
/// <summary>
/// 动态库的入口
/// </summary>
/// <param name="hinst"></param>
/// <param name="reason"></param>
/// <param name="unused1"></param>
/// <returns></returns>
static FILE* out_capture_log_ptr = NULL;
__declspec(dllexport)
BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD reason, LPVOID unused1)
{
	
	if (!out_capture_log_ptr)
	{
 
		out_capture_log_ptr = ::fopen("./capture_log_chensong.log", "wb+");
	}

 
	 
	if (out_capture_log_ptr)
	{
		::fprintf(out_capture_log_ptr, "[%s][%s][%d]\n", __FILE__, __FUNCTION__, __LINE__);
		::fflush(out_capture_log_ptr);
	}
 
	 
	if (reason == DLL_PROCESS_ATTACH) {
 
		wchar_t name[MAX_PATH];
		if (out_capture_log_ptr)
		{
			::fprintf(out_capture_log_ptr, "[%s][%s][%d]DLL_PROCESS_ATTACH\n", __FILE__, __FUNCTION__, __LINE__);
			::fflush(out_capture_log_ptr);
		}
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
		 
		if (out_capture_log_ptr)
		{
			::fprintf(out_capture_log_ptr, "[%s][%s][%d]DLL_PROCESS_DETACH\n", __FILE__, __FUNCTION__, __LINE__);
			::fflush(out_capture_log_ptr);
		}
		if (capture_thread) {
			stop_loop = true;
			WaitForSingleObject(capture_thread, 300);
			CloseHandle(capture_thread);
			 
		}
		if (out_capture_log_ptr)
		{
			::fprintf(out_capture_log_ptr, "[%s][%s][%d] DLL_PROCESS_ATTACH\n", __FILE__, __FUNCTION__, __LINE__);
			::fflush(out_capture_log_ptr);
		}
		//free_hook();
	}
	if (out_capture_log_ptr)
	{
		::fprintf(out_capture_log_ptr, "[%s][%s][%d]\n", __FILE__, __FUNCTION__, __LINE__);
		::fflush(out_capture_log_ptr);
	}
	(void)unused1;
	return true;
}

void capture_count(uint32_t count)
{
	capture.count = count;
}
void capture_init_shtex(HWND window, uint32_t cx, uint32_t cy, uint32_t format, HANDLE handle)
{
	DEBUG_EX_LOG("[width = %u][height = %u][format = %u][handle = %p]", cx, cy, format, handle);
	capture.handle =  handle;
	capture.width = cx;
	capture.height = cy;
}

void d3d11_capture_frame(unsigned char* rgba_ptr, uint32_t fmt, uint32_t width, uint32_t heigth)
{
	if (!g_d3d11_capture_ptr.old_frame_ptr)
	{
		g_d3d11_capture_ptr.old_frame_ptr = new cframe_video();
		if (!g_d3d11_capture_ptr.old_frame_ptr)
		{
			WARNING_EX_LOG("alloc struct frame data failed !!!");
			return;
		}
		g_d3d11_capture_ptr.old_frame_ptr->width=  width;
		g_d3d11_capture_ptr.old_frame_ptr->height = heigth;
		g_d3d11_capture_ptr.old_frame_ptr->capture_frame_ptr = new unsigned char[sizeof(unsigned char ) * width * heigth * 4];
		if (!g_d3d11_capture_ptr.old_frame_ptr->capture_frame_ptr)
		{
			delete g_d3d11_capture_ptr.old_frame_ptr;
			g_d3d11_capture_ptr.old_frame_ptr = NULL;
			WARNING_EX_LOG("alloc failed   frame data failed !!!");
			return;
		}
	}
	else if (g_d3d11_capture_ptr.old_frame_ptr->width != width || heigth != g_d3d11_capture_ptr.old_frame_ptr->height)
	{
		if (g_d3d11_capture_ptr.old_frame_ptr->capture_frame_ptr)
		{
			delete[] g_d3d11_capture_ptr.old_frame_ptr->capture_frame_ptr;
			g_d3d11_capture_ptr.old_frame_ptr->capture_frame_ptr = NULL;
		}
		g_d3d11_capture_ptr.old_frame_ptr->width = width;
		g_d3d11_capture_ptr.old_frame_ptr->height = heigth;
		g_d3d11_capture_ptr.old_frame_ptr->capture_frame_ptr = new unsigned char[sizeof(unsigned char) * width * heigth * 4];
		if (!g_d3d11_capture_ptr.old_frame_ptr->capture_frame_ptr)
		{
			delete g_d3d11_capture_ptr.old_frame_ptr;
			g_d3d11_capture_ptr.old_frame_ptr = NULL;
			WARNING_EX_LOG("reset size alloc failed   frame data failed !!!");
			return;
		}
	}
	g_d3d11_capture_ptr.old_frame_ptr->fmt = fmt;
	memcpy(g_d3d11_capture_ptr.old_frame_ptr->capture_frame_ptr, rgba_ptr, static_cast<size_t>(sizeof(unsigned char) * width * heigth * 4));
	void* ptr = g_d3d11_capture_ptr.cur_frame_ptr;
	g_d3d11_capture_ptr.cur_frame_ptr = g_d3d11_capture_ptr.old_frame_ptr;
	g_d3d11_capture_ptr.old_frame_ptr = (struct cframe_video*)ptr;
	DEBUG_EX_LOG("frame copy ok !!!");
	//std::swap(g_d3d11_capture_ptr.cur_frame_ptr, g_d3d11_capture_ptr.old_frame_ptr);
}

struct graphics_offsets* g_graphics_offsets = NULL;