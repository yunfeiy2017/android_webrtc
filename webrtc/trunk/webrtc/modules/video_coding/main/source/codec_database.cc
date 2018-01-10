/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/video_coding/main/source/codec_database.h"

#include <assert.h>

#include "webrtc/engine_configurations.h"
#ifdef VIDEOCODEC_I420
#include "webrtc/modules/video_coding/codecs/i420/main/interface/i420.h"
#endif
#ifdef VIDEOCODEC_VP8
#include "webrtc/modules/video_coding/codecs/vp8/include/vp8.h"
#endif
#include "webrtc/modules/video_coding/main/source/internal_defines.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

VCMDecoderMapItem::VCMDecoderMapItem(VideoCodec* settings,
                                     int number_of_cores,
                                     bool require_key_frame)
    : settings(settings),
      number_of_cores(number_of_cores),
      require_key_frame(require_key_frame) {
  assert(number_of_cores >= 0);
}

VCMExtDecoderMapItem::VCMExtDecoderMapItem(
    VideoDecoder* external_decoder_instance,
    uint8_t payload_type,
    bool internal_render_timing)
    : payload_type(payload_type),
      external_decoder_instance(external_decoder_instance),
      internal_render_timing(internal_render_timing) {
}

VCMCodecDataBase::VCMCodecDataBase(int id)
    : id_(id),
      number_of_cores_(0),
      max_payload_size_(kDefaultPayloadSize),
      periodic_key_frames_(false),
      current_enc_is_external_(false),
      send_codec_(),
      receive_codec_(),
      external_payload_type_(0),
      external_encoder_(NULL),
      internal_source_(false),
      ptr_encoder_(NULL),
      ptr_decoder_(NULL),
      current_dec_is_external_(false),
      dec_map_(),
      dec_external_map_() {
}

VCMCodecDataBase::~VCMCodecDataBase() {
  ResetSender();
  ResetReceiver();
}

int VCMCodecDataBase::NumberOfCodecs() {
  return VCM_NUM_VIDEO_CODECS_AVAILABLE;
}

bool VCMCodecDataBase::Codec(int list_id,
                             VideoCodec* settings) {
  if (!settings) {
    return false;
  }
  if (list_id >= VCM_NUM_VIDEO_CODECS_AVAILABLE) {
    return false;
  }
  memset(settings, 0, sizeof(VideoCodec));
  switch (list_id) {
#ifdef VIDEOCODEC_VP8
    case VCM_VP8_IDX: {
      strncpy(settings->plName, "VP8", 4);
      settings->codecType = kVideoCodecVP8;
      // 96 to 127 dynamic payload types for video codecs.
      settings->plType = VCM_VP8_PAYLOAD_TYPE;
      settings->startBitrate = 100;
      settings->minBitrate = VCM_MIN_BITRATE;
      settings->maxBitrate = 0;
      settings->maxFramerate = VCM_DEFAULT_FRAME_RATE;
      settings->width_used = VCM_DEFAULT_CODEC_WIDTH;
      settings->height_used = VCM_DEFAULT_CODEC_HEIGHT;
      settings->numberOfSimulcastStreams = 0;
      settings->codecSpecific.VP8.resilience = kResilientStream;
      settings->codecSpecific.VP8.numberOfTemporalLayers = 1;
      settings->codecSpecific.VP8.denoisingOn = true;
      settings->codecSpecific.VP8.errorConcealmentOn = false;
      settings->codecSpecific.VP8.automaticResizeOn = false;
      settings->codecSpecific.VP8.frameDroppingOn = true;
      settings->codecSpecific.VP8.keyFrameInterval = 3000;
      return true;
    }
#endif
#ifdef VIDEOCODEC_I420
    case VCM_I420_IDX: {
      strncpy(settings->plName, "I420", 5);
      settings->codecType = kVideoCodecI420;
      // 96 to 127 dynamic payload types for video codecs.
      settings->plType = VCM_I420_PAYLOAD_TYPE;
      // Bitrate needed for this size and framerate.
      settings->startBitrate = 3 * VCM_DEFAULT_CODEC_WIDTH *
                               VCM_DEFAULT_CODEC_HEIGHT * 8 *
                               VCM_DEFAULT_FRAME_RATE / 1000 / 2;
      settings->maxBitrate = settings->startBitrate;
      settings->maxFramerate = VCM_DEFAULT_FRAME_RATE;
      settings->width_used = VCM_DEFAULT_CODEC_WIDTH;
      settings->height_used = VCM_DEFAULT_CODEC_HEIGHT;
      settings->minBitrate = VCM_MIN_BITRATE;
      settings->numberOfSimulcastStreams = 0;
      return true;
    }
#endif
#ifndef GXH_TEST_H264
    case VCM_H264_IDX: {
      strncpy(settings->plName, "H264", 4);
      settings->codecType = kVideoCodecH264;
      // 96 to 127 dynamic payload types for video codecs.
      settings->plType = VCM_H264_PAYLOAD_TYPE;
      // Bitrate needed for this size and framerate.
      settings->startBitrate = 3 * VCM_DEFAULT_CODEC_WIDTH *
                               VCM_DEFAULT_CODEC_HEIGHT * 8 *
                               VCM_DEFAULT_FRAME_RATE / 1000 / 2;
      settings->maxBitrate = settings->startBitrate;
      settings->maxFramerate = VCM_DEFAULT_FRAME_RATE;
      settings->width_used = VCM_DEFAULT_CODEC_WIDTH;
      settings->height_used = VCM_DEFAULT_CODEC_HEIGHT;
      settings->minBitrate = VCM_MIN_BITRATE;
      settings->numberOfSimulcastStreams = 0;
      return true;
    }
   case VCM_SVC_IDX: {
      strncpy(settings->plName, "SVC", 3);
      settings->codecType = kVideoCodecSVC;
      // 96 to 127 dynamic payload types for video codecs.
      settings->plType = VCM_SVC_PAYLOAD_TYPE;
      // Bitrate needed for this size and framerate.
      settings->startBitrate = 3 * VCM_DEFAULT_CODEC_WIDTH *
                               VCM_DEFAULT_CODEC_HEIGHT * 8 *
                               VCM_DEFAULT_FRAME_RATE / 1000 / 2;
      settings->maxBitrate = settings->startBitrate;
      settings->maxFramerate = VCM_DEFAULT_FRAME_RATE;
      settings->width_used = VCM_DEFAULT_CODEC_WIDTH;
      settings->height_used = VCM_DEFAULT_CODEC_HEIGHT;
      settings->minBitrate = VCM_MIN_BITRATE;
      settings->numberOfSimulcastStreams = 0;
      return true;
    }
#endif
    default: {
      return false;
    }
  }
}

bool VCMCodecDataBase::Codec(VideoCodecType codec_type,
                             VideoCodec* settings) {
  for (int i = 0; i < VCMCodecDataBase::NumberOfCodecs(); i++) {
    const bool ret = VCMCodecDataBase::Codec(i, settings);
    if (!ret) {
      return false;
    }
    if (codec_type == settings->codecType) {
      return true;
    }
  }
  return false;
}

void VCMCodecDataBase::ResetSender() {
  DeleteEncoder();
  periodic_key_frames_ = false;
}

// Assuming only one registered encoder - since only one used, no need for more.
bool VCMCodecDataBase::RegisterSendCodec(
    const VideoCodec* send_codec,
    int number_of_cores,
    int max_payload_size) {
  if (!send_codec) {
    return false;
  }
  if (max_payload_size <= 0) {
    max_payload_size = kDefaultPayloadSize;
  }
  if (number_of_cores < 0 || number_of_cores > 32) {
    return false;
  }
  if (send_codec->plType <= 0) {
    return false;
  }
  // Make sure the start bit rate is sane...
  if (send_codec->startBitrate > 1000000) {
    return false;
  }
  if (send_codec->codecType == kVideoCodecUnknown) {
    return false;
  }
  number_of_cores_ = number_of_cores;
  max_payload_size_ = max_payload_size;

  memcpy(&send_codec_, send_codec, sizeof(VideoCodec));

  if (send_codec_.maxBitrate == 0) {
    // max is one bit per pixel
    send_codec_.maxBitrate = (static_cast<int>(send_codec_.height_used) *
        static_cast<int>(send_codec_.width_used) *
        static_cast<int>(send_codec_.maxFramerate)) / 1000;
    if (send_codec_.startBitrate > send_codec_.maxBitrate) {
      // But if the user tries to set a higher start bit rate we will
      // increase the max accordingly.
      send_codec_.maxBitrate = send_codec_.startBitrate;
    }
  }

  return true;
}

bool VCMCodecDataBase::SendCodec(VideoCodec* current_send_codec) const {
  WEBRTC_TRACE(webrtc::kTraceApiCall, webrtc::kTraceVideoCoding, VCMId(id_),
               "SendCodec");
  if (!ptr_encoder_) {
    return false;
  }
  memcpy(current_send_codec, &send_codec_, sizeof(VideoCodec));
  return true;
}

VideoCodecType VCMCodecDataBase::SendCodec() const {
  WEBRTC_TRACE(webrtc::kTraceApiCall, webrtc::kTraceVideoCoding, VCMId(id_),
               "SendCodec type");
  if (!ptr_encoder_) {
    return kVideoCodecUnknown;
  }
  return send_codec_.codecType;
}

bool VCMCodecDataBase::DeregisterExternalEncoder(
    uint8_t payload_type, bool* was_send_codec) {
  assert(was_send_codec);
  *was_send_codec = false;
  if (external_payload_type_ != payload_type) {
    return false;
  }
  if (send_codec_.plType == payload_type) {
    // De-register as send codec if needed.
    DeleteEncoder();
    memset(&send_codec_, 0, sizeof(VideoCodec));
    current_enc_is_external_ = false;
    *was_send_codec = true;
  }
  external_payload_type_ = 0;
  external_encoder_ = NULL;
  internal_source_ = false;
  return true;
}

void VCMCodecDataBase::RegisterExternalEncoder(
    VideoEncoder* external_encoder,
    uint8_t payload_type,
    bool internal_source) {
  // Since only one encoder can be used at a given time, only one external
  // encoder can be registered/used.
  external_encoder_ = external_encoder;
  external_payload_type_ = payload_type;
  internal_source_ = internal_source;
}

VCMGenericEncoder* VCMCodecDataBase::GetEncoder(
    const VideoCodec* settings,
    VCMEncodedFrameCallback* encoded_frame_callback) {
  // If encoder exists, will destroy it and create new one.
  DeleteEncoder();
  if (settings->plType == external_payload_type_) {
    // External encoder.
    ptr_encoder_ = new VCMGenericEncoder(*external_encoder_, internal_source_);
    current_enc_is_external_ = true;
  } else {
    ptr_encoder_ = CreateEncoder(settings->codecType);
    current_enc_is_external_ = false;
  }
  encoded_frame_callback->SetPayloadType(settings->plType);
  if (!ptr_encoder_) {
    WEBRTC_TRACE(webrtc::kTraceError,
                 webrtc::kTraceVideoCoding,
                 VCMId(id_),
                 "Failed to create encoder: %s.",
                 settings->plName);
    return NULL;
  }
  if (ptr_encoder_->InitEncode(settings, number_of_cores_, max_payload_size_) <
      0) {
    WEBRTC_TRACE(webrtc::kTraceError,
                 webrtc::kTraceVideoCoding,
                 VCMId(id_),
                 "Failed to initialize encoder: %s.",
                 settings->plName);
    DeleteEncoder();
    return NULL;
  } else if (ptr_encoder_->RegisterEncodeCallback(encoded_frame_callback) <
      0) {
    DeleteEncoder();
    return NULL;
  }
#ifdef GXH_TEST
    if (current_enc_is_external_)
    {
        const webrtc::I420VideoFrame frame;
        const webrtc::CodecSpecificInfo * info;
        const std::vector<FrameType> frameTypes;
        //std::vector<VideoFrameType> video_frame_types(frameTypes.size(),kDeltaFrame);
        ptr_encoder_->Encode(
                             frame,
                             info,
                             frameTypes
                             );
        
    }
#endif
  // Intentionally don't check return value since the encoder registration
  // shouldn't fail because the codec doesn't support changing the periodic key
  // frame setting.
  ptr_encoder_->SetPeriodicKeyFrames(periodic_key_frames_);
  return ptr_encoder_;
}

bool VCMCodecDataBase::SetPeriodicKeyFrames(bool enable) {
  periodic_key_frames_ = enable;
  if (ptr_encoder_) {
    return (ptr_encoder_->SetPeriodicKeyFrames(periodic_key_frames_) == 0);
  }
  return true;
}

void VCMCodecDataBase::ResetReceiver() {
  ReleaseDecoder(ptr_decoder_);
  ptr_decoder_ = NULL;
  memset(&receive_codec_, 0, sizeof(VideoCodec));
  while (!dec_map_.empty()) {
    DecoderMap::iterator it = dec_map_.begin();
    delete (*it).second;
    dec_map_.erase(it);
  }
  while (!dec_external_map_.empty()) {
    ExternalDecoderMap::iterator external_it = dec_external_map_.begin();
    delete (*external_it).second;
    dec_external_map_.erase(external_it);
  }
  current_dec_is_external_ = false;
}

bool VCMCodecDataBase::DeregisterExternalDecoder(uint8_t payload_type) {
  ExternalDecoderMap::iterator it = dec_external_map_.find(payload_type);
  if (it == dec_external_map_.end()) {
    // Not found
    return false;
  }
  if (receive_codec_.plType == payload_type) {
    // Release it if it was registered and in use.
    ReleaseDecoder(ptr_decoder_);
    ptr_decoder_ = NULL;
  }
  DeregisterReceiveCodec(payload_type);
  delete (*it).second;
  dec_external_map_.erase(it);
  return true;
}

// Add the external encoder object to the list of external decoders.
// Won't be registered as a receive codec until RegisterReceiveCodec is called.
bool VCMCodecDataBase::RegisterExternalDecoder(
    VideoDecoder* external_decoder,
    uint8_t payload_type,
    bool internal_render_timing) {
  // Check if payload value already exists, if so  - erase old and insert new.
  VCMExtDecoderMapItem* ext_decoder = new VCMExtDecoderMapItem(
      external_decoder, payload_type, internal_render_timing);
  if (!ext_decoder) {
    return false;
  }
  DeregisterExternalDecoder(payload_type);
  dec_external_map_[payload_type] = ext_decoder;
  return true;
}

bool VCMCodecDataBase::DecoderRegistered() const {
  return !dec_map_.empty();
}

bool VCMCodecDataBase::RegisterReceiveCodec(
    const VideoCodec* receive_codec,
    int number_of_cores,
    bool require_key_frame) {
  if (number_of_cores < 0) {
    return false;
  }
  WEBRTC_TRACE(webrtc::kTraceStateInfo, webrtc::kTraceVideoCoding, VCMId(id_),
               "Codec: %s, Payload type %d, Height %d, Width %d, Bitrate %d,"
               "Framerate %d.",
               receive_codec->plName, receive_codec->plType,
               receive_codec->height_used, receive_codec->width_used,
               receive_codec->startBitrate, receive_codec->maxFramerate);
  // Check if payload value already exists, if so  - erase old and insert new.
  DeregisterReceiveCodec(receive_codec->plType);
  if (receive_codec->codecType == kVideoCodecUnknown) {
    return false;
  }
  VideoCodec* new_receive_codec = new VideoCodec(*receive_codec);
  dec_map_[receive_codec->plType] = new VCMDecoderMapItem(new_receive_codec,
                                                          number_of_cores,
                                                          require_key_frame);
  return true;
}

bool VCMCodecDataBase::DeregisterReceiveCodec(
    uint8_t payload_type) {
  DecoderMap::iterator it = dec_map_.find(payload_type);
  if (it == dec_map_.end()) {
    return false;
  }
  VCMDecoderMapItem* dec_item = (*it).second;
  delete dec_item;
  dec_map_.erase(it);
  if (receive_codec_.plType == payload_type) {
    // This codec is currently in use.
    memset(&receive_codec_, 0, sizeof(VideoCodec));
    current_dec_is_external_ = false;
  }
  return true;
}

bool VCMCodecDataBase::ReceiveCodec(VideoCodec* current_receive_codec) const {
  assert(current_receive_codec);
  if (!ptr_decoder_) {
    return false;
  }
  memcpy(current_receive_codec, &receive_codec_, sizeof(VideoCodec));
  return true;
}

VideoCodecType VCMCodecDataBase::ReceiveCodec() const {
  if (!ptr_decoder_) {
    return kVideoCodecUnknown;
  }
  return receive_codec_.codecType;
}

VCMGenericDecoder* VCMCodecDataBase::GetDecoder(
    uint8_t payload_type, 
#ifndef GXH_TEST_H264
	int8_t	allocIdx,
	int8_t	encIdx,
	uint8_t *buffer,
#endif
	VCMDecodedFrameCallback* decoded_frame_callback) {

 
  if (payload_type == receive_codec_.plType || payload_type == 0) {
    return ptr_decoder_;
  }
  // Check for exisitng decoder, if exists - delete.
  if (ptr_decoder_) {
    ReleaseDecoder(ptr_decoder_);
    ptr_decoder_ = NULL;
    memset(&receive_codec_, 0, sizeof(VideoCodec));
  }
  ptr_decoder_ = CreateAndInitDecoder(payload_type, 
#ifndef GXH_TEST_H264
									  allocIdx,
									  encIdx,
									  buffer,
#endif
									  &receive_codec_,
                                      &current_dec_is_external_);
  if (!ptr_decoder_) {
    return NULL;


  }
  if (ptr_decoder_->RegisterDecodeCompleteCallback(decoded_frame_callback)
      < 0) {
		  WEBRTC_TRACE(kTraceInfo, kTraceVideo, 0,
			  "RegisterDecodeCompleteCallback is failed");
    ReleaseDecoder(ptr_decoder_);
    ptr_decoder_ = NULL;
    memset(&receive_codec_, 0, sizeof(VideoCodec));


    return NULL;
  }
  return ptr_decoder_;
}

VCMGenericDecoder* VCMCodecDataBase::CreateDecoderCopy() const {
  if (!ptr_decoder_) {
    return NULL;
  }
  VideoDecoder* decoder_copy = ptr_decoder_->_decoder.Copy();
  if (!decoder_copy) {
    return NULL;
  }
  return new VCMGenericDecoder(*decoder_copy, id_, ptr_decoder_->External());
}

void VCMCodecDataBase::ReleaseDecoder(VCMGenericDecoder* decoder) const {
  if (decoder) {
    assert(&decoder->_decoder);
    decoder->Release();
    if (!decoder->External()) {
      delete &decoder->_decoder;
    }
    delete decoder;
  }
}

void VCMCodecDataBase::CopyDecoder(const VCMGenericDecoder& decoder) {
  VideoDecoder* decoder_copy = decoder._decoder.Copy();
  if (decoder_copy) {
    VCMDecodedFrameCallback* cb = ptr_decoder_->_callback;
    ReleaseDecoder(ptr_decoder_);
    ptr_decoder_ = new VCMGenericDecoder(*decoder_copy, id_,
                                         decoder.External());
    if (cb && ptr_decoder_->RegisterDecodeCompleteCallback(cb)) {
      assert(false);
    }
  }
}

bool VCMCodecDataBase::SupportsRenderScheduling() const {
  bool render_timing = true;
  if (current_dec_is_external_) {
    const VCMExtDecoderMapItem* ext_item = FindExternalDecoderItem(
        receive_codec_.plType);
    render_timing = ext_item->internal_render_timing;
  }
  return render_timing;
}

VCMGenericDecoder* VCMCodecDataBase::GetDecoderPrivate( uint8_t payload_type)
{
	//to pass payloadtype;
	//payload_type = 0;
	//WEBRTC_TRACE(kTraceInfo, kTraceVideo, 0,  "VCMCodecDataBase GetDecoderPrivate receive_codec_plType is :%d", receive_codec_.plType);
	if (payload_type == receive_codec_.plType || payload_type == 0) 
	{
		return ptr_decoder_;
	}
	else
	{
		WEBRTC_TRACE(kTraceInfo, kTraceVideo, 0,  "VCMCodecDataBase GetDecoder failed");
		return NULL;
	}
}

VCMGenericDecoder* VCMCodecDataBase::CreateAndInitDecoder(
    uint8_t payload_type,
#ifndef GXH_TEST_H264
	int8_t allocIdx,
	int8_t encIdx,
	uint8_t *buffer,
#endif
    VideoCodec* new_codec,
    bool* external) const {
  assert(external);
  assert(new_codec);
  const VCMDecoderMapItem* decoder_item = FindDecoderItem(payload_type);
  if (!decoder_item) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCoding, VCMId(id_),
                 "Unknown payload type: %u", payload_type);
    return NULL;
  }
	VideoCodec*  decoder_item_setting = (VideoCodec*) malloc(sizeof(VideoCodec));
	memcpy(decoder_item_setting,decoder_item->settings.get(),sizeof(VideoCodec));
	scoped_ptr<VideoCodec> settings_temp(decoder_item_setting);
	 int number_of_cores_temp = decoder_item->number_of_cores;
	 bool require_key_frame_temp = decoder_item->require_key_frame;
  VCMGenericDecoder* ptr_decoder = NULL;
  const VCMExtDecoderMapItem* external_dec_item = FindExternalDecoderItem(
                                              payload_type);
  if (external_dec_item) {
    // External codec.
    ptr_decoder = new VCMGenericDecoder(
        *external_dec_item->external_decoder_instance, id_, true);
    *external = true;
  } else {
    // Create decoder.
				// ptr_decoder = CreateDecoder(decoder_item->settings->codecType);
				ptr_decoder = CreateDecoder(settings_temp->codecType);
    *external = false;
  }
  if (!ptr_decoder) {
    return NULL;
  }
  WEBRTC_TRACE(
	  kTraceInfo,
	  kTraceVideoCoding,
	  -1,
	  "CreateDecoder is successful"); 

#ifndef GXH_TEST_H264
			// VideoCodec* settings = decoder_item->settings.get();
		/*	VideoCodec* settings = decoder_item_temp->settings.get();
			settings->allocIdx = allocIdx;
			settings->encIdx = encIdx;*/
			settings_temp->allocIdx = allocIdx;
			settings_temp->encIdx = encIdx;
			settings_temp->buffer = (char *)buffer;
#endif
			if (ptr_decoder->InitDecode(settings_temp.get(),
				number_of_cores_temp,
				 require_key_frame_temp) < 0) {
					ReleaseDecoder(ptr_decoder);

	WEBRTC_TRACE(
		kTraceError,
		kTraceVideoCoding,
		-1,
		"CreateAndInitDecoder is failed"); 
    return NULL;
  }
			memcpy(new_codec, settings_temp.get(), sizeof(VideoCodec));

  WEBRTC_TRACE(
	  kTraceInfo,
	  kTraceVideoCoding,
	  -1,
	  "CreateAndInitDecoder is successful"); 
  return ptr_decoder;
}

VCMGenericEncoder* VCMCodecDataBase::CreateEncoder(
  const VideoCodecType type) const {
  switch (type) {
#ifdef VIDEOCODEC_VP8
    case kVideoCodecVP8:
      return new VCMGenericEncoder(*(VP8Encoder::Create()));
#endif
#ifndef GXH_TEST_H264
    case kVideoCodecH264:
	case kVideoCodecSVC:
          return new VCMGenericEncoder(*(new I420Encoder));
#endif
#ifdef VIDEOCODEC_I420
    case kVideoCodecI420:
          return new VCMGenericEncoder(*(new I420Encoder_14));
#endif
    default:
      return NULL;
  }
}

void VCMCodecDataBase::DeleteEncoder() {
  if (ptr_encoder_) {
    ptr_encoder_->Release();
    if (!current_enc_is_external_) {
      delete &ptr_encoder_->_encoder;
    }
#ifndef GXH_TEST
    else {
        printf("release encoder_______________________");
    }
#endif
    delete ptr_encoder_;
    ptr_encoder_ = NULL;
  }
}

VCMGenericDecoder* VCMCodecDataBase::CreateDecoder(VideoCodecType type) const {
  switch (type) {
#ifdef VIDEOCODEC_VP8
    case kVideoCodecVP8:
      return new VCMGenericDecoder(*(VP8Decoder::Create()), id_);
#endif
#ifndef GXH_TEST_H264
    case kVideoCodecH264:
	case kVideoCodecSVC:
          return new VCMGenericDecoder(*(new I420Decoder), id_);
#endif
#ifdef VIDEOCODEC_I420
    case kVideoCodecI420:
          return new VCMGenericDecoder(*(new I420Decoder_14), id_);
#endif
    default:
      return NULL;
  }
}

const VCMDecoderMapItem* VCMCodecDataBase::FindDecoderItem(
    uint8_t payload_type) const {
  DecoderMap::const_iterator it = dec_map_.find(payload_type);
  if (it != dec_map_.end()) {
    return (*it).second;
  }
  return NULL;
}

const VCMExtDecoderMapItem* VCMCodecDataBase::FindExternalDecoderItem(
    uint8_t payload_type) const {
  ExternalDecoderMap::const_iterator it = dec_external_map_.find(payload_type);
  if (it != dec_external_map_.end()) {
    return (*it).second;
  }
  return NULL;
}
}  // namespace webrtc
