
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
				  date:  2025-10-02



******************************************************************************/
#ifndef _C_RTC_AUDIO_CAPTURE_H_
#define _C_RTC_AUDIO_CAPTURE_H_

#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "libcross_platform_collection_render/desktop_capture/desktop_capture_source.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_frame.h"
#include "api/video/i420_buffer.h"

#include "libmedia_codec/x264_encoder.h"
#include <thread>
#include <atomic>
#include <rtc_base/logging.h>
#include <modules/audio_device/include/audio_device.h>
#include <rtc_base/task_utils/to_queued_task.h>
#include <api/audio/audio_frame.h>
#include <api/task_queue/default_task_queue_factory.h>
#include "libmedia_codec/audio_frame.h"
#include "libmedia_codec/audio_codec/opus_encoder.h"
namespace libcross_platform_collection_render {

	class AudioCapture : public webrtc::AudioTransport
	{
	public:
		AudioCapture(rtc::Thread* work);
		~AudioCapture();
	public:
		void Start(const std::string & device_name)  ; 
		void Stop()  ;
		
		void StartPlayout(const std::string & device_name);
		void StopPlayout();
		
		void Destroy()  ;


		void SetAudioEncoder(libmedia_codec::OpusEncoder2* encoder);


		void AppAudioData(rtc::Buffer&& data);
	public:
		int32_t  GetAudioDeviceCount();
		int32_t  GetAudioDeviceInfo(int32_t index, std::string & audio_device, std::string& device_guid);
	public:


		virtual int32_t RecordedDataIsAvailable(const void* audioSamples,
			const size_t nSamples,
			const size_t nBytesPerSample,
			const size_t nChannels,
			const uint32_t samplesPerSec,
			const uint32_t totalDelayMS,
			const int32_t clockDrift,
			const uint32_t currentMicLevel,
			const bool keyPressed,
			uint32_t& newMicLevel) override;  // NOLINT

// Implementation has to setup safe values for all specified out parameters.
		virtual int32_t NeedMorePlayData(const size_t nSamples,
			const size_t nBytesPerSample,
			const size_t nChannels,
			const uint32_t samplesPerSec,
			void* audioSamples,
			size_t& nSamplesOut,  // NOLINT
			int64_t* elapsed_time_ms,
			int64_t* ntp_time_ms)  override;  // NOLINT

// Method to pull mixed render audio data from all active VoE channels.
// The data will not be passed as reference for audio processing internally.
		virtual void PullRenderData(int bits_per_sample,
			int sample_rate,
			size_t number_of_channels,
			size_t number_of_frames,
			void* audio_data,
			int64_t* elapsed_time_ms,
			int64_t* ntp_time_ms)override;
	private:
		rtc::Thread * work_thread_;
		rtc::scoped_refptr<webrtc::AudioDeviceModule> audio_device_;
		std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory_;
		bool has_start_ = false;

		libmedia_codec::OpusEncoder2 *       opus_encoder2_;
		uint32_t   timestamp_ =0 ;
		std::string  device_name_;


		rtc::Buffer                           audio_buffer_;
		int32_t								  audio_buffer_size_;
	};
}

#endif // _C_RTC_AUDIO_CAPTURE_H_