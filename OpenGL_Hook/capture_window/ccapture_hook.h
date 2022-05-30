/********************************************************************
created:	2022-05-08

author:		chensong
 

purpose:	capture hook 
*********************************************************************/


#ifndef _C_CAPTURE_HOOK_H_
#define _C_CAPTURE_HOOK_H_
#ifdef __cplusplus
extern "C" {
#else
#if defined(_MSC_VER) && !defined(inline)
#define inline __inline
#endif
#endif

#define NUM_BUFFERS (3)
#define HOOK_VERBOSE_LOGGING 1

//一秒显示多少帧图片
#define FRAME_SUM (1000/60)

static const char* g_ccapture_hook_file_name = "./capture_hook_log/ccapture_hook";

void LOG(const char* format, ...);

//#define WARNING_EX_LOG(format, ...)	WARNING_LOG("[%s][%d]" format, FUNCTION, __LINE__, ##__VA_ARGS__)
#define DEBUG_EX_LOG(format, ...)   LOG("[%s][%d][debug]" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define ERROR_EX_LOG(format, ...)   LOG("[%s][%d][error]" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)


extern char system_path[MAX_PATH];
extern char process_name[MAX_PATH];
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

void g_send_video_callback();
//extern   bool open_shared_d3d11_texture(ID3D11Device* device, uintptr_t handler, ID3D11Texture2D* d3d11_texture);

//bool hook_gl(void);

#ifdef __cplusplus




}
#endif

#endif // _C_CAPTURE_HOOK_H_