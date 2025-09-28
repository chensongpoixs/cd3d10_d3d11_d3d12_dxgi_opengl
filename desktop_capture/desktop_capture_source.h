
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
#ifndef _C_RTC_DESKTOP_CAPTURE_SOURCE_H_
#define _C_RTC_DESKTOP_CAPTURE_SOURCE_H_

#include "api/video/video_frame.h"
#include "api/video/video_source_interface.h"
#include "media/base/video_adapter.h"
#include "media/base/video_broadcaster.h"
#include "api/video/i420_buffer.h"
#include "libmedia_codec/i420_buffer.h"
#include "libmedia_codec/x264_encoder.h"
#include "rtc_base/thread.h"
namespace libcross_platform_collection_render {

class DesktopCaptureSource
    : public rtc::VideoSourceInterface<webrtc::VideoFrame> {
 public:
  DesktopCaptureSource() {}
  ~DesktopCaptureSource() override {}

  void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                       const rtc::VideoSinkWants& wants) override;

  void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) override;
  void set_catprue_callback(libmedia_codec::X264Encoder * x264_encoder, rtc::Thread* t);
  virtual void StartCapture() {}
  virtual void  StopCapture() {}
  
 protected:
  // Notify sinkes
  void OnFrame(const webrtc::VideoFrame& frame);

 private:
  void UpdateVideoAdapter();

  rtc::VideoBroadcaster broadcaster_;
  cricket::VideoAdapter video_adapter_;

  libmedia_codec::X264Encoder * x264_encoder_ = nullptr;

  rtc::scoped_refptr<libmedia_codec::I420Buffer> libmedia_codec_i420_buffer_;
   rtc::Thread* singnal_thread_;
};

}  // namespace crtc

#endif  // crtc