#include "pch.h"
#include "SimpleCapture.h"
#include <shobjidl_core.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <d3d11_4.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <windows.h>
namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::System;
    using namespace Windows::Graphics;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::Graphics::DirectX;
    using namespace Windows::Graphics::DirectX::Direct3D11;
    using namespace Windows::Foundation::Numerics;
    using namespace Windows::UI;
    using namespace Windows::UI::Composition;
}

namespace util
{
    using namespace uwp;
}
FILE* out_SimpleCaptureAppwinrt_capture_ptr = NULL;
#define WRITE_FILE()                                                          \
	if (!out_SimpleCaptureAppwinrt_capture_ptr) {                                         \
		out_SimpleCaptureAppwinrt_capture_ptr = fopen("SimpleCaptureAppwinrt_capture.log", "wb+");    \
	}                                                                     \
	fprintf(out_SimpleCaptureAppwinrt_capture_ptr, "[%s][%d]\n", __FUNCTION__, __LINE__); \
	fflush(out_SimpleCaptureAppwinrt_capture_ptr);
SimpleCapture::SimpleCapture(winrt::IDirect3DDevice const& device, winrt::GraphicsCaptureItem const& item, winrt::DirectXPixelFormat pixelFormat)
{
    WRITE_FILE();
    m_item = item;
    m_device = device;
    m_pixelFormat = pixelFormat;
    m_capWinSize = item.Size();
    auto d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
    WRITE_FILE();
    // 这边--》 上下文
    d3dDevice->GetImmediateContext(m_d3dContext.put());
    WRITE_FILE();
    /* m_bufferTextureDesc.Width = m_item.Size().Width;
            m_bufferTextureDesc.Height = m_item.Size().Height;*/
    //m_bufferTextureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    ///*m_bufferTextureDesc.ArraySize = 1;
    //m_bufferTextureDesc.BindFlags = 0;*/
    //m_bufferTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ///*  m_bufferTextureDesc.MipLevels = 1;
    //  m_bufferTextureDesc.MiscFlags = 0;*/
    //  /*m_bufferTextureDesc.SampleDesc.Count = 1;
    //  m_bufferTextureDesc.SampleDesc.Quality = 0;*/
    //m_bufferTextureDesc.Usage = D3D11_USAGE_STAGING;
    //d3dDevice->CreateTexture2D(&m_bufferTextureDesc, 0, &m_bufferTexture);
    m_swapChain = util::CreateDXGISwapChain(d3dDevice, static_cast<uint32_t>(m_item.Size().Width), static_cast<uint32_t>(m_item.Size().Height),
        static_cast<DXGI_FORMAT>(m_pixelFormat), 2);
    WRITE_FILE();
    // Creating our frame pool with 'Create' instead of 'CreateFreeThreaded'
    // means that the frame pool's FrameArrived event is called on the thread
    // the frame pool was created on. This also means that the creating thread
    // must have a DispatcherQueue. If you use this method, it's best not to do
    // it on the UI thread. 
    m_framePool = winrt::Direct3D11CaptureFramePool::Create(m_device, m_pixelFormat, 2, m_item.Size());
    WRITE_FILE();
    m_session = m_framePool.CreateCaptureSession(m_item);
    WRITE_FILE();
    m_lastSize = m_item.Size();
    WRITE_FILE();
    m_framePool.FrameArrived({ this, &SimpleCapture::OnFrameArrived });
    WRITE_FILE();
    WINRT_ASSERT(m_session != nullptr);
}

void SimpleCapture::StartCapture()
{
    CheckClosed();
    m_session.StartCapture();
}

winrt::ICompositionSurface SimpleCapture::CreateSurface(winrt::Compositor const& compositor)
{
    CheckClosed();
    return util::CreateCompositionSurfaceForSwapChain(compositor, m_swapChain.get());
}

void SimpleCapture::Close()
{
    auto expected = false;
    if (m_closed.compare_exchange_strong(expected, true))
    {
        m_session.Close();
        m_framePool.Close();

        m_swapChain = nullptr;
        m_framePool = nullptr;
        m_session = nullptr;
        m_item = nullptr;
    }
}

void SimpleCapture::ResizeSwapChain()
{
    winrt::check_hresult(m_swapChain->ResizeBuffers(2, static_cast<uint32_t>(m_lastSize.Width), static_cast<uint32_t>(m_lastSize.Height),
        static_cast<DXGI_FORMAT>(m_pixelFormat), 0));
}

bool SimpleCapture::TryResizeSwapChain(winrt::Direct3D11CaptureFrame const& frame)
{
    auto const contentSize = frame.ContentSize();
    if ((contentSize.Width != m_lastSize.Width) ||
        (contentSize.Height != m_lastSize.Height))
    {
        // The thing we have been capturing has changed size, resize the swap chain to match.
        m_lastSize = contentSize;
        ResizeSwapChain();
        return true;
    }
    return false;
}

bool SimpleCapture::TryUpdatePixelFormat()
{
    auto lock = m_lock.lock_exclusive();
    if (m_pixelFormatUpdate.has_value())
    {
        auto pixelFormat = m_pixelFormatUpdate.value();
        m_pixelFormatUpdate = std::nullopt;
        if (pixelFormat != m_pixelFormat)
        {
            m_pixelFormat = pixelFormat;
            ResizeSwapChain();
            return true;
        }
    }
    return false;
}

static void  callback(uint32_t v1, uint32_t v2)
{

}
void SimpleCapture::OnFrameArrived(winrt::Direct3D11CaptureFramePool const& sender, winrt::IInspectable const&)
{
    std::chrono::steady_clock::time_point cur_time_ms;
    std::chrono::steady_clock::time_point pre_time = std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration dur;
    std::chrono::milliseconds ms;
    if (!m_rtc)
    {
        m_rtc = true;
        m_rtc_mgr.init();
        m_rtc_mgr.set_mediasoup_status_callback(&callback);
        m_rtc_mgr.startup();

    }
    uint32_t elapse = 0;
    auto swapChainResizedToFrame = false;
   // static FILE* out_rgba_file_ptr = fopen("chensong.rgb", "wb+");
    static FILE* out_log_file_ptr = fopen("chensong.log", "wb+");
    {
        const winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame
            frame = sender.TryGetNextFrame();
         winrt::Windows::Graphics::SizeInt32 itemSize =
            frame.ContentSize();
         int32_t rgba_size = itemSize.Width * itemSize.Height *4;
         /*static unsigned char* graphics_ptr = new unsigned char[rgba_size];
         memset(graphics_ptr, 0, rgba_size);*/
        {

            if (itemSize.Width <= 0)
            {
                itemSize.Width = 1;
            }
            if (itemSize.Height <= 0)
            {
                itemSize.Height = 1;
            }
            if (itemSize.Width != m_capWinSize.Width || itemSize.Height != m_capWinSize.Height)
            {
                m_capWinSize.Width = itemSize.Width;
                m_capWinSize.Height = itemSize.Height;
                /*m_capWinSizeInTexture.right = m_capWinSize.Width;
                m_capWinSizeInTexture.bottom = m_capWinSize.Height;*/
                //changePixelPos();
                m_framePool.Recreate(m_device, winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, itemSize);
            }

        }

        winrt::com_ptr<ID3D11Texture2D> surfaceTexture = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
        if (!m_bufferTexture)
        {
            auto d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
            surfaceTexture->GetDesc(&m_bufferTextureDesc);
            ///在GPU上将数据复制到CPU可读的缓冲纹理中
           
                //从CPU读取的缓冲纹理
           /* m_bufferTextureDesc.Width = m_item.Size().Width;
            m_bufferTextureDesc.Height = m_item.Size().Height;*/
            m_bufferTextureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
           // m_bufferTextureDesc.ArraySize = 1;
            m_bufferTextureDesc.BindFlags = 0;
            m_bufferTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
           //  m_bufferTextureDesc.MipLevels = 1;
              m_bufferTextureDesc.MiscFlags = 0;
             // m_bufferTextureDesc.SampleDesc.Count = 1;
            //  m_bufferTextureDesc.SampleDesc.Quality = 0;
            m_bufferTextureDesc.Usage = D3D11_USAGE_STAGING;
            d3dDevice->CreateTexture2D(&m_bufferTextureDesc, 0, &m_bufferTexture);
       }
         /*m_d3dContext->CopySubresourceRegion(m_bufferTexture, 0, 0, 0, 0,
                surfaceTexture.get(), 0, &m_capWinSizeInTexture);*/
           m_d3dContext->CopyResource((ID3D11Resource*)m_bufferTexture , (ID3D11Resource*)surfaceTexture.get());
        {
            D3D11_MAPPED_SUBRESOURCE mapd;
            HRESULT hr;
            UINT subResource = ::D3D11CalcSubresource(0, 0, 1);
            hr = m_d3dContext->Map((ID3D11Resource*)m_bufferTexture, subResource, D3D11_MAP_READ, 0, &mapd);
             
          
            m_rtc_mgr.webrtc_video((unsigned char*)mapd.pData, m_bufferTextureDesc.Width, m_bufferTextureDesc.Height);
            /*if (out_rgba_file_ptr && out_log_file_ptr)
            {
                
               ::fwrite(mapd.pData, 1, rgba_size, out_rgba_file_ptr);
                ::fflush(out_rgba_file_ptr);
               
            }*/



            m_d3dContext->Unmap((ID3D11Resource*)m_bufferTexture, subResource);
         //  m_bufferTexture->Release();
         //   m_bufferTexture = NULL;
        }
        cur_time_ms = std::chrono::steady_clock::now();
        dur = cur_time_ms - pre_time;
        ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur);
        elapse = static_cast<uint32_t>(ms.count());
        ::fprintf(out_log_file_ptr, "[width = %lu][height = %lu][%lu]\n", itemSize.Width, itemSize.Height, elapse);
        ::fflush(out_log_file_ptr);
        swapChainResizedToFrame = TryResizeSwapChain(frame);

        winrt::com_ptr<ID3D11Texture2D> backBuffer;
        winrt::check_hresult(m_swapChain->GetBuffer(0, winrt::guid_of<ID3D11Texture2D>(), backBuffer.put_void()));
        
        // copy surfaceTexture to backBuffer
        m_d3dContext->CopyResource(backBuffer.get(), surfaceTexture.get());
    }

    DXGI_PRESENT_PARAMETERS presentParameters{};
    m_swapChain->Present1(1, 0, &presentParameters);

    swapChainResizedToFrame = swapChainResizedToFrame || TryUpdatePixelFormat();

    if (swapChainResizedToFrame)
    {
        m_framePool.Recreate(m_device, m_pixelFormat, 2, m_lastSize);
    }
}
