#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <d3d11.h>
//#include <DxErr.h>
//#include <D3DX11.h>
#include <d3d11.h>
#include <iostream>
#include <stdio.h>
#include <dxgi.h>
#include <d3d11.h>
#include <windows.h>
#include <d3d11.h>
//#include <DxErr.h>
//#include <D3DX11.h>
#include <d3d11.h>
//#include <iostream>
#include <stdio.h>
#include <dxgi.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <string>
#include <ctime>
//////////////////////////////////////////////////////////
IDXGIFactory1* g_factory = NULL;
IDXGIAdapter* g_adapter = NULL;
uint32_t g_gpu_index = 4;
static std::string log_name = std::string("gpu_") + std::to_string(::time(NULL)) + ".log";
static FILE * out_file_ptr = fopen(log_name.c_str(), "wb+");
//////////////////////////////////////////////////////////
HINSTANCE g_hInstance = NULL;
HWND g_hWnd = NULL;
//CHAR[] g_name = L"FirstD3D11Demo";
D3D_DRIVER_TYPE g_driverType = D3D_DRIVER_TYPE_NULL;                //驱动类型
D3D_FEATURE_LEVEL g_featureLevel = D3D_FEATURE_LEVEL_11_0;            //特征等级    
ID3D11Device *g_pd3dDevice = NULL;                                    //设备
ID3D11DeviceContext *g_pImmediateContext = NULL;                    //设备上下文
IDXGISwapChain *g_pSwapChain = NULL;                                //交换链
ID3D11RenderTargetView *g_pRenderTargetView = NULL;                    //要创建的视图


HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{

	
	if (FAILED(InitWindow(hInstance, nShowCmd)))
		return 0;
	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else//渲染
		{
			Render();
		}
	}
	CleanupDevice();
	return static_cast<int>(msg.wParam);
}

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSEX wcex;
	wcex.cbClsExtra = 0;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.cbWndExtra = 0;
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wcex.hIconSm = wcex.hIcon;
	wcex.hInstance = hInstance;
	wcex.lpfnWndProc = WndProc;
	wcex.lpszClassName = "FirstD3D11Demo";;
	wcex.lpszMenuName = NULL;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	g_hInstance = hInstance;
	RECT rc{ 0,0,640,480 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindowEx(WS_EX_APPWINDOW, "FirstD3D11Demo", "FirstD3D11Demo", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, g_hInstance, NULL);
	if (!g_hWnd)
	{
		return E_FAIL;
	}

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wPararm, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wPararm, lParam);
	}
	return 0;
}

//创建设备及交换链
HRESULT InitDevice()
{
	HRESULT hResult = S_OK;//返回结果

	RECT rc;
	GetClientRect(g_hWnd, &rc);//获取窗口客户区大小
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	//驱动类型数组
	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	//特征级别数组
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	//交换链
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(DXGI_SWAP_CHAIN_DESC));//填充
	sd.BufferCount = 1;                              //我们只创建一个后缓冲（双缓冲）因此为1
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM; // DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;                      //1重采样
	sd.SampleDesc.Quality = 0;                      //采样等级
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;      //常用参数
	sd.Windowed = TRUE;                              //是否全屏
	//sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	{
		//static const GUID GUID_IDXGIFactory1 =
		//{ 0x770aae78, 0xf26f, 0x4dba, {0xa8, 0x29, 0x25, 0x3c, 0x83, 0xd1, 0xb3, 0x87} };
		//CreateDXGIFactory1(&GUID_IDXGIFactory1, (void**)&g_factory);
		hResult = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&g_factory);
		if (FAILED(hResult))
		{
			return NULL;
		}

		hResult = g_factory->EnumAdapters(g_gpu_index, &g_adapter);
		if (FAILED(hResult))
		{

			return NULL;
		}
		char desc[128] = { 0 };
		DXGI_ADAPTER_DESC adapterDesc;
		g_adapter->GetDesc(&adapterDesc);
		wcstombs(desc, adapterDesc.Description, sizeof(desc));
		if (strstr(desc, "NVIDIA") != NULL)
		{
			/*hr = D3D11CreateDevice(g_adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
				&enc->d3d11_device, nullptr, &enc->d3d11_context);*/
				//enc->adapter->Release();
				//enc->adapter = nullptr;
				//if (SUCCEEDED(hr))
			if (out_file_ptr)
			{

				fprintf(out_file_ptr, "use gpu info g_gpu_index = %u ok \n", g_gpu_index);
				fflush(out_file_ptr);

			}
		}
		else
		{
			if (out_file_ptr)
			{
				fprintf(out_file_ptr, "[g_gpu_index = %u]  not NVIDIA  [desc = %s] failed !!! \n", g_gpu_index, desc);
				fflush(out_file_ptr);
			}
			
			 
		}
		//g_adapter->EnumOutputs();
	}
	//for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; ++driverTypeIndex)
	{
		g_driverType = D3D_DRIVER_TYPE_UNKNOWN; // driverTypes[0];//D3D_DRIVER_TYPE_UNKNOWN;//
		hResult = D3D11CreateDeviceAndSwapChain(
			g_adapter,                                //默认图形适配器
			g_driverType,                        //驱动类型
			NULL,                                //实现软件渲染设备的动态库句柄，如果使用的驱动设备类型是软件设备则不能为NULL
			createDeviceFlags,                    //创建标志，0用于游戏发布，一般D3D11_CREATE_DEVICE_DEBUG允许我们创建可供调试的设备，在开发中比较有用
			featureLevels,                        //特征等级
			numFeatureLevels,                    //特征等级数量
			D3D11_SDK_VERSION,                    //sdk版本号
			&sd,
			&g_pSwapChain,
			&g_pd3dDevice,
			&g_featureLevel,
			&g_pImmediateContext
		);
		if (FAILED(hResult))
		{
			return NULL;
		}
		//	break;
	}
	if (FAILED(hResult))
	{
		return hResult;
	}

	//创建渲染目标视图
	ID3D11Texture2D *pBackBuffer = NULL;
	//获取后缓冲区地址
	hResult = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hResult))
		return hResult;

	//创建目标视图
	hResult = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	//D3D11_MAPPED_SUBRESOURCE map;
	//g_pImmediateContext->Map(pBackBuffer, 0, D3D11_MAP_READ, 0, &map);
	//释放后缓冲
	pBackBuffer->Release();
	if (FAILED(hResult))
		return hResult;

	//绑定到渲染管线
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);

	//设置viewport
	D3D11_VIEWPORT vp;
	vp.Height = (FLOAT)height;
	vp.Width = (FLOAT)width;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);
	if (g_adapter)
	{
		g_adapter->Release();
		g_adapter = NULL;
	 }
	return S_OK;
}

void Render()
{
	static bool load = false;
	if (load)
	{
		// 显示窗口
		//ShowWindow(g_hWnd, SW_SHOW);
		
		load = false;
		float ClearColor[4] = { 0.5f, 0.1f, 0.2f, 1.0f }; //red,green,blue,alpha
		g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
	}
	else
	{
		// 隐藏窗口
		//ShowWindow(g_hWnd, SW_HIDE);
		load = true;
		float ClearColor[4] = { 0.8f, 0.4f, 0.8f, 1.0f }; //red,green,blue,alpha
		g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
	}

	if (out_file_ptr)
	{
		fprintf(out_file_ptr, "[%s][%s][%d]\n", __FILE__, __FUNCTION__, __LINE__);
		fflush(out_file_ptr);
	}
	//ShowWindow(g_hWnd, SW_HIDE);

	g_pSwapChain->Present(0, 0);

	//创建渲染目标视图
	//ID3D11Texture2D *pBackBuffer = NULL;
	//获取后缓冲区地址
	/*HRESULT hResult = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hResult))
		return;*/
	//g_pSwapChain->GetDevice();
	/*D3D11_MAPPED_SUBRESOURCE map;
	g_pImmediateContext->Map(pBackBuffer, 0, D3D11_MAP_READ, 0, &map);*/
	//g_pImmediateContext->Unmap(pBackBuffer, 0);
	//D3D11_SUBRESOURCE_DATA sdesc;//texture 初始化参数
	//sdesc.pSysMem = buffer;//设置用于初始化的内容入口地址，内部会自动拷贝
	//sdesc.SysMemPitch = width;//一行占用的像素，如果硬盘中存的内容的slice是2304，这里填2304，在创建是会自动截掉多余width的部分。对于这个slice，该值在存入文件中时如果是通过D3Dtexture map函数来存放，可以通过其中的参数来获知。
	//sdesc.SysMemSlicePitch = width * height + width * height / 2;//总共占用的像素。
	//ID3D11Texture2D* texTemp = nullptr;

	////HRESULT hr = device->CreateTexture2D(&desc, NULL, &texTemp);
	//HRESULT hr = device->CreateTexture2D(&desc, &sdesc, &texTemp);//创建texture
	
}

void CleanupDevice()
{
	if (g_pImmediateContext)
		g_pImmediateContext->ClearState();
	if (g_pSwapChain)
		g_pSwapChain->Release();
	if (g_pRenderTargetView)
		g_pRenderTargetView->Release();
	if (g_pImmediateContext)
		g_pImmediateContext->Release();
	if (g_pd3dDevice)
		g_pd3dDevice->Release();
}