#include "shared_handler.h"
#define COBJMACROS
#include <dxgi.h>
#include <d3d11.h>
#include "gl-decs.h"
#include <stdio.h>
#include <stdlib.h>

#define ERROR_EX_LOG printf
#define DEBUG_EX_LOG printf

static const GUID GUID_IDXGIFactory1 =
{ 0x770aae78, 0xf26f, 0x4dba, {0xa8, 0x29, 0x25, 0x3c, 0x83, 0xd1, 0xb3, 0x87} };
static const GUID GUID_IDXGIResource =
{ 0x035f3ab4, 0x482e, 0x4e50, {0xb4, 0x1f, 0x8a, 0x7f, 0x8b, 0xd8, 0x96, 0x0b} };
#define DUMMY_WINDOW_CLASS_NAME L"graphics_gl_dummy_window"

struct gl_data
{ 
	uint32_t cx;
	uint32_t cy;
	DXGI_FORMAT format;
	GLuint fbo; 

	bool using_shtex;
	struct
	{
		struct shtex_data* shtex_info;
		ID3D11Device* d3d11_device;
		ID3D11DeviceContext* d3d11_context;
		ID3D11Texture2D* d3d11_tex; 
		IDXGISwapChain* dxgi_swap;
		HANDLE gl_device;
		HANDLE gl_dxobj;
		HANDLE handle;
		HWND hwnd;
		GLuint texture;
	};
	bool capture_init;
};

static HMODULE gl = NULL;
static struct gl_data data = { 0 };
static bool nv_capture_available = false;

static bool functions_initialized = false;
char system_path[MAX_PATH] = { 0 };
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
static inline HMODULE load_system_library(const char* name)
{
	char base_path[MAX_PATH];
	HMODULE module;

	strcpy(base_path, system_path);
	strcat(base_path, "\\");
	strcat(base_path, name);

	module = GetModuleHandleA(base_path);
	if (module)
		return module;

	return LoadLibraryA(base_path);
}


static inline HMODULE get_system_module(const char* module)
{
	char base_path[MAX_PATH];

	strcpy(base_path, system_path);
	strcat(base_path, "\\");
	strcat(base_path, module);
	return GetModuleHandleA(base_path);
}

static inline bool gl_error(const char* func, const char* str)
{

	GLenum error = glGetError();
	if (error != 0)
	{
		//ERROR_EX_LOG("%s: %s: %lu", func, str, error);
		return true;
	}

	return false;
}



void gl_shared_destroy()
{
	if (data.using_shtex)
	{
		if (data.gl_dxobj)
		{
			jimglDXUnregisterObjectNV(data.gl_device,
				data.gl_dxobj);
		}
		if (data.gl_device)
			jimglDXCloseDeviceNV(data.gl_device);
		if (data.texture)
			glDeleteTextures(1, &data.texture);
		if (data.d3d11_tex)
			ID3D11Resource_Release(data.d3d11_tex);
		if (data.d3d11_context)
			ID3D11DeviceContext_Release(data.d3d11_context);
		if (data.d3d11_device)
			ID3D11Device_Release(data.d3d11_device);
		 
		if (data.dxgi_swap)
			IDXGISwapChain_Release(data.dxgi_swap);
		 
		if (data.hwnd)
			DestroyWindow(data.hwnd);
	}

	 

	if (data.fbo)
	{
		glDeleteFramebuffers(1, &data.fbo);
	}
	 
	gl_error("gl_free", "GL error occurred on free");
	//capture_init_shtex(NULL, 0, 0, 0, NULL);
	memset(&data, 0, sizeof(data));

	printf("------------------ gl capture freed ------------------");

}

static inline void* base_get_proc(const char* name)
{

	return (void*)GetProcAddress(gl, name);
}

static inline void* wgl_get_proc(const char* name)
{

	return (void*)jimglGetProcAddress(name);
}

static inline void* get_proc(const char* name)
{

	void* func = wgl_get_proc(name);
	if (!func)
		func = base_get_proc(name);

	return func;
}


void* get_d3d11_device_context(void* cur_d3d11)
{
	ID3D11DeviceContext* d3d11_context_ptr = NULL;
	ID3D11Device_GetImmediateContext((ID3D11Device*)cur_d3d11, &d3d11_context_ptr);
	return d3d11_context_ptr;
}



static void init_nv_functions(void)
{

	jimglDXSetResourceShareHandleNV =
		get_proc("wglDXSetResourceShareHandleNV");
	jimglDXOpenDeviceNV = get_proc("wglDXOpenDeviceNV");
	jimglDXCloseDeviceNV = get_proc("wglDXCloseDeviceNV");
	jimglDXRegisterObjectNV = get_proc("wglDXRegisterObjectNV");
	jimglDXUnregisterObjectNV = get_proc("wglDXUnregisterObjectNV");
	jimglDXObjectAccessNV = get_proc("wglDXObjectAccessNV");
	jimglDXLockObjectsNV = get_proc("wglDXLockObjectsNV");
	jimglDXUnlockObjectsNV = get_proc("wglDXUnlockObjectsNV");

	nv_capture_available =
		!!jimglDXSetResourceShareHandleNV && !!jimglDXOpenDeviceNV &&
		!!jimglDXCloseDeviceNV && !!jimglDXRegisterObjectNV &&
		!!jimglDXUnregisterObjectNV && !!jimglDXObjectAccessNV &&
		!!jimglDXLockObjectsNV && !!jimglDXUnlockObjectsNV;

	if (nv_capture_available)
		DEBUG_EX_LOG("Shared-texture OpenGL capture available");
}

#define GET_PROC(cur_func, ptr, func)                                      \
	do {                                                               \
		ptr = get_proc(#func);                                     \
		if (!ptr) {                                                \
			ERROR_EX_LOG("%s: failed to get function '%s'", #cur_func, \
			     #func);                                       \
			success = false;                                   \
		}                                                          \
	} while (false)



static bool init_gl_functions(void)
{

	bool success = true;

	jimglGetProcAddress = base_get_proc("wglGetProcAddress");
	if (!jimglGetProcAddress)
	{
		ERROR_EX_LOG("init_gl_functions: failed to get wglGetProcAddress");
		return false;
	}

	GET_PROC(init_gl_functions, jimglMakeCurrent, wglMakeCurrent);
	GET_PROC(init_gl_functions, jimglGetCurrentDC, wglGetCurrentDC);
	GET_PROC(init_gl_functions, jimglGetCurrentContext,
		wglGetCurrentContext);
	GET_PROC(init_gl_functions, glTexImage2D, glTexImage2D);
	GET_PROC(init_gl_functions, glReadBuffer, glReadBuffer);
	GET_PROC(init_gl_functions, glGetTexImage, glGetTexImage);
	GET_PROC(init_gl_functions, glDrawBuffer, glDrawBuffer);
	GET_PROC(init_gl_functions, glGetError, glGetError);
	GET_PROC(init_gl_functions, glBufferData, glBufferData);
	GET_PROC(init_gl_functions, glDeleteBuffers, glDeleteBuffers);
	GET_PROC(init_gl_functions, glDeleteTextures, glDeleteTextures);
	GET_PROC(init_gl_functions, glGenBuffers, glGenBuffers);
	GET_PROC(init_gl_functions, glGenTextures, glGenTextures);
	GET_PROC(init_gl_functions, glMapBuffer, glMapBuffer);
	GET_PROC(init_gl_functions, glUnmapBuffer, glUnmapBuffer);
	GET_PROC(init_gl_functions, glBindBuffer, glBindBuffer);
	GET_PROC(init_gl_functions, glGetIntegerv, glGetIntegerv);
	GET_PROC(init_gl_functions, glBindTexture, glBindTexture);
	GET_PROC(init_gl_functions, glGenFramebuffers, glGenFramebuffers);
	GET_PROC(init_gl_functions, glDeleteFramebuffers, glDeleteFramebuffers);
	GET_PROC(init_gl_functions, glBindFramebuffer, glBindFramebuffer);
	GET_PROC(init_gl_functions, glBlitFramebuffer, glBlitFramebuffer);
	GET_PROC(init_gl_functions, glFramebufferTexture2D,
		glFramebufferTexture2D);

	init_nv_functions();
	return success;
}
typedef HRESULT(WINAPI* create_dxgi_factory1_t)(REFIID, void**);

static const D3D_FEATURE_LEVEL feature_levels[] = {
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
};

static bool gl_register_window(void)
{

	WNDCLASSW wc = { 0 };
	wc.style = CS_OWNDC;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpfnWndProc = DefWindowProc;
	wc.lpszClassName = DUMMY_WINDOW_CLASS_NAME;

	if (!RegisterClassW(&wc))
	{
		ERROR_EX_LOG("gl_register_window: failed to register window class: %d",
			GetLastError());
		return false;
	}

	return true;
}
static inline bool gl_shtex_init_window(void)
{

	data.hwnd = CreateWindowExW(
		0, DUMMY_WINDOW_CLASS_NAME, L"Dummy GL window, ignore",
		WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 2, 2, NULL,
		NULL, GetModuleHandle(NULL), NULL);
	if (!data.hwnd)
	{
		ERROR_EX_LOG("gl_shtex_init_window: failed to create window: %d",
			GetLastError());
		return false;
	}

	return true;
}

static inline bool gl_shtex_init_d3d11(void)
{

	D3D_FEATURE_LEVEL level_used;
	IDXGIFactory1* factory;
	IDXGIAdapter* adapter;
	HRESULT hr;

	HMODULE d3d11 = load_system_library("d3d11.dll");
	if (!d3d11)
	{
		printf("gl_shtex_init_d3d11: failed to load D3D11.dll: %d",
			GetLastError());
		return false;
	}

	HMODULE dxgi = load_system_library("dxgi.dll");
	if (!dxgi)
	{
		printf("gl_shtex_init_d3d11: failed to load DXGI.dll: %d",
			GetLastError());
		return false;
	}

	DXGI_SWAP_CHAIN_DESC desc = { 0 };
	desc.BufferCount = 2;
	desc.BufferDesc.Format = data.format;
	desc.BufferDesc.Width = 2;
	desc.BufferDesc.Height = 2;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SampleDesc.Count = 1;
	desc.Windowed = true;
	desc.OutputWindow = data.hwnd;

	create_dxgi_factory1_t create_factory =
		(void*)GetProcAddress(dxgi, "CreateDXGIFactory1");
	if (!create_factory)
	{
		printf("gl_shtex_init_d3d11: failed to load CreateDXGIFactory1 "
			"procedure: %d",
			GetLastError());
		return false;
	}

	PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN create =
		(void*)GetProcAddress(d3d11, "D3D11CreateDeviceAndSwapChain");
	if (!create)
	{
		printf("gl_shtex_init_d3d11: failed to load "
			"D3D11CreateDeviceAndSwapChain procedure: %d",
			GetLastError());
		return false;
	}

	hr = create_factory(&GUID_IDXGIFactory1, (void**)&factory);
	if (FAILED(hr))
	{
		ERROR_EX_LOG("gl_shtex_init_d3d11: failed to create factory");
		return false;
	}
	//DEBUG_EX_LOG("set [gpu index = %u] ...", g_gpu_index);
	hr = IDXGIFactory1_EnumAdapters1(factory, 0,
		(IDXGIAdapter1**)&adapter);
	IDXGIFactory1_Release(factory);

	if (FAILED(hr))
	{
		//ERROR_EX_LOG("set gpu failed !!! [gpu_index = %u]gl_shtex_init_d3d11: failed to create adapter", g_gpu_index);
		return false;
	}
	//DEBUG_EX_LOG("set [gpu index = %u] ok ", g_gpu_index);
	hr = create(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, feature_levels,
		sizeof(feature_levels) / sizeof(D3D_FEATURE_LEVEL),
		D3D11_SDK_VERSION, &desc, &data.dxgi_swap,
		&data.d3d11_device, &level_used, &data.d3d11_context);
	IDXGIAdapter_Release(adapter);

	if (FAILED(hr))
	{
		//ERROR_EX_LOG("set gpu failed !!! [gpu_index = %u]gl_shtex_init_d3d11: failed to create device", g_gpu_index);
		return false;
	}

	return true;
}




static inline bool gl_shtex_init_d3d11_tex(void)
{
	IDXGIResource* dxgi_res;
	HRESULT hr;

	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = data.cx;
	desc.Height = data.cy;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = data.format;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE; 
	desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	hr = ID3D11Device_CreateTexture2D(data.d3d11_device, &desc, NULL, &data.d3d11_tex);
	if (FAILED(hr))
	{
		ERROR_EX_LOG("gl_shtex_init_d3d11_tex: failed to create texture");
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// 设置D3D11的同享GPU模式  
	hr = ID3D11Device_QueryInterface(data.d3d11_tex, &GUID_IDXGIResource,
		(void**)&dxgi_res);
	//>>>>>>> b5a2a73c8d165c99ef70c41948300d7e8e9bf805
	if (FAILED(hr))
	{
		ERROR_EX_LOG("gl_shtex_init_d3d11_tex: failed to create texture");
		return false;
	}
	hr = IDXGIResource_GetSharedHandle(dxgi_res, &data.handle);
	IDXGIResource_Release(dxgi_res);

	if (FAILED(hr))
	{
		ERROR_EX_LOG("gl_shtex_init_d3d11_tex: failed to get shared handle");
		return false;
	}


	return true;
}




static inline bool gl_shtex_init_gl_tex(void)
{

	//1. 把D3D11的资源转换为OpenGL的资源 
	data.gl_device = jimglDXOpenDeviceNV(data.d3d11_device);
	if (!data.gl_device)
	{
		ERROR_EX_LOG("gl_shtex_init_gl_tex: failed to open device");
		return false;
	}

	// 2. 得到OpenGL的纹理信息
	glGenTextures(1, &data.texture);
	//glGenBuffers(1, data.texture);
	if (gl_error("gl_shtex_init_gl_tex", "failed to generate texture"))
	{
		return false;
	}
	// 3. 把D3D11的纹理信息映射到OpenGL的纹理到到资源中去

	data.gl_dxobj = jimglDXRegisterObjectNV(data.gl_device, data.d3d11_tex, data.texture, GL_TEXTURE_2D, WGL_ACCESS_WRITE_DISCARD_NV);
	if (!data.gl_dxobj)
	{
		ERROR_EX_LOG("gl_shtex_init_gl_tex: failed to register object");


		return false;
	}

	return true;
}

static inline bool gl_init_fbo(void)
{

	glGenFramebuffers(1, &data.fbo);
	return !gl_error("gl_init_fbo", "failed to initialize FBO");
}






static bool gl_shtex_init( )
{
	//<<<<<<< HEAD
	// // 1. 这个窗口初始化 我没有看懂
	if (!gl_shtex_init_window())
	{
		return false;
	}
	//gl_shtex_init_window();
	// 2. 创建设备 与交换链
	if (!gl_shtex_init_d3d11())
	{
		return false;
	}
	// 3. 创建Texture 结构
	if (!gl_shtex_init_d3d11_tex())
	{
		return false;
	}
	// 4. d3d11 的设备映射到OpenGL中去 
	if (!gl_shtex_init_gl_tex())
	{
		return false;
	}
	if (!gl_init_fbo())
	{
		return false;
	}

	//capture_count(0);
//	capture_init_shtex(window, data.cx, data.cy, data.format, data.handle);
	DEBUG_EX_LOG("gl shared texture capture successful");
	return true;
}


#define INIT_SUCCESS 0
#define INIT_FAILED -1
#define INIT_SHTEX_FAILED -2


static int gl_init( )
{

	 
	int ret = INIT_FAILED;
	bool success = false;
	RECT rc = { 0 };

	 
	 
	 
	data.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	data.using_shtex = true;


	success = gl_shtex_init( );
	if (!success)
	{
		ret = INIT_SHTEX_FAILED;
	}


	if (!success)
	{
		//gl_free();
	}
	else
	{
		ret = INIT_SUCCESS;
	}

	return ret;
}

static void gl_copy_backbuffer(GLuint dst)
{

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, data.fbo);
	if (gl_error("gl_copy_backbuffer", "failed to bind FBO"))
	{
		return;
	}

	glBindTexture(GL_TEXTURE_2D, dst);
	if (gl_error("gl_copy_backbuffer", "failed to bind texture"))
	{
		return;
	}

	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, dst, 0);
	if (gl_error("gl_copy_backbuffer", "failed to set frame buffer"))
	{
		return;
	}

	glReadBuffer(GL_BACK);


	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	if (gl_error("gl_copy_backbuffer", "failed to set draw buffer"))
	{
		return;
	}
	// TODO@chensong 2022-05-16   OpenGL 指定顶点位置 复制  
	glBlitFramebuffer(0, data.cy, data.cx, 0, 0, 0, data.cx, data.cy,
		GL_COLOR_BUFFER_BIT, GL_LINEAR);
	gl_error("gl_copy_backbuffer", "failed to blit");
}



static void gl_shtex_capture(void)
{

	GLint last_fbo;
	GLint last_tex;
	// 1. 加锁 GPU的内存
	jimglDXLockObjectsNV(data.gl_device, 1, &data.gl_dxobj);

	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &last_fbo);
	if (gl_error("gl_shtex_capture", "failed to get last fbo"))
	{
		return;
	}

	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_tex);
	if (gl_error("gl_shtex_capture", "failed to get last texture"))
	{
		return;
	}

	gl_copy_backbuffer(data.texture);

	glBindTexture(GL_TEXTURE_2D, last_tex);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, last_fbo);


	jimglDXUnlockObjectsNV(data.gl_device, 1, &data.gl_dxobj);


	IDXGISwapChain_Present(data.dxgi_swap, 0, 0);
	 
	//++read_cpu;
	return;

}




void gl_shared_capture(uint32_t width, uint32_t height)
{
	static bool critical_failure = false;

	if (critical_failure)
	{
		return;
	}

	if (!functions_initialized)
	{
		functions_initialized = init_gl_functions();
		if (!functions_initialized)
		{
			critical_failure = true;
			return;
		}
	}

	/* reset error flag */
	glGetError();

	/*if (capture_should_stop()) {
		gl_free();
	}*/

	if (!data.capture_init)
	{
		data.capture_init = true;
		if (gl_init( ) == INIT_SHTEX_FAILED)
		{
			// error info 
			return;
		}
	}

	if (data.capture_init)
	{ 
		gl_shtex_capture();
		 
	}



}
   

bool gl_shared_init(uint32_t width, uint32_t height)
{
	if (data.capture_init)
	{
		return true;
	}
	gl_register_window();
	init_system_path();
	
	bool success = false;
	data.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	data.using_shtex = true;
	data.cx = width;
	data.cy = height;
	gl = get_system_module("opengl32.dll");
	if (!gl)
	{
		return false;
	}
	if (!functions_initialized)
	{
		functions_initialized = init_gl_functions();
		if (!functions_initialized)
		{
			 
		}
	}
	success = gl_shtex_init();
	if (!success)
	{
		//ret = INIT_SHTEX_FAILED;
	}

	data.capture_init = true;
	return 1;
}