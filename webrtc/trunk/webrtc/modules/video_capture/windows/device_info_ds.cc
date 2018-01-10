/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "device_info_ds.h"

#include "../video_capture_config.h"
#include "../video_capture_delay.h"
#include "help_functions_ds.h"
#include "ref_count.h"
#include "trace.h"

#include <Streams.h>
#include <Dvdmedia.h>

namespace webrtc
{
namespace videocapturemodule
{
	typedef struct CameraInfo
	{
		char mainType[30];
		char subType[30];
		long width;
		long height;
		double frameRate;
	}CAMERAINFO;
const WebRtc_Word32 NoWindowsCaptureDelays = 1;
const DelayValues WindowsCaptureDelays[NoWindowsCaptureDelays] = {
  "Microsoft LifeCam Cinema",
  "usb#vid_045e&pid_075d",
  {
    {640,480,125},
    {640,360,117},
    {424,240,111},
    {352,288,111},
    {320,240,116},
    {176,144,101},
    {160,120,109},
    {1280,720,166},
    {960,544,126},
    {800,448,120},
    {800,600,127}
  },
};

// static
DeviceInfoDS* DeviceInfoDS::Create(const WebRtc_Word32 id)
{
    DeviceInfoDS* dsInfo = new DeviceInfoDS(id);
    if (!dsInfo || dsInfo->Init() != 0)
    {
        delete dsInfo;
        dsInfo = NULL;
    }
    return dsInfo;
}

DeviceInfoDS::DeviceInfoDS(const WebRtc_Word32 id)
    : DeviceInfoImpl(id), _dsDevEnum(NULL), _dsMonikerDevEnum(NULL),
      _CoUninitializeIsRequired(true)
{
    // 1) Initialize the COM library (make Windows load the DLLs).
    //
    // CoInitializeEx must be called at least once, and is usually called only once,
    // for each thread that uses the COM library. Multiple calls to CoInitializeEx
    // by the same thread are allowed as long as they pass the same concurrency flag,
    // but subsequent valid calls return S_FALSE.
    // To close the COM library gracefully on a thread, each successful call to
    // CoInitializeEx, including any call that returns S_FALSE, must be balanced
    // by a corresponding call to CoUninitialize.
    //

    /*Apartment-threading, while allowing for multiple threads of execution,
     serializes all incoming calls by requiring that calls to methods of objects created by this thread always run on the same thread
     the apartment/thread that created them. In addition, calls can arrive only at message-queue boundaries (i.e., only during a
     PeekMessage, SendMessage, DispatchMessage, etc.). Because of this serialization, it is not typically necessary to write concurrency control into
     the code for the object, other than to avoid calls to PeekMessage and SendMessage during processing that must not be interrupted by other method
     invocations or calls to other objects in the same apartment/thread.*/

    ///CoInitializeEx(NULL, COINIT_APARTMENTTHREADED ); //| COINIT_SPEED_OVER_MEMORY
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED); // Use COINIT_MULTITHREADED since Voice Engine uses COINIT_MULTITHREADED
    if (FAILED(hr))
    {
        // Avoid calling CoUninitialize() since CoInitializeEx() failed.
        _CoUninitializeIsRequired = FALSE;

        if (hr == RPC_E_CHANGED_MODE)
        {
            // Calling thread has already initialized COM to be used in a single-threaded
            // apartment (STA). We are then prevented from using STA.
            // Details: hr = 0x80010106 <=> "Cannot change thread mode after it is set".
            //
            WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCapture, _id,
                         "VideoCaptureWindowsDSInfo::VideoCaptureWindowsDSInfo "
                         "CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) => "
                         "RPC_E_CHANGED_MODE, error 0x%x",
                         hr);
        }
    }

	InitializeCriticalSection(&sec);
	InitializeCriticalSection(&sec2);

	m_graphBuilder =NULL;
	m_pCaptureBuild2 =NULL;
}

DeviceInfoDS::~DeviceInfoDS()
{
	RELEASE_AND_CLEAR(m_pCaptureBuild2); 
	RELEASE_AND_CLEAR(m_graphBuilder);
	
    RELEASE_AND_CLEAR(_dsMonikerDevEnum);
    RELEASE_AND_CLEAR(_dsDevEnum);
	
	
    if (_CoUninitializeIsRequired)
    {
        CoUninitialize();
    }
	
}

WebRtc_Word32 DeviceInfoDS::Init()
{

    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
                                  IID_ICreateDevEnum, (void **) &_dsDevEnum);
    if (hr != NOERROR)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Failed to create CLSID_SystemDeviceEnum, error 0x%x", hr);
        return -1;
    } 
	RELEASE_AND_CLEAR(m_pCaptureBuild2); 
	RELEASE_AND_CLEAR(m_graphBuilder);
	// Get the interface for DirectShow's GraphBuilder
	 if(m_graphBuilder==NULL)
	 {
		 hr = CoCreateInstance(CLSID_FilterGraph, NULL,CLSCTX_INPROC_SERVER, IID_IGraphBuilder,(void **) &m_graphBuilder);
		 if (FAILED(hr))
		 {
			 WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
				 "Failed to create graph builder.");
			 return -1;
		 }
	 }
	 //init capture build

	 if(m_pCaptureBuild2==NULL)
	 {
		 hr = CoCreateInstance (CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC,IID_ICaptureGraphBuilder2, (void **) &m_pCaptureBuild2);
		 if (FAILED(hr))
			 return -1;
	 }
    return 0;
}
WebRtc_UWord32 DeviceInfoDS::NumberOfDevices()
{
    ReadLockScoped cs(_apiLock);
    return GetDeviceInfo(0, 0, 0, 0, 0, 0, 0);
}

WebRtc_Word32 DeviceInfoDS::GetDeviceName(
                                       WebRtc_UWord32 deviceNumber,
                                       char* deviceNameUTF8,
                                       WebRtc_UWord32 deviceNameLength,
                                       char* deviceUniqueIdUTF8,
                                       WebRtc_UWord32 deviceUniqueIdUTF8Length,
                                       char* productUniqueIdUTF8,
                                       WebRtc_UWord32 productUniqueIdUTF8Length)
{
    ReadLockScoped cs(_apiLock);
    const WebRtc_Word32 result = GetDeviceInfo(deviceNumber, deviceNameUTF8,
                                               deviceNameLength,
                                               deviceUniqueIdUTF8,
                                               deviceUniqueIdUTF8Length,
                                               productUniqueIdUTF8,
                                               productUniqueIdUTF8Length);
    return result > (WebRtc_Word32) deviceNumber ? 0 : -1;
}
ICaptureGraphBuilder2* DeviceInfoDS::GetCpatureGraphBuilder2Ptr()
{

	return m_pCaptureBuild2;
}
IGraphBuilder * DeviceInfoDS::GetGraphBuilderPtr()
{

	return m_graphBuilder;
}
WebRtc_Word32 DeviceInfoDS::GetDeviceInfo(
                                       WebRtc_UWord32 deviceNumber,
                                       char* deviceNameUTF8,
                                       WebRtc_UWord32 deviceNameLength,
                                       char* deviceUniqueIdUTF8,
                                       WebRtc_UWord32 deviceUniqueIdUTF8Length,
                                       char* productUniqueIdUTF8,
                                       WebRtc_UWord32 productUniqueIdUTF8Length)

{

	EnterCriticalSection(&sec2);
    // enumerate all video capture devices
    RELEASE_AND_CLEAR(_dsMonikerDevEnum);
    HRESULT hr =
        _dsDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
                                          &_dsMonikerDevEnum, 0);
    if (hr != NOERROR)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Failed to enumerate CLSID_SystemDeviceEnum, error 0x%x."
                     " No webcam exist?", hr);
		LeaveCriticalSection(&sec2);
        return 0;
    }

    _dsMonikerDevEnum->Reset();
    ULONG cFetched;
    IMoniker *pM;
    int index = 0;
    while (S_OK == _dsMonikerDevEnum->Next(1, &pM, &cFetched))
    {
        IPropertyBag *pBag;
        hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **) &pBag);
        if (S_OK == hr)
        {
            // Find the description or friendly name.
            VARIANT varName;
            VariantInit(&varName);
            hr = pBag->Read(L"Description", &varName, 0);
            if (FAILED(hr))
            {
                hr = pBag->Read(L"FriendlyName", &varName, 0);
            }
            if (SUCCEEDED(hr))
            {
                // ignore all VFW drivers
                if ((wcsstr(varName.bstrVal, (L"(VFW)")) == NULL) &&
                    (_wcsnicmp(varName.bstrVal, (L"Google Camera Adapter"),21)
                        != 0))
                {
                    // Found a valid device.
                    if (index == static_cast<int>(deviceNumber))
                    {
                        int convResult = 0;
                        if (deviceNameLength > 0)
                        {
                            convResult = WideCharToMultiByte(CP_UTF8, 0,
                                                             varName.bstrVal, -1,
                                                             (char*) deviceNameUTF8,
                                                             deviceNameLength, NULL,
                                                             NULL);
                            if (convResult == 0)
                            {
                                WEBRTC_TRACE(webrtc::kTraceError,
                                             webrtc::kTraceVideoCapture, _id,
                                             "Failed to convert device name to UTF8. %d",
                                             GetLastError());
								LeaveCriticalSection(&sec2);
                                return -1;
                            }
                        }
                        if (deviceUniqueIdUTF8Length > 0)
                        {
                            hr = pBag->Read(L"DevicePath", &varName, 0);
                            if (FAILED(hr))
                            {
                                strncpy_s((char *) deviceUniqueIdUTF8,
                                          deviceUniqueIdUTF8Length,
                                          (char *) deviceNameUTF8, convResult);
                                WEBRTC_TRACE(webrtc::kTraceError,
                                             webrtc::kTraceVideoCapture, _id,
                                             "Failed to get deviceUniqueIdUTF8 using deviceNameUTF8");
                            }
                            else
                            {
                                convResult = WideCharToMultiByte(
                                                          CP_UTF8,
                                                          0,
                                                          varName.bstrVal,
                                                          -1,
                                                          (char*) deviceUniqueIdUTF8,
                                                          deviceUniqueIdUTF8Length,
                                                          NULL, NULL);
                                if (convResult == 0)
                                {
                                    WEBRTC_TRACE(webrtc::kTraceError,
                                                 webrtc::kTraceVideoCapture, _id,
                                                 "Failed to convert device name to UTF8. %d",
                                                 GetLastError());
									LeaveCriticalSection(&sec2);
                                    return -1;
                                }
                                if (productUniqueIdUTF8
                                    && productUniqueIdUTF8Length > 0)
                                {
                                    GetProductId(deviceUniqueIdUTF8,
                                                 productUniqueIdUTF8,
                                                 productUniqueIdUTF8Length);
                                }
                            }
                        }

                    }
                    ++index; // increase the number of valid devices
                }
            }
            VariantClear(&varName);
            pBag->Release();
            pM->Release();
        }

    }
    if (deviceNameLength)
    {
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, _id, "%s %s",
                     __FUNCTION__, deviceNameUTF8);
    }
	LeaveCriticalSection(&sec2);
    return index;
}

IBaseFilter * DeviceInfoDS::GetDeviceFilter(
                                     const char* deviceUniqueIdUTF8,
                                     char* productUniqueIdUTF8,
                                     WebRtc_UWord32 productUniqueIdUTF8Length)
{

    const WebRtc_Word32 deviceUniqueIdUTF8Length =
        (WebRtc_Word32) strlen((char*) deviceUniqueIdUTF8); // UTF8 is also NULL terminated
    if (deviceUniqueIdUTF8Length > kVideoCaptureUniqueNameLength)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Device name too long");
        return NULL;
    }

    // enumerate all video capture devices
    RELEASE_AND_CLEAR(_dsMonikerDevEnum);
    HRESULT hr = _dsDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
                                                   &_dsMonikerDevEnum, 0);
    if (hr != NOERROR)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Failed to enumerate CLSID_SystemDeviceEnum, error 0x%x."
                     " No webcam exist?", hr);
        return 0;
    }
    _dsMonikerDevEnum->Reset();
    ULONG cFetched;
    IMoniker *pM;

    IBaseFilter *captureFilter = NULL;
    bool deviceFound = false;
    while (S_OK == _dsMonikerDevEnum->Next(1, &pM, &cFetched) && !deviceFound)
    {
        IPropertyBag *pBag;
        hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **) &pBag);
        if (S_OK == hr)
        {
            // Find the description or friendly name.
            VARIANT varName;
            VariantInit(&varName);
            if (deviceUniqueIdUTF8Length > 0)
            {
                hr = pBag->Read(L"DevicePath", &varName, 0);
                if (FAILED(hr))
                {
                    hr = pBag->Read(L"Description", &varName, 0);
                    if (FAILED(hr))
                    {
                        hr = pBag->Read(L"FriendlyName", &varName, 0);
                    }
                }
                if (SUCCEEDED(hr))
                {
                    char tempDevicePathUTF8[256];
					memset(tempDevicePathUTF8,0,256);
                    tempDevicePathUTF8[0] = 0;
                    WideCharToMultiByte(CP_UTF8, 0, varName.bstrVal, -1,
                                        tempDevicePathUTF8,
                                        sizeof(tempDevicePathUTF8), NULL,
                                        NULL);
                    if (strncmp(tempDevicePathUTF8,
                                (const char*) deviceUniqueIdUTF8,
                                deviceUniqueIdUTF8Length) == 0)
                    {
                        // We have found the requested device
                        deviceFound = true;
                        hr = pM->BindToObject(0, 0, IID_IBaseFilter,
                                              (void**) &captureFilter);
                        if FAILED(hr)
                        {
                            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture,
                                         _id, "Failed to bind to the selected capture device %d",hr);
                        }

                        if (productUniqueIdUTF8
                            && productUniqueIdUTF8Length > 0) // Get the device name
                        {

                            GetProductId(deviceUniqueIdUTF8,
                                         productUniqueIdUTF8,
                                         productUniqueIdUTF8Length);
                        }

                    }
                }
            }
            VariantClear(&varName);
            pBag->Release();
            pM->Release();
        }
    }
    return captureFilter;
}

WebRtc_Word32 DeviceInfoDS::GetWindowsCapability(
                              const WebRtc_Word32 capabilityIndex,
                              VideoCaptureCapabilityWindows& windowsCapability)

{
    ReadLockScoped cs(_apiLock);
    // Make sure the number is valid
    if (capabilityIndex >= _captureCapabilities.Size() || capabilityIndex < 0)
        return -1;

    MapItem* item = _captureCapabilities.Find(capabilityIndex);
    if (!item)
        return -1;

    VideoCaptureCapabilityWindows* capPointer =
                static_cast<VideoCaptureCapabilityWindows*> (item->GetItem());
    windowsCapability = *capPointer;
    return 0;
}
#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }

void DeviceInfoDS::UnconvertMediaType(AM_MEDIA_TYPE *pmt, RawVideoType &type)
{
	char * subname =GuidNames[pmt->subtype];

	if(strcmp(subname,"MEDIASUBTYPE_I420")==0)
		type = kVideoI420;
	else if(strcmp(subname,"MEDIASUBTYPE_IYUV")==0)
		type= kVideoIYUV;
	else if(strcmp(subname,"MEDIASUBTYPE_RGB24")==0)
		type =kVideoRGB24;
	else if(strcmp(subname,"MEDIASUBTYPE_ARGB4444")==0)
		type = kVideoARGB4444;
	else if(strcmp(subname,"MEDIASUBTYPE_RGB565")==0)
		type = kVideoRGB565;
	else if(strcmp(subname,"MEDIASUBTYPE_ARGB1555")==0)
		type = kVideoARGB1555;
	else if(strcmp(subname,"MEDIASUBTYPE_YUY2")==0)
		type = kVideoYUY2;
	else if(strcmp(subname,"MEDIASUBTYPE_YV12")==0)
		type = kVideoYV12;
	else if(strcmp(subname,"MEDIASUBTYPE_UYVY")==0)
		type= kVideoUYVY;
	else if(strcmp(subname,"MEDIASUBTYPE_NV12")==0)
		type = kVideoNV12;
	else if(strcmp(subname,"MEDIASUBTYPE_MJPG")==0)
		type = kVideoMJPEG;
	else
		type =kVideoYUY2;
	 
	 
#if  0
		switch (pmt->subtype)
		{
	case MEDIASUBTYPE_I420:
		  type =kVideoI420;
		  break;
	case MEDIASUBTYPE_IYUV:
		 type= kVideoIYUV;
		break;
	case kVideoRGB24:
		type =MEDIASUBTYPE_RGB24;
		break;;
	/*case kVideoARGB:
		return pmt->subtype =MEDIASUBTYPE_ARGB32;;*/
	case MEDIASUBTYPE_ARGB4444:
		 type = kVideoARGB4444;
		 break;
	case MEDIASUBTYPE_RGB565:
		 type = kVideoRGB565;
		 break;
	case MEDIASUBTYPE_ARGB1555:
		 type = kVideoARGB1555;
		 break;
	case MEDIASUBTYPE_YUY2:
		 type = kVideoYUY2;
		 break;
	case MEDIASUBTYPE_YV12:
		 type = kVideoYV12;
		 break;
	case MEDIASUBTYPE_UYVY:
		  type= kVideoUYVY;
		  break;
	/*case kVideoNV21:
		return pmt->subtype =MEDIASUBTYPE_NV21;;*/
	case MEDIASUBTYPE_NV12:
		  type = kVideoNV12;
		  break;
	/*case kVideoBGRA:
		return pmt->subtype =MEDIASUBTYPE_B;;*/
	case MEDIASUBTYPE_MJPG:
		  type = kVideoMJPEG;
		  break;
	default:
		  type =kVideoYUY2;
	}
#endif
}
WebRtc_Word32 DeviceInfoDS::CreateCapabilityMap( const char* deviceUniqueIdUTF8)

{
	IAMStreamConfig* iconfig = NULL; 

	HRESULT hr = S_OK;
	 IBaseFilter* pCapFilter = GetDeviceFilter(deviceUniqueIdUTF8, NULL, 0);
	hr = m_pCaptureBuild2->FindInterface( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, pCapFilter,IID_IAMStreamConfig,(void**)&iconfig); 
	if(hr!=NOERROR)
	{ 
		hr=m_pCaptureBuild2->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,pCapFilter,IID_IAMStreamConfig,(void**)&iconfig); 
	} 

	if(FAILED(hr)) 
		return FALSE; 

	int nCount = 0;
	int nSize = 0;
	hr = iconfig->GetNumberOfCapabilities(&nCount, &nSize);

	// 判断是否为视频信息
	if (sizeof(VIDEO_STREAM_CONFIG_CAPS) == nSize)
	{

		//清空_captureCapabilities
		MapItem* item = NULL;
		while (item = _captureCapabilities.Last())
		{
			VideoCaptureCapabilityWindows* cap =static_cast<VideoCaptureCapabilityWindows*> (item->GetItem());
			delete cap;
			_captureCapabilities.Erase(item);
		}


		for (int i=0; i<nCount; i++)
		{
			VIDEO_STREAM_CONFIG_CAPS scc;
			AM_MEDIA_TYPE* pmmt;

			hr = iconfig->GetStreamCaps(i, &pmmt, (BYTE *)&scc);//得到摄像头支持的所有媒体格式
			CAMERAINFO info;

			if (pmmt->formattype == FORMAT_VideoInfo || pmmt->formattype == FORMAT_VideoInfo2)
			{

				VideoCaptureCapabilityWindows* capability = new VideoCaptureCapabilityWindows();
				VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER *)pmmt->pbFormat;
				capability->height =  pvih->bmiHeader.biHeight;
				capability->width =pvih->bmiHeader.biWidth;
				capability->expectedCaptureDelay =0;
				capability->maxFPS =10000000/pvih->AvgTimePerFrame;
				UnconvertMediaType(pmmt,capability->rawType);
				_captureCapabilities.Insert(i, capability);

			}

		}
	}
	SAFE_RELEASE(iconfig); 
	return nCount;  
#if 0
	 EnterCriticalSection(&sec);
    // Reset old capability list
    MapItem* item = NULL;
    while (item = _captureCapabilities.Last())
    {
        VideoCaptureCapabilityWindows* cap =
            static_cast<VideoCaptureCapabilityWindows*> (item->GetItem());
        delete cap;
        _captureCapabilities.Erase(item);
    }
	 
    const WebRtc_Word32 deviceUniqueIdUTF8Length =
        (WebRtc_Word32) strlen((char*) deviceUniqueIdUTF8);
    if (deviceUniqueIdUTF8Length > kVideoCaptureUniqueNameLength)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Device name too long");
	LeaveCriticalSection(&sec);
        return -1;
    }
    WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, _id,
                 "CreateCapabilityMap called for device %s", deviceUniqueIdUTF8);


    char productId[kVideoCaptureProductIdLength];
	memset(productId,0,kVideoCaptureProductIdLength);
    IBaseFilter* captureDevice = DeviceInfoDS::GetDeviceFilter(
                                               deviceUniqueIdUTF8,
                                               productId,
                                               kVideoCaptureProductIdLength);
    if (!captureDevice)
        return -1;
    IPin* outputCapturePin = GetOutputPin(captureDevice, GUID_NULL);
    if (!outputCapturePin)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Failed to get capture device output pin");
        RELEASE_AND_CLEAR(captureDevice);
		LeaveCriticalSection(&sec);
        return -1;
    }
    IAMExtDevice* extDevice = NULL;
    HRESULT hr = captureDevice->QueryInterface(IID_IAMExtDevice,
                                               (void **) &extDevice);
    if (SUCCEEDED(hr) && extDevice)
    {
        WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, _id,
                     "This is an external device");
        extDevice->Release();
    }

    IAMStreamConfig* streamConfig = NULL;
    hr = outputCapturePin->QueryInterface(IID_IAMStreamConfig,
                                          (void**) &streamConfig);
    if (FAILED(hr))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Failed to get IID_IAMStreamConfig interface from capture device");
		LeaveCriticalSection(&sec);
        return -1;
    }

    // this  gets the FPS
    IAMVideoControl* videoControlConfig = NULL;
    HRESULT hrVC = captureDevice->QueryInterface(IID_IAMVideoControl,
                                      (void**) &videoControlConfig);
    if (FAILED(hrVC))
    {
        WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCapture, _id,
                     "IID_IAMVideoControl Interface NOT SUPPORTED");
    }

    AM_MEDIA_TYPE *pmt = NULL;
    VIDEO_STREAM_CONFIG_CAPS caps;
    int count, size=0;

    hr = streamConfig->GetNumberOfCapabilities(&count, &size);
	if (size != sizeof(VIDEO_STREAM_CONFIG_CAPS)) 
	{
		LeaveCriticalSection(&sec);
		return -1;
	}

    if (FAILED(hr))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Failed to GetNumberOfCapabilities");
        RELEASE_AND_CLEAR(videoControlConfig);
        RELEASE_AND_CLEAR(streamConfig);
        RELEASE_AND_CLEAR(outputCapturePin);
        RELEASE_AND_CLEAR(captureDevice);
		LeaveCriticalSection(&sec);
        return -1;
    }



    WebRtc_Word32 index = 0; // Index in created _capabilities map
    // Check if the device support formattype == FORMAT_VideoInfo2 and FORMAT_VideoInfo.
    // Prefer FORMAT_VideoInfo since some cameras (ZureCam) has been seen having problem with MJPEG and FORMAT_VideoInfo2
    // Interlace flag is only supported in FORMAT_VideoInfo2
    bool supportFORMAT_VideoInfo2 = false;
    bool supportFORMAT_VideoInfo = false;
    bool foundInterlacedFormat = false;
    GUID preferedVideoFormat = FORMAT_VideoInfo;
    for (WebRtc_Word32 tmp = 0; tmp < count; ++tmp)
    {
        hr = streamConfig->GetStreamCaps(tmp, &pmt,
                                         reinterpret_cast<BYTE*> (&caps));
        if (!FAILED(hr))
        {
            if (pmt->majortype == MEDIATYPE_Video
                && pmt->formattype == FORMAT_VideoInfo2)
            {
                WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, _id,
                             " Device support FORMAT_VideoInfo2");
                supportFORMAT_VideoInfo2 = true;
                VIDEOINFOHEADER2* h =
                    reinterpret_cast<VIDEOINFOHEADER2*> (pmt->pbFormat);
                assert(h);
                foundInterlacedFormat |= h->dwInterlaceFlags
                                        & (AMINTERLACE_IsInterlaced
                                           | AMINTERLACE_DisplayModeBobOnly);
            }
            if (pmt->majortype == MEDIATYPE_Video
                && pmt->formattype == FORMAT_VideoInfo)
            {
                WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, _id,
                             " Device support FORMAT_VideoInfo2");
                supportFORMAT_VideoInfo = true;
            }
        }
    }
    if (supportFORMAT_VideoInfo2)
    {
        if (supportFORMAT_VideoInfo && !foundInterlacedFormat)
        {
            preferedVideoFormat = FORMAT_VideoInfo;
        }
        else
        {
            preferedVideoFormat = FORMAT_VideoInfo2;
        }
    }

    for (WebRtc_Word32 tmp = 0; tmp < count; ++tmp)
    {
        hr = streamConfig->GetStreamCaps(tmp, &pmt,
                                         reinterpret_cast<BYTE*> (&caps));
        if (FAILED(hr))
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                         "Failed to GetStreamCaps");
            RELEASE_AND_CLEAR(videoControlConfig);
            RELEASE_AND_CLEAR(streamConfig);
            RELEASE_AND_CLEAR(outputCapturePin);
            RELEASE_AND_CLEAR(captureDevice);
			LeaveCriticalSection(&sec);
            return -1;
        }

        if (pmt->majortype == MEDIATYPE_Video
            && pmt->formattype == preferedVideoFormat)
        {

            VideoCaptureCapabilityWindows* capability = new VideoCaptureCapabilityWindows();
            WebRtc_Word64 avgTimePerFrame = 0;

          /*  if (pmt->formattype == FORMAT_VideoInfo)
            {
				 
                VIDEOINFOHEADER* h =reinterpret_cast<VIDEOINFOHEADER*> (pmt->pbFormat);
                assert(h);
                capability->directShowCapabilityIndex = tmp;
                capability->width = h->bmiHeader.biWidth;
                capability->height = h->bmiHeader.biHeight;
                avgTimePerFrame = h->AvgTimePerFrame;
            }*/
            if (pmt->formattype == FORMAT_VideoInfo2||pmt->formattype == FORMAT_VideoInfo)
            {
                VIDEOINFOHEADER2* h =
                    reinterpret_cast<VIDEOINFOHEADER2*> (pmt->pbFormat);
                assert(h);
                capability->directShowCapabilityIndex = tmp;
                capability->width = h->bmiHeader.biWidth;
                capability->height = h->bmiHeader.biHeight;
                capability->interlaced = h->dwInterlaceFlags
                                        & (AMINTERLACE_IsInterlaced
                                           | AMINTERLACE_DisplayModeBobOnly);
                avgTimePerFrame = h->AvgTimePerFrame;

				char mainType[30];
				char subType[30];
			
            }
/*
            if (hrVC == S_OK)
            {
                LONGLONG *maxFps; // array
                long listSize;
                SIZE size;
                size.cx = capability->width;
                size.cy = capability->height;

                // GetMaxAvailableFrameRate doesn't return max frame rate always
                // eg: Logitech Notebook. This may be due to a bug in that API
                // because GetFrameRateList array is reversed in the above camera. So
                // a util method written. Can't assume the first value will return
                // the max fps.
                hrVC = videoControlConfig->GetFrameRateList(outputCapturePin,
                                                            tmp, size,
                                                            &listSize,
                                                            &maxFps);

                if (hrVC == S_OK && listSize > 0)
                {
                    LONGLONG maxFPS = GetMaxOfFrameArray(maxFps, listSize);
                    capability->maxFPS = static_cast<int> (10000000
                                                           / maxFPS);
                    capability->supportFrameRateControl = true;
                }
                else // use existing method
                {
                    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture,
                                 _id,
                                 "GetMaxAvailableFrameRate NOT SUPPORTED");
                    if (avgTimePerFrame > 0)
                        capability->maxFPS = static_cast<int> (10000000
                                                               / avgTimePerFrame);
                    else
                        capability->maxFPS = 0;
                }
            }
            else // use existing method in case IAMVideoControl is not supported
            {
                if (avgTimePerFrame > 0)
                    capability->maxFPS = static_cast<int> (10000000
                                                           / avgTimePerFrame);
                else
                    capability->maxFPS = 0;
            }
*/

			if (avgTimePerFrame > 0)
				capability->maxFPS = static_cast<int> (10000000
				/ avgTimePerFrame);
			else
				capability->maxFPS = 0;
            // can't switch MEDIATYPE :~(
            if (pmt->subtype == MEDIASUBTYPE_I420)
            {
                capability->rawType = kVideoI420;
            }
            else if (pmt->subtype == MEDIASUBTYPE_IYUV)
            {
                capability->rawType = kVideoIYUV;
            }
            else if (pmt->subtype == MEDIASUBTYPE_RGB24)
            {
                capability->rawType = kVideoRGB24;
            }
            else if (pmt->subtype == MEDIASUBTYPE_YUY2)
            {
                capability->rawType = kVideoYUY2;
            }
            else if (pmt->subtype == MEDIASUBTYPE_RGB565)
            {
                capability->rawType = kVideoRGB565;
            }
            else if (pmt->subtype == MEDIASUBTYPE_MJPG)
            {
                capability->rawType = kVideoMJPEG;
            }
            else if (pmt->subtype == MEDIASUBTYPE_dvsl
                    || pmt->subtype == MEDIASUBTYPE_dvsd
                    || pmt->subtype == MEDIASUBTYPE_dvhd) // If this is an external DV camera
            {
                capability->rawType = kVideoYUY2;// MS DV filter seems to create this type
            }
            else if (pmt->subtype == MEDIASUBTYPE_UYVY) // Seen used by Declink capture cards
            {
                capability->rawType = kVideoUYVY;
            }
            else if (pmt->subtype == MEDIASUBTYPE_HDYC) // Seen used by Declink capture cards. Uses BT. 709 color. Not entiry correct to use UYVY. http://en.wikipedia.org/wiki/YCbCr
            {
                WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCapture, _id,
                             "Device support HDYC.");
                capability->rawType = kVideoUYVY;
            }
            else
            {
                WCHAR strGuid[39];
                StringFromGUID2(pmt->subtype, strGuid, 39);
                WEBRTC_TRACE( webrtc::kTraceWarning,
                             webrtc::kTraceVideoCapture, _id,
                             "Device support unknown media type %ls, width %d, height %d",
                             strGuid);
                delete capability;
                continue;
            }

            // Get the expected capture delay from the static list
            capability->expectedCaptureDelay
                            = GetExpectedCaptureDelay(WindowsCaptureDelays,
                                                      NoWindowsCaptureDelays,
                                                      productId,
                                                      capability->width,
                                                      capability->height);
            _captureCapabilities.Insert(index++, capability);
            WEBRTC_TRACE( webrtc::kTraceInfo, webrtc::kTraceVideoCapture, _id,
                         "Camera capability, width:%d height:%d type:%d fps:%d",
                         capability->width, capability->height,
                         capability->rawType, capability->maxFPS);
        }
        DeleteMediaType(pmt);
        pmt = NULL;
    }
    RELEASE_AND_CLEAR(streamConfig);
    RELEASE_AND_CLEAR(videoControlConfig);
    RELEASE_AND_CLEAR(outputCapturePin);
    RELEASE_AND_CLEAR(captureDevice); // Release the capture device

    // Store the new used device name
    _lastUsedDeviceNameLength = deviceUniqueIdUTF8Length;
    _lastUsedDeviceName = (char*) realloc(_lastUsedDeviceName,
                                                   _lastUsedDeviceNameLength
                                                       + 1);
    memcpy(_lastUsedDeviceName, deviceUniqueIdUTF8, _lastUsedDeviceNameLength+ 1);
    WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, _id,
                 "CreateCapabilityMap %d", _captureCapabilities.Size());

    DWORD ret = _captureCapabilities.Size();
	LeaveCriticalSection(&sec);
	return ret;

#endif 
	return 0;

}

/* Constructs a product ID from the Windows DevicePath. on a USB device the devicePath contains product id and vendor id.
 This seems to work for firewire as well
 /* Example of device path
 "\\?\usb#vid_0408&pid_2010&mi_00#7&258e7aaf&0&0000#{65e8773d-8f56-11d0-a3b9-00a0c9223196}\global"
 "\\?\avc#sony&dv-vcr&camcorder&dv#65b2d50301460008#{65e8773d-8f56-11d0-a3b9-00a0c9223196}\global"
 */
void DeviceInfoDS::GetProductId(const char* devicePath,
                                      char* productUniqueIdUTF8,
                                      WebRtc_UWord32 productUniqueIdUTF8Length)
{
    *productUniqueIdUTF8 = '\0';
    char* startPos = strstr((char*) devicePath, "\\\\?\\");
    if (!startPos)
    {
        strncpy_s((char*) productUniqueIdUTF8, productUniqueIdUTF8Length, "", 1);
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                     "Failed to get the product Id");
        return;
    }
    startPos += 4;

    char* pos = strchr(startPos, '&');
    if (!pos || pos >= (char*) devicePath + strlen((char*) devicePath))
    {
        strncpy_s((char*) productUniqueIdUTF8, productUniqueIdUTF8Length, "", 1);
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                     "Failed to get the product Id");
        return;
    }
    // Find the second occurrence.
    pos = strchr(pos + 1, '&');
    WebRtc_UWord32 bytesToCopy = (WebRtc_UWord32)(pos - startPos);
    if (pos && (bytesToCopy <= productUniqueIdUTF8Length) && bytesToCopy
        <= kVideoCaptureProductIdLength)
    {
        strncpy_s((char*) productUniqueIdUTF8, productUniqueIdUTF8Length,
                  (char*) startPos, bytesToCopy);
    }
    else
    {
        strncpy_s((char*) productUniqueIdUTF8, productUniqueIdUTF8Length, "", 1);
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                     "Failed to get the product Id");
    }
}

WebRtc_Word32 DeviceInfoDS::DisplayCaptureSettingsDialogBox(
                                         const char* deviceUniqueIdUTF8,
                                         const char* dialogTitleUTF8,
                                         void* parentWindow,
                                         WebRtc_UWord32 positionX,
                                         WebRtc_UWord32 positionY)
{
    ReadLockScoped cs(_apiLock);
    HWND window = (HWND) parentWindow;

    IBaseFilter* filter = GetDeviceFilter(deviceUniqueIdUTF8, NULL, 0);
    if (!filter)
        return -1;

    ISpecifyPropertyPages* pPages = NULL;
    CAUUID uuid;
    HRESULT hr = S_OK;

    hr = filter->QueryInterface(IID_ISpecifyPropertyPages, (LPVOID*) &pPages);
    if (!SUCCEEDED(hr))
    {
        filter->Release();
        return -1;
    }
    hr = pPages->GetPages(&uuid);
    if (!SUCCEEDED(hr))
    {
        filter->Release();
        return -1;
    }

    WCHAR tempDialogTitleWide[256];
    tempDialogTitleWide[0] = 0;
    int size = 255;

    // UTF-8 to wide char
    MultiByteToWideChar(CP_UTF8, 0, (char*) dialogTitleUTF8, -1,
                        tempDialogTitleWide, size);

    // Invoke a dialog box to display.

    hr = OleCreatePropertyFrame(window, // You must create the parent window.
                                positionX, // Horizontal position for the dialog box.
                                positionY, // Vertical position for the dialog box.
                                tempDialogTitleWide,// String used for the dialog box caption.
                                1, // Number of pointers passed in pPlugin.
                                (LPUNKNOWN*) &filter, // Pointer to the filter.
                                uuid.cElems, // Number of property pages.
                                uuid.pElems, // Array of property page CLSIDs.
                                LOCALE_USER_DEFAULT, // Locale ID for the dialog box.
                                0, NULL); // Reserved
    // Release memory.
    if (uuid.pElems)
    {
        CoTaskMemFree(uuid.pElems);
    }
    filter->Release();
    return 0;
}
} // namespace videocapturemodule
} // namespace webrtc
