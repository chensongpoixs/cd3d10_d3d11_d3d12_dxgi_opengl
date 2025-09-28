/******************************************************************************
 *  Copyright (c) 2025 The CRTC project authors . All Rights Reserved.
 *
 *  Please visit https://chensongpoixs.github.io for detail
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 ******************************************************************************/
 /*****************************************************************************
				   Author: chensong
				   date:  2025-09-18



 ******************************************************************************/
#include "libcross_platform_collection_render/track_capture/ctrack_capture.h"
#include  "libcross_platform_collection_render/desktop_capture/desktop_capture_source.h"
#include "pc/video_track_source.h"
#include "libcross_platform_collection_render/desktop_capture/desktop_capture.h"
#include "libcross_platform_collection_render/vcm_capture/vcm_capturer.h"
#include "modules/video_capture/video_capture_factory.h"
namespace libcross_platform_collection_render
{

	rtc::scoped_refptr<CapturerTrackSource> CapturerTrackSource::Create(bool vcm_capture)
	{
		if (vcm_capture)
		{
			const size_t kWidth = 1280;
			const size_t kHeight = 720;
			const size_t kFps = 30;
			std::unique_ptr<VcmCapturer> capturer;
			std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
				webrtc::VideoCaptureFactory::CreateDeviceInfo());
			if (!info) {
				return nullptr;
			}
			int num_devices = info->NumberOfDevices();
			for (int i = 0; i < num_devices; ++i) {
				capturer = absl::WrapUnique(
					VcmCapturer::Create(kWidth, kHeight, kFps, i));
				if (capturer) {
					return new rtc::RefCountedObject<CapturerTrackSource>(
						std::move(capturer));
				}
			}
		}
		/*const size_t kWidth = 640;
		const size_t kHeight = 480;
		const size_t kFps = 30;
		std::unique_ptr<webrtc::test::VcmCapturer> capturer;
		std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
			webrtc::VideoCaptureFactory::CreateDeviceInfo());
		if (!info) {
		  return nullptr;
		}
		int num_devices = info->NumberOfDevices();
		for (int i = 0; i < num_devices; ++i) {
		  capturer = absl::WrapUnique(
			  webrtc::test::VcmCapturer::Create(kWidth, kHeight, kFps, i));
		  if (capturer) {
			return new
				rtc::RefCountedObject<CapturerTrackSource>(std::move(capturer));
		  }
		}*/
		std::unique_ptr<libcross_platform_collection_render::DesktopCapture> capturer(
			libcross_platform_collection_render::DesktopCapture::Create(90, 0));
		if (capturer) {
		//	capturer->StartCapture();
			return new rtc::RefCountedObject<CapturerTrackSource>(
				std::move(capturer));
		}
		return nullptr;
	}

	void CapturerTrackSource::StartCapture()
	{
		if (capturer_)
		{
			capturer_->StartCapture();
		}
	}

	void CapturerTrackSource::Stop()
	{
		if (capturer_)
		{
			capturer_->StopCapture();
		}
	}
	void CapturerTrackSource::set_catprue_callback(libmedia_codec::X264Encoder * x264_encoder, rtc::Thread * f)
	{
		if (capturer_)
		{
			capturer_->set_catprue_callback(x264_encoder, f);
		}
	}
}