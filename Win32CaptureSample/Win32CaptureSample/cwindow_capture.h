/***********************************************************************************************
			created: 		2022-04-26

			author:			chensong

			purpose:		winrt    WGC技术 从GPU中获取界面窗口数据
************************************************************************************************/
#ifndef _C_WINRT_CAPTURE_H_
#define _C_WINRT_CAPTURE_H_



#include <Windows.h>

namespace chen
{
	class  __declspec(dllimport) cwindow_capture
	{
	public:

		cwindow_capture();
		~cwindow_capture();


		bool startup();


		//auto capturePicker = winrt::GraphicsCapturePicker();
//    auto savePicker = winrt::FileSavePicker();
	};
}

#endif // _C_WINRT_CAPTURE_H_