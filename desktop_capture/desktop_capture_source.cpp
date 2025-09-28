
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

#include "libcross_platform_collection_render/desktop_capture/desktop_capture_source.h"
#include "libyuv.h"
#include "api/video/i420_buffer.h"
#include "api/video/video_rotation.h"
#include "rtc_base/logging.h"

namespace libcross_platform_collection_render {

void DesktopCaptureSource::AddOrUpdateSink(
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
    const rtc::VideoSinkWants& wants) {
  broadcaster_.AddOrUpdateSink(sink, wants);
  UpdateVideoAdapter();
}

void DesktopCaptureSource::RemoveSink(
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) {
  broadcaster_.RemoveSink(sink);
  UpdateVideoAdapter();
}

void DesktopCaptureSource::UpdateVideoAdapter() {
  video_adapter_.OnSinkWants(broadcaster_.wants());
	rtc::VideoSinkWants wants = broadcaster_.wants();
	//video_adapter_.OnOutputFormatRequest( wants.resolutions);
}

void DesktopCaptureSource::OnFrame(const webrtc::VideoFrame& frame) {
  

	//RTC_LOG(LS_INFO) << "width :" << frame.width() << "height : " << frame.height();
	 
	//i420_buffer_->set_texture();
	//libyuv::ConvertToI420(frame->data(), 0, i420_buffer_->MutableDataY(),
	//	i420_buffer_->StrideY(), i420_buffer_->MutableDataU(),
	//	i420_buffer_->StrideU(), i420_buffer_->MutableDataV(),
	//	i420_buffer_->StrideV(), 0, 0, width, height, width,
	//	height, libyuv::kRotate0, libyuv::FOURCC_ARGB);
#if 1
	if (!libmedia_codec_i420_buffer_.get() ||
		libmedia_codec_i420_buffer_->width() * libmedia_codec_i420_buffer_->height() < frame.width() * frame.height()) {
		libmedia_codec_i420_buffer_ = libmedia_codec::I420Buffer::Create(frame.width() , frame.height());
		//libmedia_codec_i420_buffer_ = libmedia_codec::I420Buffer::Create(width, height);
	}


	int src_width = frame.width();
	int src_height = frame.height();
	int stridey = frame.video_frame_buffer()->GetI420()->StrideY();
	int strideu = frame.video_frame_buffer()->GetI420()->StrideU();
	int stridev = frame.video_frame_buffer()->GetI420()->StrideV();

	int size = stridey * src_height + (strideu + stridev) * ((src_height + 1) / 2);
	memcpy(libmedia_codec_i420_buffer_->MutableDataY(), frame.video_frame_buffer()->GetI420()->DataY(), stridey * src_height);
	memcpy(libmedia_codec_i420_buffer_->MutableDataU(), frame.video_frame_buffer()->GetI420()->DataU(), strideu * ((src_height + 1) / 2));
	memcpy(libmedia_codec_i420_buffer_->MutableDataV(), frame.video_frame_buffer()->GetI420()->DataV(), strideu * ((src_height + 1) / 2));

	//rtc::scoped_refptr<webrtc::VideoFrameBuffer> video_frame_buffer = frame.video_frame_buffer();
	//memcpy(libmedia_codec_i420_buffer_->MutableDataY(), frame.video_frame_buffer()->GetI420()->DataY(), ((frame.width() * frame.height()) + (frame.width() * frame.height() / 2)));
	//libyuv::ConvertToI420(video_frame_buffer->data(), 0, libmedia_codec_i420_buffer_->MutableDataY(),
	//	libmedia_codec_i420_buffer_->StrideY(), libmedia_codec_i420_buffer_->MutableDataU(),
	//	libmedia_codec_i420_buffer_->StrideU(), libmedia_codec_i420_buffer_->MutableDataV(),
	//	libmedia_codec_i420_buffer_->StrideV(), 0, 0, width, height, width,
	//	height, libyuv::kRotate0, libyuv::FOURCC_ARGB);
	//libyuv::ConvertToI420(frame->data(), 0, libmedia_codec_i420_buffer_->MutableDataY(),
	//	libmedia_codec_i420_buffer_->StrideY(), libmedia_codec_i420_buffer_->MutableDataU(),
	//	libmedia_codec_i420_buffer_->StrideU(), libmedia_codec_i420_buffer_->MutableDataV(),
	//	libmedia_codec_i420_buffer_->StrideV(), 0, 0, width, height, width,
	//	height, libyuv::kRotate0, libyuv::FOURCC_ARGB);
	
	if (x264_encoder_)
	{
		libmedia_codec::VideoFrame captureFrame =
			libmedia_codec::VideoFrame::Builder()
			.set_video_frame_buffer(libmedia_codec_i420_buffer_)
			.set_timestamp_rtp(rtc::TimeMillis())  // set_ntp_time_ms
			.set_ntp_time_ms(rtc::TimeMillis())
			.set_timestamp_ms(rtc::TimeMillis())
			.set_rotation(libmedia_codec::kVideoRotation_0)
			.build();
		std::shared_ptr< libmedia_codec::VideoFrame>   out = std::make_shared< libmedia_codec::VideoFrame>(captureFrame);
		// 很奇怪 为什么摄像头需要再同一个thread中调用
		singnal_thread_->PostTask(RTC_FROM_HERE, [this, out](){
			x264_encoder_->OnNewMediaFrame(out);
		});
		
	}
#endif // 
	broadcaster_.OnFrame(frame);
  return;
  //if (!video_adapter_.AdaptFrameResolution(
  //        frame.width(), frame.height(), frame.timestamp_us() * 1000,
  //        &cropped_width, &cropped_height, &out_width, &out_height)) {
  //  // Drop frame in order to respect frame rate constraint.
  //  return;
  //}

//  if (out_height != frame.height() || out_width != frame.width()) {
//    // Video adapter has requested a down-scale. Allocate a new buffer and
//    // return scaled version.
//    // For simplicity, only scale here without cropping.
//    rtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer =
//        webrtc::I420Buffer::Create(out_width, out_height);
//    scaled_buffer->ScaleFrom(*frame.video_frame_buffer()->ToI420());
//    webrtc::VideoFrame::Builder new_frame_builder =
//        webrtc::VideoFrame::Builder()
//            .set_video_frame_buffer(scaled_buffer)
//			.set_timestamp_rtp(0)
//			.set_timestamp_ms(rtc::TimeMillis())
//            .set_rotation(webrtc::kVideoRotation_0)
//            .set_timestamp_us(frame.timestamp_us())
//            .set_id(frame.id());
//    ;
//
//
//	/*
//	VideoFrame captureFrame =
//      VideoFrame::Builder()
//          .set_video_frame_buffer(buffer)
//          .set_timestamp_rtp(0)
//          .set_timestamp_ms(rtc::TimeMillis())
//          .set_rotation(!apply_rotation ? _rotateFrame : kVideoRotation_0)
//          .build();
//  captureFrame.set_ntp_time_ms(captureTime);
//  RTC_LOG(INFO) << "[chensong]ntp time ms = " << captureTime;
//	
//	*/
//    /*if (frame.has_update_rect()) {
//      webrtc::VideoFrame::UpdateRect new_rect =
//          frame.update_rect().ScaleWithFrame(frame.width(), frame.height(), 0,
//                                             0, frame.width(), frame.height(),
//                                             out_width, out_height);
//      new_frame_builder.set_update_rect(new_rect);
//    }*/
//    broadcaster_.OnFrame(new_frame_builder.build());
//  } else {
//    // No adaptations needed, just return the frame as is.
//
//    broadcaster_.OnFrame(frame);
//  }
}
void DesktopCaptureSource::set_catprue_callback(libmedia_codec::X264Encoder * x264_encoder, rtc::Thread* t)
{
	x264_encoder_ = x264_encoder;
	singnal_thread_ = t;
}
}  // namespace  