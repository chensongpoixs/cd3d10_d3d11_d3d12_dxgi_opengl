/***********************************************************************************************
					created: 		2022-07-21

					author:			chensong

					purpose:		RenderOffScreen demo one 
************************************************************************************************/

//#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <commctrl.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <string>
#pragma comment( lib, "OpenGL32.lib" )
#pragma comment( lib, "glu32.lib" )
//using namespace std;

BOOL SaveBmp(HBITMAP hBitmap, std::string FileName)
{
	HDC hDC;
	//当前分辨率下每象素所占字节数
	int iBits;
	//位图中每象素所占字节数
	WORD wBitCount;
	//定义调色板大小， 位图中像素字节大小 ，位图文件大小 ， 写入文件字节数
	DWORD dwPaletteSize = 0, dwBmBitsSize = 0, dwDIBSize = 0, dwWritten = 0;
	//位图属性结构
	BITMAP Bitmap;
	//位图文件头结构
	BITMAPFILEHEADER bmfHdr;
	//位图信息头结构
	BITMAPINFOHEADER bi;
	//指向位图信息头结构
	LPBITMAPINFOHEADER lpbi;
	//定义文件，分配内存句柄，调色板句柄
	HANDLE fh, hDib, hPal, hOldPal = NULL;

	//计算位图文件每个像素所占字节数
	hDC = CreateDC("DISPLAY", NULL, NULL, NULL);
	iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
	DeleteDC(hDC);
	if (iBits <= 1)
	{
		wBitCount = 1;
	}
	else if (iBits <= 4)
	{
		wBitCount = 4;
	}
	else if (iBits <= 8)
	{
		wBitCount = 8;
	}
	else
	{
		wBitCount = 24;
	}

	GetObject(hBitmap, sizeof(Bitmap), (LPSTR)&Bitmap);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = Bitmap.bmWidth;
	bi.biHeight = Bitmap.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = wBitCount;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrImportant = 0;
	bi.biClrUsed = 0;

	dwBmBitsSize = ((Bitmap.bmWidth * wBitCount + 31) / 32) * 4 * Bitmap.bmHeight;

	//为位图内容分配内存
	hDib = GlobalAlloc(GHND, dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
	*lpbi = bi;

	// 处理调色板
	hPal = GetStockObject(DEFAULT_PALETTE);
	if (hPal)
	{
		hDC = ::GetDC(NULL);
		hOldPal = ::SelectPalette(hDC, (HPALETTE)hPal, FALSE);
		RealizePalette(hDC);
	}

	// 获取该调色板下新的像素值
	GetDIBits(hDC, hBitmap, 0, (UINT)Bitmap.bmHeight, (LPSTR)lpbi + sizeof(BITMAPINFOHEADER)
		+ dwPaletteSize, (BITMAPINFO *)lpbi, DIB_RGB_COLORS);

	//恢复调色板
	if (hOldPal)
	{
		::SelectPalette(hDC, (HPALETTE)hOldPal, TRUE);
		RealizePalette(hDC);
		::ReleaseDC(NULL, hDC);
	}

	//创建位图文件
	fh = CreateFile(FileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);


	if (fh == INVALID_HANDLE_VALUE) return FALSE;

	// 设置位图文件头
	bmfHdr.bfType = 0x4D42; // "BM"
	dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;
	bmfHdr.bfSize = dwDIBSize;
	bmfHdr.bfReserved1 = 0;
	bmfHdr.bfReserved2 = 0;
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;
	// 写入位图文件头
	WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
	// 写入位图文件其余内容
	WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL);
	//清除
	GlobalUnlock(hDib);
	GlobalFree(hDib);
	CloseHandle(fh);

	return TRUE;
}

void mGLRender()
{
	//1. 表示清除颜色设为黄色
	glClearColor(0.9f, 0.9f, 0.3f, 1.0f); 
	//2. 表示实际完成了把整个窗口清除为黑色的任务，glClear（）的唯一参数表示需要被清除的缓冲区
	/*
	参数选择：
	GL_COLOR_BUFFER_BIT:    当前可写的颜色缓冲
      GL_DEPTH_BUFFER_BIT:    深度缓冲
      GL_ACCUM_BUFFER_BIT:   累积缓冲
　　GL_STENCIL_BUFFER_BIT: 模板缓冲
	*/
	glClear(GL_COLOR_BUFFER_BIT);
	
	//  3. 指定哪一个矩阵堆栈是下一个矩阵操作的目标,可选值:
	//　GL_MODELVIEW,对模型视图矩阵堆栈应用随后的矩阵操作。可以在执行此命令后，输出自己的物体图形了。
	//  GL_PROJECTION, 对投影矩阵堆栈应用随后的矩阵操作。可以在执行此命令后，为我们的场景增加透视。
	//	GL_TEXTURE, 对纹理矩阵堆栈应用随后的矩阵操作。可以在执行此命令后，为我们的图形增加纹理贴图。
	glMatrixMode(GL_PROJECTION);
	// 4. 设置视角的函数
	gluPerspective(30.0, 1.0, 1.0, 10.0);
	glMatrixMode(GL_MODELVIEW);
	// 5. 设置视角的方向函数 
	gluLookAt(0, 0, -5, 0, 0, 0, 0, 1, 0);

	// 6. 
	/*
		在glBegin()和glEnd()之间可调用的函数
		函数 函数意义
		glVertex*() 设置顶点坐标
		glColor*() 设置当前颜色
		glIndex*() 设置当前颜色表
		glNormal*() 设置法向坐标
		glEvalCoord*() 产生坐标
		glCallList(),glCallLists() 执行显示列表
		glTexCoord*() 设置纹理坐标
		glEdgeFlag*() 控制边界绘制
		glMaterial*() 设置材质
	*/
	glBegin(GL_TRIANGLES);
	// 6.1 设置当前颜色
	glColor3d(1, 0, 0);
	// 6.2 设置顶点坐标
	glVertex3d(0, 1, 0);
	////////
	glColor3d(0, 1, 0);
	glVertex3d(-1, -1, 0);
	//
	glColor3d(0, 0, 1);
	glVertex3d(1, -1, 0);
	//
	glEnd();
	glFlush(); // remember to flush GL output!
}

int main(int argc, char* argv[])
{
	const int WIDTH = 500;
	const int HEIGHT = 500;

	// Create a memory DC compatible with the screen
	HDC hdc = CreateCompatibleDC(0);
	if (hdc == 0)
	{
		std::cout << "Could not create memory device context" << std::endl;;
	}

	// Create a bitmap compatible with the DC
	// must use CreateDIBSection(), and this means all pixel ops must be synchronised
	// using calls to GdiFlush() (see CreateDIBSection() docs)
	BITMAPINFO bmi = {
			{ sizeof(BITMAPINFOHEADER), WIDTH, HEIGHT, 1, 32, BI_RGB, 0, 0, 0, 0, 0 },
			{ 0 }
	};
	DWORD *pbits; // pointer to bitmap bits
	HBITMAP hbm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void **)&pbits,
		0, 0);
	if (hbm == 0)
	{
		std::cout << "Could not create bitmap" << std::endl;
	}

	//HDC hdcScreen = GetDC(0);
	//HBITMAP hbm = CreateCompatibleBitmap(hdcScreen,WIDTH,HEIGHT);

	// Select the bitmap into the DC
	HGDIOBJ r = SelectObject(hdc, hbm);
	if (r == 0)
	{
		std::cout << "Could not select bitmap into DC" << std::endl;;
	}

	// Choose the pixel format
	PIXELFORMATDESCRIPTOR pfd = {
			sizeof(PIXELFORMATDESCRIPTOR), // struct size
					1, // Version number
					PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL, // use OpenGL drawing to BM
					PFD_TYPE_RGBA, // RGBA pixel values
					32, // color bits
					0, 0, 0, // RGB bits shift sizes...
					0, 0, 0, // Don't care about them
					0, 0, // No alpha buffer info
					0, 0, 0, 0, 0, // No accumulation buffer
					32, // depth buffer bits
					0, // No stencil buffer
					0, // No auxiliary buffers
					PFD_MAIN_PLANE, // Layer type
					0, // Reserved (must be 0)
					0, // No layer mask
					0, // No visible mask
					0 // No damage mask
	};
	int pfid = ChoosePixelFormat(hdc, &pfd);
	if (pfid == 0)
	{
		std::cout << "Pixel format selection failed" << std::endl;;
	}

	// Set the pixel format
	// - must be done *after* the bitmap is selected into DC
	BOOL b = SetPixelFormat(hdc, pfid, &pfd);
	if (!b)
	{
		std::cout << "Pixel format set failed" << std::endl;;
	}

	// Create the OpenGL resource context (RC) and make it current to the thread
	HGLRC hglrc = wglCreateContext(hdc);
	if (hglrc == 0)
	{
		std::cout << "OpenGL resource context creation failed" << std::endl;;
	}
	wglMakeCurrent(hdc, hglrc);

	// Draw using GL - remember to sync with GdiFlush()
	GdiFlush();
	mGLRender();

	SaveBmp(hbm, "output.bmp");
	/*
	Examining the bitmap bits (pbits) at this point with a debugger will reveal
	that the colored triangle has been drawn.
	*/

	// Clean up
	wglDeleteContext(hglrc); // Delete RC
	SelectObject(hdc, r); // Remove bitmap from DC
	DeleteObject(hbm); // Delete bitmap
	DeleteDC(hdc); // Delete DC

	return 0;
}


/*

好了，编译成功，运行，确实是可以啊！看看步骤是什么样的：

CreateCompatibleDC

创建dc

CreateDIBSection

创建图像

SelectObject

图像选入DC

SetPixelFormat

设置像元格式

wglCreateContext

创建RC

wglMakeCurrent

选择RC

mGLRender

开始渲染

SaveBmp

保存图像（这段是我从网上随便摘下来的）

...

清理
*/