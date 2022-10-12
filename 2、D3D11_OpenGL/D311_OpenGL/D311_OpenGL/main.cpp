#include <iostream>
#include <dxgi.h>
#include <d3d11.h>

static const GUID GUID_IDXGIFactory1 =
{ 0x770aae78, 0xf26f, 0x4dba, {0xa8, 0x29, 0x25, 0x3c, 0x83, 0xd1, 0xb3, 0x87} };
static const GUID GUID_IDXGIResource =
{ 0x035f3ab4, 0x482e, 0x4e50, {0xb4, 0x1f, 0x8a, 0x7f, 0x8b, 0xd8, 0x96, 0x0b} };



int main(int argc, char* argv[])
{
	IDXGIFactory1* factory;
	HRESULT hr;
	CreateDXGIFactory1(&GUID_IDXGIFactory1, (void **)&factory);

	return 0;
}