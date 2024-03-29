#include <d3d11.h>
#include <dxgi.h>

#include "dxgi-helpers.hpp"
//#include "graphics-hook.h"
#include "ccapture_hook.h"
#include <iostream>
#include "C:\Work\cabroad_server\Server\Robot\ccloud_rendering_c.h"
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
		
	};
	int write_tick_count = 0;
};

static struct d3d11_data data = {};

void d3d11_free(void)
{
	capture_count(0);
	capture_init_shtex(NULL, 0, 0, 0, NULL);
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

	if (data.using_shtex) 
	{
		if (data.texture)
		{
			data.texture->Release();
		}
	} 
	else 
	{
		ERROR_EX_LOG("not using shtex release  !!! ");
	}

	memset(&data, 0, sizeof(data));

	DEBUG_EX_LOG("----------------- d3d11 capture freed ----------------");
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
	desc.Format = data.format; 
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;


	////////////////////////
	//D3D11_TEXTURE2D_DESC bufferTextureDesc = { 0 };
	//bufferTextureDesc.Width =  cx;
	//bufferTextureDesc.Height =  cy;
	//bufferTextureDesc.MipLevels = 1;
	//bufferTextureDesc.ArraySize = 1;

	//bufferTextureDesc.SampleDesc.Count = 1;
	//bufferTextureDesc.Format = data.format;// DXGI_FORMAT_B8G8R8A8_UNORM;
	//bufferTextureDesc.BindFlags = 0;
	//bufferTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	//bufferTextureDesc.MiscFlags = 0;
	//bufferTextureDesc.Usage = D3D11_USAGE_STAGING;


	hr = data.device->CreateTexture2D(&desc, nullptr, tex);
	if (FAILED(hr)) {
		WARNING_EX_LOG("create_d3d11_tex: failed to create texture", hr);
		return false;
	}

	if (!!handle) 
	{
		DEBUG_EX_LOG("set shared GPU !!!");
		IDXGIResource *dxgi_res;
		hr = (*tex)->QueryInterface(__uuidof(IDXGIResource),
					    (void **)&dxgi_res);
		if (FAILED(hr)) {
			WARNING_EX_LOG("create_d3d11_tex: failed to query "
				"IDXGIResource interface from texture",
				hr);
			return false;
		}
		// �õ�����GPU���
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
	if (FAILED(hr))
	{
		WARNING_EX_LOG("d3d11_init_format: swap->GetDesc failed", hr);
		return false;
	}

	data.format = strip_dxgi_format_srgb(desc.BufferDesc.Format);
	DEBUG_EX_LOG("data.format = %u][desc.BufferDesc.Width = %u]", data.format, desc.BufferDesc.Width);
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
	capture_count(0);
	capture_init_shtex(window, data.cx, data.cy, data.format,  data.handle);
	DEBUG_EX_LOG("capture_init_shtex d3d11 shared texture capture successful");
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
	DEBUG_EX_LOG("[data.multisampled = %u][data.format = %u][width = %u][height = %u]", data.multisampled, data.format, data.cx, data.cy);
	if (data.multisampled) {
		data.context->ResolveSubresource(dst, 0, src, 0, data.format);
	} else {
		data.context->CopyResource(dst, src);
	}


	//D3D11_TEXTURE2D_DESC bufferTextureDesc = { 0 };
	//bufferTextureDesc.Width = data.cx;
	//bufferTextureDesc.Height = data.cy;
	//bufferTextureDesc.MipLevels = 1;
	//bufferTextureDesc.ArraySize = 1;
	// 
	//bufferTextureDesc.SampleDesc.Count = 1;
	//bufferTextureDesc.Format = data.format;// DXGI_FORMAT_B8G8R8A8_UNORM;
	//bufferTextureDesc.BindFlags = 0;
	//bufferTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	//bufferTextureDesc.MiscFlags = 0;
	//bufferTextureDesc.Usage = D3D11_USAGE_STAGING;
	//ID3D11Texture2D* tex;
	//HRESULT hr   = data.device->CreateTexture2D(&bufferTextureDesc, nullptr, &tex);;// CreateTexture2D(data.device, &bufferTextureDesc, NULL,
	//	//&data.d3d11_tex_video);
	//if (SUCCEEDED(hr))
	//{
	//	data.context->CopyResource(tex, src);
	//	D3D11_MAPPED_SUBRESOURCE map;
	//	UINT subResource = 0;
	//	hr = data.context->Map(tex, 0, D3D11_MAP_READ, 0, &map);
	//	static FILE* out_file_yuv_ptr = fopen("read_yuv.rgb", "wb+");
	//	if (SUCCEEDED(hr))
	//	{
	//		DEBUG_EX_LOG("map ok  !!!");
	//		if (out_file_yuv_ptr)
	//		{
	//			fwrite(map.pData, 1, data.cx * data.cy * 4, out_file_yuv_ptr);
	//			fflush(out_file_yuv_ptr);
	//		}
	//	}
	//	else
	//	{
	//		ERROR_EX_LOG("map failed !!!");
	//	}
	//	data.context->Unmap(tex, 0);
	//	tex->Release();
	//}


	if (data.write_tick_count == 0)
	{
		capture_count(1);
		c_set_send_video_callback(&g_send_video_callback);
	}
	data.write_tick_count = GetTickCount64();
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
	if (!data.init_capture)
	{
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
			//capture_init_shtex(NULL, data.cx, data.cy, data.format, data.handle);
			
		}
		else
		{
			ERROR_EX_LOG("not use shtex !!!");
		}

		backbuffer->Release();
	}
}
