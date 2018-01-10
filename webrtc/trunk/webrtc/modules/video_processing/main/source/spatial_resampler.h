/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/*
 * spatial_resampler.h
 */

#ifndef VPM_SPATIAL_RESAMPLER_H
#define VPM_SPATIAL_RESAMPLER_H

#include "typedefs.h"

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_processing/main/interface/video_processing_defines.h"

#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "common_video/libyuv/include/scaler.h"

namespace webrtc {

class VPMSpatialResampler
{
public:
  virtual ~VPMSpatialResampler() {};
  virtual WebRtc_Word32 SetTargetFrameSize(WebRtc_Word32 width,
                                           WebRtc_Word32 height) = 0;
  virtual void SetInputFrameResampleMode(VideoFrameResampling
                                         resamplingMode) = 0;
  virtual void Reset() = 0;
  virtual WebRtc_Word32 ResampleFrame(const I420VideoFrame& inFrame,
                                      I420VideoFrame* outFrame) = 0;
  virtual WebRtc_Word32 CutI420Frame(I420VideoFrame& inFrame,
		                             WebRtc_UWord32 fromWidth, WebRtc_UWord32 fromHeight,
									 WebRtc_UWord32 toWidth, WebRtc_UWord32 toHeight) = 0;
  virtual WebRtc_Word32 CutI420FrameRo(I420VideoFrame& inFrame,
		                             WebRtc_UWord32 fromWidth, WebRtc_UWord32 fromHeight,
									 WebRtc_UWord32 toWidth, WebRtc_UWord32 toHeight) = 0;
  virtual WebRtc_Word32 ResampleFrame32(const I420VideoFrame& inFrame,
			                                  I420VideoFrame* outFrame) = 0;
  virtual WebRtc_Word32 TargetWidth() = 0;
  virtual WebRtc_Word32 TargetHeight() = 0;
  virtual bool ApplyResample(WebRtc_Word32 width, WebRtc_Word32 height) = 0;
};

class VPMSimpleSpatialResampler : public VPMSpatialResampler
{
public:
  VPMSimpleSpatialResampler();
  ~VPMSimpleSpatialResampler();
  virtual WebRtc_Word32 SetTargetFrameSize(WebRtc_Word32 width,
                                           WebRtc_Word32 height);
  virtual void SetInputFrameResampleMode(VideoFrameResampling resamplingMode);
  virtual void Reset();
  virtual WebRtc_Word32 ResampleFrame(const I420VideoFrame& inFrame,
                                      I420VideoFrame* outFrame);
  virtual WebRtc_Word32 CutI420Frame(I420VideoFrame& inFrame,
		                              WebRtc_UWord32 fromWidth, WebRtc_UWord32 fromHeight,
									  WebRtc_UWord32 toWidth, WebRtc_UWord32 toHeight);
  virtual WebRtc_Word32 CutI420FrameRo(I420VideoFrame& inFrame,
		                             WebRtc_UWord32 fromWidth, WebRtc_UWord32 fromHeight,
									 WebRtc_UWord32 toWidth, WebRtc_UWord32 toHeight);
  virtual WebRtc_Word32 ResampleFrame32(const I420VideoFrame& inFrame,
			                                  I420VideoFrame* outFrame);
  virtual WebRtc_Word32 TargetWidth();
  virtual WebRtc_Word32 TargetHeight();
  virtual bool ApplyResample(WebRtc_Word32 width, WebRtc_Word32 height);

private:

  VideoFrameResampling        _resamplingMode;
  WebRtc_Word32               _targetWidth;
  WebRtc_Word32               _targetHeight;
  Scaler                      _scaler;
};

} //namespace

#endif