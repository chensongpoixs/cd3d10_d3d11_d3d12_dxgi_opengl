
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
#ifndef _C_RTC_DESKTOP_CAPTURE_H_
#define _C_RTC_DESKTOP_CAPTURE_H_

#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "libcross_platform_collection_render/desktop_capture/desktop_capture_source.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_frame.h"
#include "api/video/i420_buffer.h"

#include "libmedia_codec/x264_encoder.h"
#include <thread>
#include <atomic>

namespace libcross_platform_collection_render {

class DesktopCapture : public DesktopCaptureSource,
                       public webrtc::DesktopCapturer::Callback,
                       public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  static DesktopCapture* Create(size_t target_fps, size_t capture_screen_index);

  ~DesktopCapture() override;

  std::string GetWindowTitle() const { return window_title_; }

  void StartCapture()override ;
  void StopCapture() override;
 
 private:
  DesktopCapture();

  void Destory();

  void OnFrame(const webrtc::VideoFrame& frame) override {}

  bool Init(size_t target_fps, size_t capture_screen_index);

  void OnCaptureResult(webrtc::DesktopCapturer::Result result,
                       std::unique_ptr<webrtc::DesktopFrame> frame) override;

  std::unique_ptr<webrtc::DesktopCapturer> dc_;

  size_t fps_;
  std::string window_title_;

  std::unique_ptr<std::thread> capture_thread_;
  std::atomic_bool start_flag_;

  rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer_;
  
};
}  // namespace crtc

#endif  // crtc