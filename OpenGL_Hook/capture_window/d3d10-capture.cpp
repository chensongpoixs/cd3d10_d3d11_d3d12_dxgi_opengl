#include <d3d10.h>
#include <dxgi.h>

#include "dxgi-helpers.hpp"
//#include "graphics-hook.h"
#include "ccapture_hook.h"
#include <iostream>
#include "ccloud_rendering_c.h"
struct d3d10_data {
	ID3D10Device *device; /* do not release */
	uint32_t cx;
	uint32_t cy;
	DXGI_FORMAT format;
	bool using_shtex;
	bool multisampled;

	ID3D10Texture2D *scale_tex;
	ID3D10ShaderResourceView *scale_resource;

	ID3D10VertexShader *vertex_shader;
	ID3D10InputLayout *vertex_layout;
	ID3D10PixelShader *pixel_shader;

	ID3D10SamplerState *sampler_state;
	ID3D10BlendState *blend_state;
	ID3D10DepthStencilState *zstencil_state;
	ID3D10RasterizerState *raster_state;

	ID3D10Buffer *vertex_buffer;
	bool init_capture;
	union {
		/* shared texture */
		struct {
			struct shtex_data *shtex_info;
			ID3D10Texture2D *texture;
			HANDLE handle;
		};
		
	};
	int write_tick_count;
};

static struct d3d10_data data = {};

void d3d10_free(void)
{
	capture_count(0);
	capture_init_shtex(NULL, 0, 0, 0, NULL);
	if (data.scale_tex)
	{
		data.scale_tex->Release();
	}
	if (data.scale_resource)
	{
		data.scale_resource->Release();
	}
	if (data.vertex_shader)
	{
		data.vertex_shader->Release();
	}
	if (data.vertex_layout)
	{
		data.vertex_layout->Release();
	}
	if (data.pixel_shader)
	{
		data.pixel_shader->Release();
	}
	if (data.sampler_state)
	{
		data.sampler_state->Release();
	}
	if (data.blend_state)
	{
		data.blend_state->Release();
	}
	if (data.zstencil_state)
	{
		data.zstencil_state->Release();
	}
	if (data.raster_state)
	{
		data.raster_state->Release();
	}
	if (data.vertex_buffer)
	{
		data.vertex_buffer->Release();
	}

	//capture_free();

	if (data.using_shtex) 
	{
		if (data.texture)
		{
			data.texture->Release();
		}
		
	}
	else 
	{
		ERROR_EX_LOG("not using release !!!");
	}
	
	memset(&data, 0, sizeof(data));

	DEBUG_EX_LOG("----------------- d3d10 capture freed ----------------");
}

static bool create_d3d10_stage_surface(ID3D10Texture2D **tex)
{
	HRESULT hr;
	DEBUG_EX_LOG("");
	D3D10_TEXTURE2D_DESC desc = {};
	desc.Width = data.cx;
	desc.Height = data.cy;
	desc.Format = data.format;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D10_USAGE_STAGING;
	desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;

	hr = data.device->CreateTexture2D(&desc, nullptr, tex);
	if (FAILED(hr)) {
		WARNING_EX_LOG("create_d3d10_stage_surface: failed to create texture",
			hr);
		return false;
	}

	return true;
}

static bool create_d3d10_tex(uint32_t cx, uint32_t cy, ID3D10Texture2D **tex,
			     HANDLE *handle)
{
	HRESULT hr;
	DEBUG_EX_LOG("");
	D3D10_TEXTURE2D_DESC desc = {};
	desc.Width = cx;
	desc.Height = cy;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_TYPELESS;  /*apply_dxgi_format_typeless(
		data.format, global_hook_info->allow_srgb_alias);*/
	desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D10_USAGE_DEFAULT;
	desc.MiscFlags = D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX;

	hr = data.device->CreateTexture2D(&desc, nullptr, tex);
	if (FAILED(hr)) {
		WARNING_EX_LOG("create_d3d10_tex: failed to create texture", hr);
		return false;
	}

	if (!!handle) {
		IDXGIResource *dxgi_res;
		hr = (*tex)->QueryInterface(__uuidof(IDXGIResource),
					    (void **)&dxgi_res);
		if (FAILED(hr)) {
			WARNING_EX_LOG("create_d3d10_tex: failed to query "
				"IDXGIResource interface from texture",
				hr);
			return false;
		}

		hr = dxgi_res->GetSharedHandle(handle);
		dxgi_res->Release();
		if (FAILED(hr)) 
		{
			WARNING_EX_LOG("create_d3d10_tex: failed to get shared handle",
				hr);
			return false;
		}
	}

	return true;
}

static inline bool d3d10_init_format(IDXGISwapChain *swap, HWND &window)
{
	DXGI_SWAP_CHAIN_DESC desc;
	HRESULT hr;
	DEBUG_EX_LOG("");
	hr = swap->GetDesc(&desc);
	if (FAILED(hr))
	{
		ERROR_EX_LOG("d3d10_init_format: swap->GetDesc failed", hr);
		return false;
	}

	data.format = strip_dxgi_format_srgb(desc.BufferDesc.Format);
	data.multisampled = desc.SampleDesc.Count > 1;
	window = desc.OutputWindow;
	data.cx = desc.BufferDesc.Width;
	data.cy = desc.BufferDesc.Height;

	return true;
}


static bool d3d10_shtex_init(HWND window)
{
	bool success;

	data.using_shtex = true;
	DEBUG_EX_LOG("");
	success = create_d3d10_tex(data.cx, data.cy, &data.texture, &data.handle);

	if (!success) 
	{
		WARNING_EX_LOG("d3d10_shtex_init: failed to create texture");
		return false;
	}
	/*if (!capture_init_shtex(&data.shtex_info, window, data.cx, data.cy,
				data.format, false, (uintptr_t)data.handle)) {
		return false;
	}*/
	capture_count(0);
	capture_init_shtex(window, data.cx, data.cy, data.format,  data.handle);
	DEBUG_EX_LOG("capture_init_shtex d3d10 shared texture capture successful");
	return true;
}

static void d3d10_init(IDXGISwapChain *swap)
{
	HWND window;
	HRESULT hr;
	DEBUG_EX_LOG("");
	hr = swap->GetDevice(__uuidof(ID3D10Device), (void **)&data.device);
	if (FAILED(hr)) 
	{
		ERROR_EX_LOG("d3d10_init: failed to get device from swap", hr);
		return;
	}

	/* remove the unneeded extra reference */
	data.device->Release();

	if (!d3d10_init_format(swap, window)) 
	{
		return;
	}

	const bool success = d3d10_shtex_init(window);
	if (!success)
	{
		d3d10_free();
	}
}

static inline void d3d10_copy_texture(ID3D10Resource *dst, ID3D10Resource *src)
{
	if (data.multisampled) {
		data.device->ResolveSubresource(dst, 0, src, 0, data.format);
	} else {
		data.device->CopyResource(dst, src);
	}
}

static inline void d3d10_shtex_capture(ID3D10Resource *backbuffer)
{
	d3d10_copy_texture(data.texture, backbuffer);
}




void d3d10_capture(void *swap_ptr, void *backbuffer_ptr)
{
	IDXGIResource *dxgi_backbuffer = (IDXGIResource *)backbuffer_ptr;
	IDXGISwapChain *swap = (IDXGISwapChain *)swap_ptr;
	DEBUG_EX_LOG("");
	HRESULT hr;
	/*if (capture_should_stop()) {
		d3d10_free();
	}*/
	if (!data.init_capture) {
		d3d10_init(swap);
		data.init_capture = true;
	}
	//if (capture_ready()) 
	{
		ID3D10Resource *backbuffer;

		hr = dxgi_backbuffer->QueryInterface(__uuidof(ID3D10Resource),
						     (void **)&backbuffer);
		if (FAILED(hr)) 
		{
			WARNING_EX_LOG("d3d10_shtex_capture: failed to get "
				"backbuffer",
				hr);
			return;
		}

		if (data.using_shtex)
		{
			
			d3d10_shtex_capture(backbuffer);
			//capture_init_shtex(NULL, data.cx, data.cy, data.format, data.handle);
			if (data.write_tick_count == 0)
			{

				c_set_send_video_callback(&g_send_video_callback);
				capture_count(1);
			}
			data.write_tick_count = GetTickCount64();
		}
		else
		{
			ERROR_EX_LOG("not use shtex !!!");
		}
			//d3d10_shmem_capture(backbuffer);

		backbuffer->Release();
	}
}
