#include "cwindow_capture.h"
#include "pch.h"
#include "App.h"
#include "SampleWindow.h"
#include "cwindow_util.h"
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
App app;
namespace winrt
{
	using namespace Windows::Storage::Pickers;
	using namespace Windows::Graphics::Capture;
	using namespace Windows::UI::Composition;
}
FILE* out_winrt_capture_ptr = NULL;
#define WRITE_FILE()                                                          \
	if (!out_winrt_capture_ptr) {                                         \
		out_winrt_capture_ptr = fopen("winrt_capture.log", "wb+");    \
	}                                                                     \
	fprintf(out_winrt_capture_ptr, "[%s][%d]\n", __FUNCTION__, __LINE__); \
	fflush(out_winrt_capture_ptr);

#define WRITE_ERROR_FILE()                                                 \
	if (!out_winrt_capture_ptr) {                                      \
		out_winrt_capture_ptr = fopen("winrt_capture.log", "wb+"); \
	}                                                                  \
	fprintf(out_winrt_capture_ptr, "[%s][%d][ERROR]\n", __FUNCTION__,  \
		__LINE__);                                                 \
	fflush(out_winrt_capture_ptr);
namespace util
{
	using namespace desktop;
}
namespace chen
{
	cwindow_capture::cwindow_capture()
	{
	}
	cwindow_capture::~cwindow_capture()
	{
	}
	bool cwindow_capture::startup( )
	{
		HWND cur_wnd = FindMainWindow();
		if (!cur_wnd)
		{
			WRITE_FILE();
			return false;
		}
		auto capturePicker = winrt::GraphicsCapturePicker();
		  auto savePicker = winrt::FileSavePicker();
		  WRITE_FILE();
		  //    // Initialize Composition
   /* auto compositor = winrt::Compositor();
	WRITE_FILE();
    auto root = compositor.CreateContainerVisual();
	WRITE_FILE();
    root.RelativeSizeAdjustment({ 1.0f, 1.0f });
	WRITE_FILE();
    root.Size({ -220.0f, 0.0f });

	WRITE_FILE();
    root.Offset({ 220.0f, 0.0f, 0.0f });*/
	WRITE_FILE();
	app.init(   capturePicker, savePicker);
	WRITE_FILE();
	app.TryStartCaptureFromWindowHandle(cur_wnd);
		  //std::move(temp_app);
	WRITE_FILE();
		return true;
	}
}