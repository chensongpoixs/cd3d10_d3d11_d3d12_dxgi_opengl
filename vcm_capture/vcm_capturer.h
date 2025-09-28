
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
				  date:  2025-09-28



******************************************************************************/
 
#ifndef _C_RTC_TEST_VCM_CAPTURER_H_
#define _C_RTC_TEST_VCM_CAPTURER_H_

#include <memory>
#include <vector>

#include "api/scoped_refptr.h"
#include "modules/video_capture/video_capture.h"
#include "libcross_platform_collection_render/desktop_capture/desktop_capture_source.h"
#include "libcross_platform_collection_render/desktop_capture/desktop_capture.h"
//#include "test/test_video_capturer.h"
#include "media/base/video_adapter.h"
#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "media/base/video_broadcaster.h"
namespace libcross_platform_collection_render {
class VcmCapturer :   public DesktopCaptureSource,
	public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  static VcmCapturer* Create(size_t width,
                             size_t height,
                             size_t target_fps,
                             size_t capture_device_index);
  virtual ~VcmCapturer();
  void StartCapture() override;
  void StopCapture() override;
  void OnFrame(const webrtc::VideoFrame& frame) override;

 private:
  VcmCapturer();
  bool Init(size_t width,
            size_t height,
            size_t target_fps,
            size_t capture_device_index);
  void Destroy();

  rtc::scoped_refptr<webrtc::VideoCaptureModule> vcm_;
  webrtc::VideoCaptureCapability capability_;

  webrtc::Mutex lock_;
//  std::unique_ptr<webrtc::FramePreprocessor> preprocessor_ RTC_GUARDED_BY(lock_);
 // rtc::VideoBroadcaster broadcaster_;
 // cricket::VideoAdapter video_adapter_;
};

}  // namespace  libcross_platform_collection_render

#endif  // _C_RTC_TEST_VCM_CAPTURER_H_
