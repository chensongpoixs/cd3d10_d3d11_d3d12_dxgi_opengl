/********************************************************************
created:	2022-05-08

author:		chensong
 

purpose:	capture hook 
*********************************************************************/


#ifndef _C_CAPTURE_HOOK_H_
#define _C_CAPTURE_HOOK_H_


#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>



#pragma pack(push, 8)

struct d3d8_offsets
{
	uint32_t present;
};

struct d3d9_offsets
{
	uint32_t present;
	uint32_t present_ex;
	uint32_t present_swap;
	uint32_t d3d9_clsoff;
	uint32_t is_d3d9ex_clsoff;
};

struct d3d12_offsets
{
	uint32_t execute_command_lists;
};

struct dxgi_offsets
{
	uint32_t present;
	uint32_t resize;

	uint32_t present1;
};

struct dxgi_offsets2
{
	uint32_t release;
};

struct ddraw_offsets
{
	uint32_t surface_create;
	uint32_t surface_restore;
	uint32_t surface_release;
	uint32_t surface_unlock;
	uint32_t surface_blt;
	uint32_t surface_flip;
	uint32_t surface_set_palette;
	uint32_t palette_set_entries;
};




struct graphics_offsets
{
	struct d3d8_offsets d3d8;
	struct d3d9_offsets d3d9;
	struct dxgi_offsets dxgi;
	struct ddraw_offsets ddraw;
	struct dxgi_offsets2 dxgi2;
	struct d3d12_offsets d3d12;
};



#pragma pack(pop)



extern struct graphics_offsets* g_graphics_offsets;



//#include <iostream>
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

#define NUM_BUFFERS (3)

static const char* g_ccapture_hook_file_name = "ccapture_hook.log";

void LOG(const char* format, ...);

#define WARNING_EX_LOG(format, ...)	LOG("[%s][%d][warn]" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define DEBUG_EX_LOG(format, ...)   LOG("[%s][%d][debug]" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define ERROR_EX_LOG(format, ...)   LOG("[%s][%d][error]" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)


extern char system_path[MAX_PATH];
extern char process_name[MAX_PATH];
extern HWND dummy_window;



extern void d3d10_capture(void* swap, void* backbuffer);
extern void d3d10_free(void);
extern void d3d11_capture(void* swap, void* backbuffer);
extern void d3d11_free(void);

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


static inline void* get_offset_addr(HMODULE module, uint32_t offset)
{
	return (void*)((uintptr_t)module + (uintptr_t)offset);
}

void g_send_video_callback();
//extern   bool open_shared_d3d11_texture(ID3D11Device* device, uintptr_t handler, ID3D11Texture2D* d3d11_texture);

//bool hook_gl(void);

 

#ifdef __cplusplus




}
#endif

#endif // _C_CAPTURE_HOOK_H_