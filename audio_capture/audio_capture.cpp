
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

#include "libcross_platform_collection_render/audio_capture/audio_capture.h"
#include <rtc_base/task_utils/to_queued_task.h>
namespace libcross_platform_collection_render
{

	namespace {
		static const int32_t    kAudioBufferSize = 1024 * 1024 * 8;
	}

	AudioCapture::AudioCapture(rtc::Thread * work)
		: work_thread_(work),
		audio_device_(nullptr)
		, task_queue_factory_(webrtc::CreateDefaultTaskQueueFactory())
		, opus_encoder2_(nullptr)
		, audio_buffer_(kAudioBufferSize)
		, audio_buffer_size_(0)
	{
		// 在api_thread创建并初始化音频设备
		work_thread_->PostTask(webrtc::ToQueuedTask([=]() {
			audio_device_ = webrtc::AudioDeviceModule::Create(
				webrtc::AudioDeviceModule::kPlatformDefaultAudio,
				task_queue_factory_.get());
			audio_device_->Init();
		}));
	}

	AudioCapture::~AudioCapture()
	{
	}

	void AudioCapture::Start(const std::string& device_name)
	{
		device_name_ = device_name;
		RTC_LOG(LS_INFO) << "AudioCapture Start call";
		work_thread_->PostTask(webrtc::ToQueuedTask([=]() {
			RTC_LOG(LS_INFO) << "AudioCapture Start PostTask call";

			 
			do {
				 

				 
				// 2. 设置回调
				audio_device_->RegisterAudioCallback(this);

				// 3. 检查系统是否存在麦克风设备
				int total = audio_device_->RecordingDevices();
				if (total <= 0) {
					RTC_LOG(LS_WARNING) << "no audio device";
					 
					break;
				}
				WCHAR buffer[128] = { 0 };
				auto bytetowchar = [&](const std::string &src) {
					if (MultiByteToWideChar(CP_UTF8, 0, src.c_str(), src.size(), buffer,
						128) == 0) {
						RTC_LOG(LS_ERROR)
							<< "MultiByteToWideChar(CP_UTF8) failed with error code "
							<< GetLastError();
					}
				};
				// 4. 检查关联的mic_id是否能够在系统设备中找到
				int device_index = -1;
				for (int i = 0; i < total; ++i) {
					char name[128];
					char guid[128];
					audio_device_->RecordingDeviceName(i, name, guid);
					if (0 == strcmp(guid, device_name_.c_str())) {
						device_index = i;
						break;
					}
				}

				if (device_index <= -1) {
					RTC_LOG(LS_WARNING) << "audio device not found, device_name: " << device_name_;
					 
					break;
				}

				// 5. 设置启用的麦克风设备
				if (audio_device_->SetRecordingDevice(device_index)) {
					RTC_LOG(LS_WARNING) << "SetRecordingDevice failed, device_name: " << device_name_;
					 
					break;
				}

				// 6. 设置为立体声采集
				audio_device_->SetStereoRecording(true);

				// 7. 初始化麦克风
				if (audio_device_->InitRecording() || !audio_device_->RecordingIsInitialized()) {
					RTC_LOG(LS_WARNING) << "InitRecording failed, device_name: " << device_name_;
					 
					break;
				}

				// 8. 启动麦克风采集
				if (audio_device_->StartRecording()) {
					RTC_LOG(LS_WARNING) << "StartRecording failed, device_name: " << device_name_;
					 
					break;
				}

				has_start_ = true;

			} while (0);

			

		}));
	}

	void AudioCapture::Stop()
	{
		RTC_LOG(LS_INFO) << "AudioCapture Stop call";
		work_thread_->PostTask(webrtc::ToQueuedTask([=]() {
			RTC_LOG(LS_INFO) << "AudioCapture Stop PostTask";
			if (!has_start_) {
				return;
			}

			has_start_ = false;
			 
			// 停止录音
			if (audio_device_->RecordingIsInitialized()) {
				audio_device_->StopRecording();
			}
		}));
	}

	void AudioCapture::StartPlayout(const std::string & device_name)
	{
		device_name_ = device_name;
		RTC_LOG(LS_INFO) << "Audio StartPlayout Start call";
		work_thread_->PostTask(webrtc::ToQueuedTask([=]() {
			RTC_LOG(LS_INFO) << "Audio StartPlayout Start PostTask call";


			do {

				audio_device_->SetPlayoutDevice(webrtc::AudioDeviceModule::kDefaultDevice);
				audio_device_->RegisterAudioCallback(this);
				// 7. 初始化麦克风
				if (audio_device_->InitPlayout() || !audio_device_->PlayoutIsInitialized()) {
					RTC_LOG(LS_WARNING) << "Init Playout failed, device_name: " << device_name_;

					break;
				}

				// 8. 启动麦克风采集
				if (audio_device_->StartPlayout()) {
					RTC_LOG(LS_WARNING) << "Start Playout failed, device_name: " << device_name_;

					break;
				}

				has_start_ = true;

			} while (0);



		}));
	}
	void AudioCapture::StopPlayout()
	{
		RTC_LOG(LS_INFO) << "Audio Play out Stop call";
		work_thread_->PostTask(webrtc::ToQueuedTask([=]() {
			RTC_LOG(LS_INFO) << "AudioPlay out Stop PostTask";
			if (!has_start_) {
				return;
			}

			has_start_ = false;

			// 停止录音
			if (audio_device_->PlayoutIsInitialized()) {
				audio_device_->StopPlayout();
			}
		}));
	}

	void AudioCapture::Destroy()
	{
		RTC_LOG(LS_INFO) << "AudioCapture Destroy call";
		work_thread_->PostTask(webrtc::ToQueuedTask([=]() {
			RTC_LOG(LS_INFO) << "AudioCapture Destroy PostTask";
			delete this;
		}));
	}

	void AudioCapture::SetAudioEncoder(libmedia_codec::OpusEncoder2 * encoder)
	{
		opus_encoder2_ = encoder;
	}

	void AudioCapture::AppAudioData(rtc::Buffer&& data)
	{
		work_thread_->PostTask(webrtc::ToQueuedTask([this, data_ = std::move(data)]() {
			if (audio_buffer_.size() + data_.size() > kAudioBufferSize)
			{
				RTC_LOG(LS_WARNING) << " conume audio buffer size tai big  : " << audio_buffer_.size();
				return;
			}

			audio_buffer_.AppendData(data_);
		}));
	}

	int32_t AudioCapture::GetAudioDeviceCount()
	{
		return work_thread_->Invoke<int32_t>(RTC_FROM_HERE, [=]() {
			return  audio_device_->RecordingDevices();
		});
		return 0;
	}

	int32_t AudioCapture::GetAudioDeviceInfo(int32_t index, std::string & audio_device, std::string& device_guid)
	{
		return work_thread_->Invoke<int32_t>(RTC_FROM_HERE, [&]() {
			//return  audio_device_->RecordingDevices();
			char name[128] = {0};
			char guid[128] = {0};
			int32_t ret = audio_device_->RecordingDeviceName(
				index, name, guid);
			audio_device = name;
			device_guid = guid;
			return ret;
		});
		return 0;
	}

	int32_t AudioCapture::RecordedDataIsAvailable(
		const void * audioSamples, 
		const size_t nSamples, // 每个声道数包含的样本数
		const size_t nBytesPerSample, // 每个样本的字节数
		const size_t nChannels, 
		const uint32_t samplesPerSec, 
		const uint32_t totalDelayMS, // 参考信号和远端回声信号之间的延迟
		const int32_t clockDrift, 
		const uint32_t currentMicLevel, 
		const bool keyPressed, 
		uint32_t & newMicLevel)
	{

		int len = static_cast<int>(nSamples * nBytesPerSample);
		//auto frame = std::make_shared<MediaFrame>(webrtc::AudioFrame::kMaxDataSizeBytes);
		//frame->fmt.media_type = MainMediaType::kMainTypeAudio;
		//frame->fmt.sub_fmt.audio_fmt.type = SubMediaType::kSubTypePcm;
		//frame->fmt.sub_fmt.audio_fmt.nbytes_per_sample = nBytesPerSample;
		//frame->fmt.sub_fmt.audio_fmt.samples_per_channel = nSamples;
		//frame->fmt.sub_fmt.audio_fmt.channels = nChannels;
		//frame->fmt.sub_fmt.audio_fmt.samples_per_sec = samplesPerSec;
		//frame->fmt.sub_fmt.audio_fmt.total_delay_ms = totalDelayMS;
		//frame->fmt.sub_fmt.audio_fmt.key_pressed = keyPressed;
		//frame->data_len[0] = len;
		//memcpy(frame->data[0], audioSamples, len);
		// 计算时间戳，根据采样频率进行单调递增
		timestamp_ += nSamples;
		//frame->ts = timestamp_;


		std::shared_ptr< libmedia_codec::AudioFrame> audio_frame = std::make_shared<libmedia_codec::AudioFrame>();
		
		audio_frame->num_channels_ = nChannels;
		audio_frame->sample_rate_hz_ = samplesPerSec;
		//audio_frame->packet_infos_
		//audio_frame->packet_infos_
		audio_frame->samples_per_channel_ = nSamples;
		memcpy(audio_frame->mutable_data(), audioSamples, len);
		audio_frame->timestamp_ = rtc::SystemTimeMillis();
		if (opus_encoder2_)
		{
			opus_encoder2_->OnNewMediaFrame(audio_frame);
		}
		return 0;
	}

	int32_t AudioCapture::NeedMorePlayData(const size_t nSamples, 
		const size_t nBytesPerSample, const size_t nChannels, 
		const uint32_t samplesPerSec, void * audioSamples, 
		size_t & nSamplesOut, int64_t * elapsed_time_ms, 
		int64_t * ntp_time_ms)
	{
		//一帧的音频数据的大小
		const size_t bytes_to_read = nSamples * nBytesPerSample * nChannels;
		if (audio_buffer_.size() > bytes_to_read)
		{
			//没有数据的时候没有输出
			nSamplesOut = 0;
			return 0;
		}
		memcpy(audioSamples, audio_buffer_.begin(), bytes_to_read);


	//	memmove(audio_buffer_.begin(), audio_buffer_.begin() + bytes_to_read, audio_buffer_.size() - bytes_to_read);
		//设置最新的buffer的大小
		audio_buffer_.SetData(audio_buffer_.begin() + bytes_to_read, audio_buffer_.size() - bytes_to_read);

		nSamplesOut = bytes_to_read / (nBytesPerSample * nChannels);



		return 0;
	}

	void AudioCapture::PullRenderData(int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames, void * audio_data, int64_t * elapsed_time_ms, int64_t * ntp_time_ms)
	{
	}

}