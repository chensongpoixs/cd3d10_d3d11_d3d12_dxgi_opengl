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

#ifndef _C_RTC_TRACK_CAPTURE__H_
#define _C_RTC_TRACK_CAPTURE__H_


#include "api/media_stream_interface.h"
#include "rtc_base/thread.h"
#include "pc/video_track_source.h"
#include "libcross_platform_collection_render/desktop_capture/desktop_capture.h"
#include "libmedia_codec/x264_encoder.h"
namespace libcross_platform_collection_render {

	class CapturerTrackSource : public webrtc::VideoTrackSource {
	public:
		static rtc::scoped_refptr<CapturerTrackSource> Create( bool vcm_capture = true);
		bool is_screencast() const override { return m_screencast; }
		absl::optional<bool> needs_denoising() const override { return m_screencast; }


		void  StartCapture();
		void  Stop();
		

		void set_catprue_callback(libmedia_codec::X264Encoder* x264_encoder, rtc::Thread * f);
	protected:
		explicit CapturerTrackSource(
			std::unique_ptr<libcross_platform_collection_render::DesktopCaptureSource> capturer)
			: VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

	private:
		rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
			return capturer_.get();
		}
		// std::unique_ptr<webrtc::test::VcmCapturer> capturer_;
		std::unique_ptr<libcross_platform_collection_render::DesktopCaptureSource> capturer_;
		bool m_screencast = true;
	};
}


#endif // 