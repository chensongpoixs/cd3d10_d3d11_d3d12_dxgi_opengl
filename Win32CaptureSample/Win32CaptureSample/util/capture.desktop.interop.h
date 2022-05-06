#pragma once
#include <winrt/Windows.Graphics.Capture.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.capture.h>
static FILE* out_interop_Appwinrt_capture_ptr = NULL;
#define WRITE_FILE()                                                          \
	if (!out_interop_Appwinrt_capture_ptr) {                                         \
		out_interop_Appwinrt_capture_ptr = fopen("interop_winrt_capture.log", "wb+");    \
	}                                                                     \
	fprintf(out_interop_Appwinrt_capture_ptr, "[%s][%d]\n", __FUNCTION__, __LINE__); \
	fflush(out_interop_Appwinrt_capture_ptr);

#define WRITE_ERROR_FILE()                                                 \
	if (!out_interop_Appwinrt_capture_ptr) {                                      \
		out_interop_Appwinrt_capture_ptr = fopen("interop_winrt_capture.log", "wb+"); \
	}                                                                  \
	fprintf(out_interop_Appwinrt_capture_ptr, "[%s][%d][ERROR]\n", __FUNCTION__,  \
		__LINE__);                                                 \
	fflush(out_interop_Appwinrt_capture_ptr);
namespace util
{
    inline auto CreateCaptureItemForWindow(HWND hwnd)
    {
        WRITE_FILE();
        auto interop_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
        WRITE_FILE();
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
        //winrt::check_hresult(
        WRITE_FILE();
        const HRESULT hr = interop_factory->CreateForWindow(hwnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(item));//);
        if (FAILED(hr))
        {
            return item;
        }
        WRITE_FILE();
        return item;
    }

    inline auto CreateCaptureItemForMonitor(HMONITOR hmon)
    {
        auto interop_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
        winrt::check_hresult(interop_factory->CreateForMonitor(hmon, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(item)));
        return item;
    }
}
