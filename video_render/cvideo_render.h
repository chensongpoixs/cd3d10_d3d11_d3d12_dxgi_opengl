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

#ifndef _C_RTC_VIDEO_RENDER_
#define _C_RTC_VIDEO_RENDER_


#include "api/media_stream_interface.h"
#include "rtc_base/thread.h"

namespace libcross_platform_collection_render {

	class  cvideo_renderer : public rtc::VideoSinkInterface<webrtc::VideoFrame>
	{
	public:
		virtual ~cvideo_renderer();
		cvideo_renderer(rtc::scoped_refptr<webrtc::VideoTrackInterface> track_to_render);
		// Creates a platform-specific renderer if possible, or a null implementation
  // if failing.
		static cvideo_renderer* Create(const void* hwnd, size_t width,
			size_t height,
			webrtc::VideoTrackInterface* track_to_render);
		// Returns a renderer rendering to a platform specific window if possible,
		// NULL if none can be created.
		// Creates a platform-specific renderer if possible, returns NULL if a
		// platform renderer could not be created. This occurs, for instance, when
		// running without an X environment on Linux.
		static cvideo_renderer* CreatePlatformRenderer(const void* hwnd,
			size_t width, size_t height,
			webrtc::VideoTrackInterface* track_to_render);

	public:
		

		rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;
	  
	};

}

#endif // 