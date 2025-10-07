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
				   date:  2025-09-17



 ******************************************************************************/
#include "libcross_platform_collection_render/video_render/cvideo_render.h"
#include "api/video/i420_buffer.h" 
 
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "libyuv/convert_argb.h"
#include "libcross_platform_collection_render/video_render/d3d_renderer.h"
namespace libcross_platform_collection_render
{

	cvideo_renderer* cvideo_renderer::CreatePlatformRenderer(const void* hwnd,
		size_t width,
		size_t height,
		webrtc::VideoTrackInterface* track_to_render) {
		return D3dRenderer::Create(hwnd, width, height, track_to_render);
	}
	cvideo_renderer::~cvideo_renderer()
	{
		if (rendered_track_)
		{
			rendered_track_->RemoveSink(this);
		}


		RTC_LOG_F(LS_INFO) << " -->>";
	}
	cvideo_renderer::cvideo_renderer(rtc::scoped_refptr<webrtc::VideoTrackInterface> track_to_render)
		:rendered_track_(track_to_render)  
	{
		if (rendered_track_)
		{
			rendered_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
		}
		
	}
	cvideo_renderer* cvideo_renderer::Create(const void* hwnd,
		size_t width,
		size_t height,
		webrtc::VideoTrackInterface* track_to_render) {
		cvideo_renderer* renderer = CreatePlatformRenderer(hwnd, width, height, track_to_render);
		if (renderer != NULL) {
			// TODO(mflodman) Add a warning log.
			return renderer;
		}

		return nullptr;
	}
}