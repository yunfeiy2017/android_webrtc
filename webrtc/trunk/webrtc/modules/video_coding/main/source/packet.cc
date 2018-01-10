/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "packet.h"
#include "module_common_types.h"

#include <assert.h>

namespace webrtc {

VCMPacket::VCMPacket()
  :
    payloadType(0),
    timestamp(0),
    seqNum(0),
    dataPtr(NULL),
    sizeBytes(0),
    markerBit(false),
#ifndef GXH_TEST_H264
	allocIdx(-1),
	encIdx(-1),
#endif
    frameType(kFrameEmpty),
    codec(kVideoCodecUnknown),
    isFirstPacket(false),
    completeNALU(kNaluUnset),
    insertStartCode(false),
    codecSpecificHeader() {
}

VCMPacket::VCMPacket(const WebRtc_UWord8* ptr,
                               const WebRtc_UWord32 size,
                               const WebRtcRTPHeader& rtpHeader) :
    payloadType(rtpHeader.header.payloadType),
    timestamp(rtpHeader.header.timestamp),
    seqNum(rtpHeader.header.sequenceNumber),
    dataPtr(ptr),
    sizeBytes(size),
    markerBit(rtpHeader.header.markerBit),
#ifndef GXH_TEST_H264
	allocIdx(rtpHeader.header.allocIdx),
	encIdx(rtpHeader.header.encIdx),
#endif
    frameType(rtpHeader.frameType),
    codec(kVideoCodecUnknown),
    isFirstPacket(rtpHeader.type.Video.isFirstPacket),
    completeNALU(kNaluComplete),
    insertStartCode(false),
    codecSpecificHeader(rtpHeader.type.Video)
{
    CopyCodecSpecifics(rtpHeader.type.Video);
}

VCMPacket::VCMPacket(const WebRtc_UWord8* ptr, WebRtc_UWord32 size, WebRtc_UWord16 seq, WebRtc_UWord32 ts, bool mBit) :
    payloadType(0),
    timestamp(ts),
    seqNum(seq),
    dataPtr(ptr),
    sizeBytes(size),
    markerBit(mBit),

    frameType(kVideoFrameDelta),
    codec(kVideoCodecUnknown),
    isFirstPacket(false),
    completeNALU(kNaluComplete),
    insertStartCode(false),
    codecSpecificHeader()
{}

void VCMPacket::Reset() {
  payloadType = 0;
  timestamp = 0;
  seqNum = 0;
  dataPtr = NULL;
  sizeBytes = 0;
  markerBit = false;
  frameType = kFrameEmpty;
  codec = kVideoCodecUnknown;
  isFirstPacket = false;
  completeNALU = kNaluUnset;
  insertStartCode = false;
  memset(&codecSpecificHeader, 0, sizeof(RTPVideoHeader));
}

void VCMPacket::CopyCodecSpecifics(const RTPVideoHeader& videoHeader)
{
    switch(videoHeader.codec)
    {
        case kRTPVideoVP8:
            {
                // Handle all packets within a frame as depending on the previous packet
                // TODO(holmer): This should be changed to make fragments independent
                // when the VP8 RTP receiver supports fragments.
                if (isFirstPacket && markerBit)
                    completeNALU = kNaluComplete;
                else if (isFirstPacket)
                    completeNALU = kNaluStart;
                else if (markerBit)
                    completeNALU = kNaluEnd;
                else
                    completeNALU = kNaluIncomplete;

                codec = kVideoCodecVP8;
                break;
            }
#ifndef GXH_TEST_H264
		case kRTPVideoH264:
            {
                codec = kVideoCodecH264;
                break;
            }
		case kRTPVideoSVC:
            {
                codec = kVideoCodecSVC;
                break;
            }
#endif
        case kRTPVideoI420:
            {
                codec = kVideoCodecI420;
                break;
            }
        default:
            {
                codec = kVideoCodecUnknown;
                break;
            }
    }
}

}
