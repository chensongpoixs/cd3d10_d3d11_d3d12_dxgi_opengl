#include <d3d11.h>
#include <dxgi.h>

#include "dxgi-helpers.hpp"
//#include "graphics-hook.h"
#include "ccapture_hook.h"
#include <iostream>
struct d3d11_data {
	ID3D11Device *device;         /* do not release */
	ID3D11DeviceContext *context; /* do not release */
	uint32_t cx;
	uint32_t cy;
	DXGI_FORMAT format;
	bool using_shtex;
	bool multisampled;

	ID3D11Texture2D *scale_tex;
	ID3D11ShaderResourceView *scale_resource;

	ID3D11VertexShader *vertex_shader;
	ID3D11InputLayout *vertex_layout;
	ID3D11PixelShader *pixel_shader;

	ID3D11SamplerState *sampler_state;
	ID3D11BlendState *blend_state;
	ID3D11DepthStencilState *zstencil_state;
	ID3D11RasterizerState *raster_state;

	ID3D11Buffer *vertex_buffer;

	bool init_capture;

	union {
		/* shared texture */
		struct {
			struct shtex_data *shtex_info;
			ID3D11Texture2D *texture;
			HANDLE handle;
		};
		/* shared memory */
		struct {
			ID3D11Texture2D *copy_surfaces[NUM_BUFFERS];
			bool texture_ready[NUM_BUFFERS];
			bool texture_mapped[NUM_BUFFERS];
			uint32_t pitch;
			struct shmem_data *shmem_info;
			int cur_tex;
			int copy_wait;
		};
	};
};

static struct d3d11_data data = {};

void d3d11_free(void)
{
	if (data.scale_tex)
		data.scale_tex->Release();
	if (data.scale_resource)
		data.scale_resource->Release();
	if (data.vertex_shader)
		data.vertex_shader->Release();
	if (data.vertex_layout)
		data.vertex_layout->Release();
	if (data.pixel_shader)
		data.pixel_shader->Release();
	if (data.sampler_state)
		data.sampler_state->Release();
	if (data.blend_state)
		data.blend_state->Release();
	if (data.zstencil_state)
		data.zstencil_state->Release();
	if (data.raster_state)
		data.raster_state->Release();
	if (data.vertex_buffer)
		data.vertex_buffer->Release();

	//capture_free();

	if (data.using_shtex) {
		if (data.texture)
			data.texture->Release();
	} else {
		for (size_t i = 0; i < NUM_BUFFERS; i++) {
			if (data.copy_surfaces[i]) {
				if (data.texture_mapped[i])
					data.context->Unmap(
						data.copy_surfaces[i], 0);
				data.copy_surfaces[i]->Release();
			}
		}
	}

	memset(&data, 0, sizeof(data));

	DEBUG_EX_LOG("----------------- d3d11 capture freed ----------------");
}

static bool create_d3d11_stage_surface(ID3D11Texture2D **tex)
{
	HRESULT hr;
	DEBUG_EX_LOG("");
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = data.cx;
	desc.Height = data.cy;
	desc.Format = data.format;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	hr = data.device->CreateTexture2D(&desc, nullptr, tex);
	if (FAILED(hr)) {
		WARNING_EX_LOG("create_d3d11_stage_surface: failed to create texture",
			hr);
		return false;
	}

	return true;
}

static bool create_d3d11_tex(uint32_t cx, uint32_t cy, ID3D11Texture2D **tex,
			     HANDLE *handle)
{
	HRESULT hr;
	DEBUG_EX_LOG("");
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = cx;
	desc.Height = cy;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_TYPELESS;  /*apply_dxgi_format_typeless(
		data.format, global_hook_info->allow_srgb_alias);*/
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	hr = data.device->CreateTexture2D(&desc, nullptr, tex);
	if (FAILED(hr)) {
		WARNING_EX_LOG("create_d3d11_tex: failed to create texture", hr);
		return false;
	}

	if (!!handle) {
		IDXGIResource *dxgi_res;
		hr = (*tex)->QueryInterface(__uuidof(IDXGIResource),
					    (void **)&dxgi_res);
		if (FAILED(hr)) {
			WARNING_EX_LOG("create_d3d11_tex: failed to query "
				"IDXGIResource interface from texture",
				hr);
			return false;
		}
		// µÃµ½¹²ÏíGPU¾ä±ú
		hr = dxgi_res->GetSharedHandle(handle);
		dxgi_res->Release();
		if (FAILED(hr)) {
			WARNING_EX_LOG("create_d3d11_tex: failed to get shared handle",
				hr);
			return false;
		}
	}

	return true;
}

static inline bool d3d11_init_format(IDXGISwapChain *swap, HWND &window)
{
	DXGI_SWAP_CHAIN_DESC desc;
	HRESULT hr;
	DEBUG_EX_LOG("");
	hr = swap->GetDesc(&desc);
	if (FAILED(hr)) {
		WARNING_EX_LOG("d3d11_init_format: swap->GetDesc failed", hr);
		return false;
	}

	data.format = strip_dxgi_format_srgb(desc.BufferDesc.Format);
	data.multisampled = desc.SampleDesc.Count > 1;
	window = desc.OutputWindow;
	data.cx = desc.BufferDesc.Width;
	data.cy = desc.BufferDesc.Height;

	return true;
}

 
 
static bool d3d11_shtex_init(HWND window)
{
	bool success;
	DEBUG_EX_LOG("");
	data.using_shtex = true;

	success = create_d3d11_tex(data.cx, data.cy, &data.texture, &data.handle);

	if (!success) 
	{
		WARNING_EX_LOG("d3d11_shtex_init: failed to create texture");
		return false;
	}
	/*if (!capture_init_shtex(&data.shtex_info, window, data.cx, data.cy,
				data.format, false, (uintptr_t)data.handle)) {
		return false;
	}*/

	DEBUG_EX_LOG("d3d11 shared texture capture successful");
	return true;
}

static void d3d11_init(IDXGISwapChain *swap)
{
	HWND window;
	HRESULT hr;
	DEBUG_EX_LOG("");
	hr = swap->GetDevice(__uuidof(ID3D11Device), (void **)&data.device);
	if (FAILED(hr)) {
		ERROR_EX_LOG("d3d11_init: failed to get device from swap", hr);
		return;
	}

	data.device->Release();

	data.device->GetImmediateContext(&data.context);
	data.context->Release();

	if (!d3d11_init_format(swap, window)) {
		return;
	}

	const bool success =    d3d11_shtex_init(window);
	if (!success)
	{
		d3d11_free();
	}
}

static inline void d3d11_copy_texture(ID3D11Resource *dst, ID3D11Resource *src)
{
	DEBUG_EX_LOG("");
	if (data.multisampled) {
		data.context->ResolveSubresource(dst, 0, src, 0, data.format);
	} else {
		data.context->CopyResource(dst, src);
	}
}

static inline void d3d11_shtex_capture(ID3D11Resource *backbuffer)
{
	d3d11_copy_texture(data.texture, backbuffer);
}

 



void d3d11_capture(void *swap_ptr, void *backbuffer_ptr)
{
	IDXGIResource *dxgi_backbuffer = (IDXGIResource *)backbuffer_ptr;
	IDXGISwapChain *swap = (IDXGISwapChain *)swap_ptr;
	DEBUG_EX_LOG("");
	HRESULT hr;
	/*if (capture_should_stop()) {
		d3d11_free();
	}*/
	if (!data.init_capture) {
		d3d11_init(swap);
		data.init_capture = true;

	}
	/*if (capture_ready())*/ 
	{
		ID3D11Resource *backbuffer;

		hr = dxgi_backbuffer->QueryInterface(__uuidof(ID3D11Resource),
						     (void **)&backbuffer);
		if (FAILED(hr)) 
		{
			WARNING_EX_LOG("d3d11_shtex_capture: failed to get "
				"backbuffer",
				hr);
			return;
		}

		if (data.using_shtex)
		{
			d3d11_shtex_capture(backbuffer);
		}
		else
		{
			ERROR_EX_LOG("not use shtex !!!");
		}

		backbuffer->Release();
	}
}
