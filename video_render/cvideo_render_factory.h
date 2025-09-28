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

#ifndef _C_RTC_VIDEO_RENDER_FACTORY_
#define _C_RTC_VIDEO_RENDER_FACTORY_


#include "api/media_stream_interface.h"
#include "rtc_base/thread.h"
 
namespace libcross_platform_collection_render {



	class  cvideo_render_factory
	{
	public:
		explicit cvideo_render_factory();

		virtual ~cvideo_render_factory();



	public:
		rtc::scoped_refptr<webrtc::VideoTrackInterface>   create_video_render(const std::string & id, webrtc::VideoTrackSourceInterface * source);

	public:
		rtc::Thread* signaling_thread() { return signaling_thread_; }
		const rtc::Thread* signaling_thread() const { return signaling_thread_; }
		rtc::Thread* worker_thread() { return worker_thread_; }
		const rtc::Thread* worker_thread() const { return worker_thread_; } 

	private:


		std::unique_ptr<rtc::Thread> owned_worker_thread_;

		std::unique_ptr<rtc::Thread> owned_signaling_thread_;



		rtc::Thread *const   worker_thread_;
		rtc::Thread* const   signaling_thread_;
	};


}

#endif // _C_RTC_VIDEO_RENDER_