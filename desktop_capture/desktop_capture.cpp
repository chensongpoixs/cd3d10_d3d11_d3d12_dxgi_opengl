
/*
*  Copyright (c) 2025 The CRTC project authors . All Rights Reserved.
*
*  Please visit https://chensongpoixs.github.io for detail
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/
/*****************************************************************************
				  Author: chensong
				  date:  2025-09-17



******************************************************************************/
#include "libcross_platform_collection_render/desktop_capture/desktop_capture.h"

#include "modules/desktop_capture/desktop_capture_options.h"
#include "rtc_base/logging.h"
#include "libyuv.h"
#include "rtc_base/logging.h"

namespace libcross_platform_collection_render {

DesktopCapture::DesktopCapture() : dc_(nullptr), start_flag_(false) {}

DesktopCapture::~DesktopCapture() {
  Destory();
}

void DesktopCapture::Destory() {
  StopCapture();

  if (!dc_)
    return;

  dc_.reset(nullptr);
}

DesktopCapture* DesktopCapture::Create(size_t target_fps,
                                       size_t capture_screen_index) {
  std::unique_ptr<DesktopCapture> dc(new DesktopCapture());
  if (!dc->Init(target_fps, capture_screen_index)) {
    RTC_LOG(LS_WARNING) << "Failed to create DesktopCapture(fps = "
                        << target_fps << ")";
    return nullptr;
  }
  return dc.release();
}

bool DesktopCapture::Init(size_t target_fps, size_t capture_screen_index) 
{
	// 窗口
	/*dc_ = webrtc::DesktopCapturer::CreateWindowCapturer(
		webrtc::DesktopCaptureOptions::CreateDefault());*/
	//桌面allow_directx_capturer_
	webrtc::DesktopCaptureOptions options;
	options.set_allow_directx_capturer(true);
	dc_ = webrtc::DesktopCapturer::CreateScreenCapturer(options);

  if (!dc_)
    return false;

  webrtc::DesktopCapturer::SourceList sources;
  dc_->GetSourceList(&sources);
  if (capture_screen_index > sources.size()) {
    RTC_LOG(LS_WARNING) << "The total sources of screen is " << sources.size()
                        << ", but require source of index at "
                        << capture_screen_index;
    return false;
  }

  RTC_CHECK(dc_->SelectSource(sources[capture_screen_index].id));
  window_title_ = sources[capture_screen_index].title;
  fps_ = target_fps;

  RTC_LOG(LS_INFO) << "Init DesktopCapture finish window_title = " << window_title_ << " , fps = " << fps_ <<"";
  // Start new thread to capture
  return true;
}


void DesktopCapture::OnCaptureResult(
    webrtc::DesktopCapturer::Result result,
    std::unique_ptr<webrtc::DesktopFrame> frame) {
  //RTC_LOG(LS_INFO) << "new Frame";

  static auto timestamp =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
  static size_t cnt = 0;

  cnt++;
  auto timestamp_curr = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
  if (timestamp_curr - timestamp > 1000) {
    RTC_LOG(LS_INFO) << "FPS: " << cnt;
    cnt = 0;
    timestamp = timestamp_curr;
  }

  // Convert DesktopFrame to VideoFrame
  if (result != webrtc::DesktopCapturer::Result::SUCCESS) {
    RTC_LOG(LS_ERROR) << "Capture frame faiiled, result: " << result;
  }
  int width = frame->size().width();
  int height = frame->size().height();
  // int half_width = (width + 1) / 2;

  if (!i420_buffer_.get() ||
      i420_buffer_->width() * i420_buffer_->height() < width * height) {
    i420_buffer_ = webrtc::I420Buffer::Create(width, height);
	//libmedia_codec_i420_buffer_ = libmedia_codec::I420Buffer::Create(width, height);
  }
  //i420_buffer_->set_texture();
  libyuv::ConvertToI420(frame->data(), 0, i420_buffer_->MutableDataY(),
                        i420_buffer_->StrideY(), i420_buffer_->MutableDataU(),
                        i420_buffer_->StrideU(), i420_buffer_->MutableDataV(),
                        i420_buffer_->StrideV(), 0, 0, width, height, width,
                        height, libyuv::kRotate0, libyuv::FOURCC_ARGB);
#if 0
    libyuv::ConvertToI420(frame->data(), 0, libmedia_codec_i420_buffer_->MutableDataY(),
	  libmedia_codec_i420_buffer_->StrideY(), libmedia_codec_i420_buffer_->MutableDataU(),
	  libmedia_codec_i420_buffer_->StrideU(), libmedia_codec_i420_buffer_->MutableDataV(),
	  libmedia_codec_i420_buffer_->StrideV(), 0, 0, width, height, width,
	  height, libyuv::kRotate0, libyuv::FOURCC_ARGB); 

  if (x264_encoder_)
  {
	  libmedia_codec::VideoFrame captureFrame =
		  libmedia_codec::VideoFrame::Builder()
		  .set_video_frame_buffer(libmedia_codec_i420_buffer_)
		  .set_timestamp_rtp(rtc::TimeMillis())  // set_ntp_time_ms
		  .set_ntp_time_ms(rtc::TimeMillis())
		  .set_timestamp_ms(rtc::TimeMillis())
		  .set_rotation(libmedia_codec::kVideoRotation_0)
		  .build();
	  std::shared_ptr< libmedia_codec::VideoFrame>   out = std::make_shared< libmedia_codec::VideoFrame>(captureFrame);
	  x264_encoder_->OnNewMediaFrame(out);
  }
#endif // 
  // seting 马流的信息

  webrtc::VideoFrame captureFrame =
	  webrtc::VideoFrame::Builder()
	  .set_video_frame_buffer(i420_buffer_)
          .set_timestamp_rtp(rtc::TimeMillis())  // set_ntp_time_ms
          .set_ntp_time_ms(rtc::TimeMillis())
	  .set_timestamp_ms(rtc::TimeMillis())
	  .set_rotation(webrtc::kVideoRotation_0)
	  .build();


 // captureFrame.set_ntp_time_ms(0);
  DesktopCaptureSource::OnFrame(captureFrame);
  // rtc media info 
 /* DesktopCaptureSource::OnFrame(
      webrtc::VideoFrame(i420_buffer_, 0, 0, webrtc::kVideoRotation_0));*/
}

void DesktopCapture::StartCapture() {
  if (start_flag_) {
    RTC_LOG(WARNING) << "Capture already been running...";
    return;
  }

  start_flag_ = true;

  // Start new thread to capture
  capture_thread_.reset(new std::thread([this]() {
    dc_->Start(this);

    while (start_flag_) {
      dc_->CaptureFrame();
     // std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps_));
    }
  }));
}

void DesktopCapture::StopCapture() {
  start_flag_ = false;

  if (capture_thread_ && capture_thread_->joinable()) {
    capture_thread_->join();
  }
}



}  // namespace webrtc_demo