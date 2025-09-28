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
#include "libcross_platform_collection_render/video_render/cvideo_render_factory.h"
#include "pc/media_stream_track_proxy.h" // webrtc proxy 
#include "pc/video_track.h"
namespace libcross_platform_collection_render
{
	namespace {



		rtc::Thread* MaybeStartThread(rtc::Thread* old_thread,
			const std::string& thread_name,
			bool with_socket_server,
			std::unique_ptr<rtc::Thread>& thread_holder) {
			if (old_thread) {
				return old_thread;
			}
			if (with_socket_server) {
				thread_holder = rtc::Thread::CreateWithSocketServer();
			}
			else {
				thread_holder = rtc::Thread::Create();
			}
			thread_holder->SetName(thread_name, nullptr);
			thread_holder->Start();
			return thread_holder.get();
		}
	}


	cvideo_render_factory::cvideo_render_factory()
		: worker_thread_(MaybeStartThread(nullptr, "video_render_factory_worker_thread", false, owned_worker_thread_))
		, signaling_thread_(MaybeStartThread(nullptr, "video_render_factory_signaling_thread", false, owned_signaling_thread_))
	{
	}

	cvideo_render_factory::~cvideo_render_factory()
	{
		//owned_worker_thread_->Quit();
		owned_worker_thread_->Stop();
		owned_signaling_thread_->Stop();
		//owned_signaling_thread_->Quit();
		//owned_worker_thread_.reset();
		//owned_signaling_thread_.reset();
	}


	rtc::scoped_refptr<webrtc::VideoTrackInterface> cvideo_render_factory::create_video_render(const std::string & id, webrtc::VideoTrackSourceInterface * source)
	{
		//return rtc::scoped_refptr<webrtc::VideoTrackInterface>();
	

		rtc::scoped_refptr<webrtc::VideoTrackInterface> track(
			webrtc::VideoTrack::Create(id, source, worker_thread()));
		return webrtc::VideoTrackProxy::Create(signaling_thread(), worker_thread(), track);
	}

}