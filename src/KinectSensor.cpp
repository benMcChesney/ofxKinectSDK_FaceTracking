//------------------------------------------------------------------------------
// <copyright file="KinectSensor.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "StdAfx.h"
#include "KinectSensor.h"
#include <math.h>


KinectSensor::KinectSensor()
{
    m_hNextDepthFrameEvent = NULL;
    m_hNextVideoFrameEvent = NULL;
    m_hNextSkeletonEvent = NULL;
    m_pDepthStreamHandle = NULL;
    m_pVideoStreamHandle = NULL;
    m_hThNuiProcess=NULL;
    m_hEvNuiProcessStop=NULL;
    m_bNuiInitialized = false;
    m_FramesTotal = 0;
    m_SkeletonTotal = 0;
    m_VideoBuffer = NULL;
    m_DepthBuffer = NULL;
    m_ZoomFactor = 1.0f;
    m_ViewOffset.x = 0;
    m_ViewOffset.y = 0;
}

KinectSensor::~KinectSensor()
{
    Release();
}

HRESULT KinectSensor::GetVideoConfiguration(FT_CAMERA_CONFIG* videoConfig)
{
    if (!videoConfig)
    {
        return E_POINTER;
    }

    UINT width = m_VideoBuffer ? m_VideoBuffer->GetWidth() : 0;
    UINT height =  m_VideoBuffer ? m_VideoBuffer->GetHeight() : 0;
    FLOAT focalLength = 0.f;

    if(width == 640 && height == 480)
    {
        focalLength = NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS;
    }
    else if(width == 1280 && height == 960)
    {
        focalLength = NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS * 2.f;
    }

    if(focalLength == 0.f)
    {
        return E_UNEXPECTED;
    }


    videoConfig->FocalLength = focalLength;
    videoConfig->Width = width;
    videoConfig->Height = height;
    return(S_OK);
}

HRESULT KinectSensor::GetDepthConfiguration(FT_CAMERA_CONFIG* depthConfig)
{
    if (!depthConfig)
    {
        return E_POINTER;
    }

    UINT width = m_DepthBuffer ? m_DepthBuffer->GetWidth() : 0;
    UINT height =  m_DepthBuffer ? m_DepthBuffer->GetHeight() : 0;
    FLOAT focalLength = 0.f;

    if(width == 80 && height == 60)
    {
        focalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS / 4.f;
    }
    else if(width == 320 && height == 240)
    {
        focalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS;
    }
    else if(width == 640 && height == 480)
    {
        focalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS * 2.f;
    }
        
    if(focalLength == 0.f)
    {
        return E_UNEXPECTED;
    }

    depthConfig->FocalLength = focalLength;
    depthConfig->Width = width;
    depthConfig->Height = height;

    return S_OK;
}

HRESULT KinectSensor::Init(NUI_IMAGE_TYPE depthType, NUI_IMAGE_RESOLUTION depthRes, BOOL bNearMode, BOOL bFallbackToDefault, NUI_IMAGE_TYPE colorType, NUI_IMAGE_RESOLUTION colorRes, BOOL bSeatedSkeletonMode)
{
    HRESULT hr = E_UNEXPECTED;

    Release(); // Deal with double initializations.

    //do not support NUI_IMAGE_TYPE_COLOR_RAW_YUV for now
    if(colorType != NUI_IMAGE_TYPE_COLOR && colorType != NUI_IMAGE_TYPE_COLOR_YUV
        || depthType != NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX && depthType != NUI_IMAGE_TYPE_DEPTH)
    {
        return E_INVALIDARG;
    }

    m_VideoBuffer = FTCreateImage();
    if (!m_VideoBuffer)
    {
        return E_OUTOFMEMORY;
    }

    DWORD width = 0;
    DWORD height = 0;

    NuiImageResolutionToSize(colorRes, width, height);

    hr = m_VideoBuffer->Allocate(width, height, FTIMAGEFORMAT_UINT8_B8G8R8X8);
    if (FAILED(hr))
    {
        return hr;
    }

    m_DepthBuffer = FTCreateImage();
    if (!m_DepthBuffer)
    {
        return E_OUTOFMEMORY;
    }

    NuiImageResolutionToSize(depthRes, width, height);

    hr = m_DepthBuffer->Allocate(width, height, FTIMAGEFORMAT_UINT16_D13P3);
    if (FAILED(hr))
    {
        return hr;
    }
    
    m_FramesTotal = 0;
    m_SkeletonTotal = 0;

    for (int i = 0; i < NUI_SKELETON_COUNT; ++i)
    {
        m_HeadPoint[i] = m_NeckPoint[i] = FT_VECTOR3D(0, 0, 0);
        m_SkeletonTracked[i] = false;
    }

    m_hNextDepthFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hNextVideoFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hNextSkeletonEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    DWORD dwNuiInitDepthFlag = (depthType == NUI_IMAGE_TYPE_DEPTH)? NUI_INITIALIZE_FLAG_USES_DEPTH : NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX;

    hr = NuiInitialize(dwNuiInitDepthFlag | NUI_INITIALIZE_FLAG_USES_SKELETON | NUI_INITIALIZE_FLAG_USES_COLOR);
    if (FAILED(hr))
    {
        return hr;
    }
    m_bNuiInitialized = true;

	DWORD dwSkeletonFlags = NUI_SKELETON_TRACKING_FLAG_ENABLE_IN_NEAR_RANGE;
	if (bSeatedSkeletonMode)
	{
		dwSkeletonFlags |= NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT;
	}
    hr = NuiSkeletonTrackingEnable( m_hNextSkeletonEvent, dwSkeletonFlags );
    if (FAILED(hr))
    {
        return hr;
    }

    hr = NuiImageStreamOpen(
        colorType,
        colorRes,
        0,
        2,
        m_hNextVideoFrameEvent,
        &m_pVideoStreamHandle );
    if (FAILED(hr))
    {
        return hr;
    }

    hr = NuiImageStreamOpen(
        depthType,
        depthRes,
        (bNearMode)? NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE : 0,
        2,
        m_hNextDepthFrameEvent,
        &m_pDepthStreamHandle );
    if (FAILED(hr))
    {
        if(bNearMode && bFallbackToDefault)
        {
            hr = NuiImageStreamOpen(
                depthType,
                depthRes,
                0,
                2,
                m_hNextDepthFrameEvent,
                &m_pDepthStreamHandle );
        }

        if(FAILED(hr))
        {
            return hr;
        }
    }

    // Start the Nui processing thread
    m_hEvNuiProcessStop=CreateEvent(NULL,TRUE,FALSE,NULL);
    m_hThNuiProcess=CreateThread(NULL,0,ProcessThread,this,0,NULL);

    return hr;
}

void KinectSensor::Release()
{
    // Stop the Nui processing thread
    if(m_hEvNuiProcessStop!=NULL)
    {
        // Signal the thread
        SetEvent(m_hEvNuiProcessStop);

        // Wait for thread to stop
        if(m_hThNuiProcess!=NULL)
        {
            WaitForSingleObject(m_hThNuiProcess,INFINITE);
            CloseHandle(m_hThNuiProcess);
            m_hThNuiProcess = NULL;
        }
        CloseHandle(m_hEvNuiProcessStop);
        m_hEvNuiProcessStop = NULL;
    }

    if (m_bNuiInitialized)
    {
        NuiShutdown();
    }
    m_bNuiInitialized = false;

    if (m_hNextSkeletonEvent && m_hNextSkeletonEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hNextSkeletonEvent);
        m_hNextSkeletonEvent = NULL;
    }
    if (m_hNextDepthFrameEvent && m_hNextDepthFrameEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hNextDepthFrameEvent);
        m_hNextDepthFrameEvent = NULL;
    }
    if (m_hNextVideoFrameEvent && m_hNextVideoFrameEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hNextVideoFrameEvent);
        m_hNextVideoFrameEvent = NULL;
    }
    if (m_VideoBuffer)
    {
        m_VideoBuffer->Release();
        m_VideoBuffer = NULL;
    }
    if (m_DepthBuffer)
    {
        m_DepthBuffer->Release();
        m_DepthBuffer = NULL;
    }
}

DWORD WINAPI KinectSensor::ProcessThread(LPVOID pParam)
{
    KinectSensor*  pthis=(KinectSensor *) pParam;
    HANDLE          hEvents[4];

    // Configure events to be listened on
    hEvents[0]=pthis->m_hEvNuiProcessStop;
    hEvents[1]=pthis->m_hNextDepthFrameEvent;
    hEvents[2]=pthis->m_hNextVideoFrameEvent;
    hEvents[3]=pthis->m_hNextSkeletonEvent;

    // Main thread loop
    while (true)
    {
        // Wait for an event to be signaled
        WaitForMultipleObjects(sizeof(hEvents)/sizeof(hEvents[0]),hEvents,FALSE,100);

        // If the stop event is set, stop looping and exit
        if (WAIT_OBJECT_0 == WaitForSingleObject(pthis->m_hEvNuiProcessStop, 0))
        {
            break;
        }

        // Process signal events
        if (WAIT_OBJECT_0 == WaitForSingleObject(pthis->m_hNextDepthFrameEvent, 0))
        {
            pthis->GotDepthAlert();
            pthis->m_FramesTotal++;
        }
        if (WAIT_OBJECT_0 == WaitForSingleObject(pthis->m_hNextVideoFrameEvent, 0))
        {
            pthis->GotVideoAlert();
        }
        if (WAIT_OBJECT_0 == WaitForSingleObject(pthis->m_hNextSkeletonEvent, 0))
        {
            pthis->GotSkeletonAlert();
            pthis->m_SkeletonTotal++;
        }
    }

    return 0;
}

void KinectSensor::GotVideoAlert( )
{
    const NUI_IMAGE_FRAME* pImageFrame = NULL;

    HRESULT hr = NuiImageStreamGetNextFrame(m_pVideoStreamHandle, 0, &pImageFrame);
    if (FAILED(hr))
    {
        return;
    }

    INuiFrameTexture* pTexture = pImageFrame->pFrameTexture;
    NUI_LOCKED_RECT LockedRect;
    pTexture->LockRect(0, &LockedRect, NULL, 0);
    if (LockedRect.Pitch)
    {   // Copy video frame to face tracking
        memcpy(m_VideoBuffer->GetBuffer(), PBYTE(LockedRect.pBits), min(m_VideoBuffer->GetBufferSize(), UINT(pTexture->BufferLen())));
    }
    else
    {
        OutputDebugString(L"Buffer length of received texture is bogus\r\n");
    }

    hr = NuiImageStreamReleaseFrame(m_pVideoStreamHandle, pImageFrame);
}


void KinectSensor::GotDepthAlert( )
{
    const NUI_IMAGE_FRAME* pImageFrame = NULL;

    HRESULT hr = NuiImageStreamGetNextFrame(m_pDepthStreamHandle, 0, &pImageFrame);

    if (FAILED(hr))
    {
        return;
    }

    INuiFrameTexture* pTexture = pImageFrame->pFrameTexture;
    NUI_LOCKED_RECT LockedRect;
    pTexture->LockRect(0, &LockedRect, NULL, 0);
    if (LockedRect.Pitch)
    {   // Copy depth frame to face tracking
        memcpy(m_DepthBuffer->GetBuffer(), PBYTE(LockedRect.pBits), min(m_DepthBuffer->GetBufferSize(), UINT(pTexture->BufferLen())));
    }
    else
    {
        OutputDebugString( L"Buffer length of received depth texture is bogus\r\n" );
    }

    hr = NuiImageStreamReleaseFrame(m_pDepthStreamHandle, pImageFrame);
}

void KinectSensor::GotSkeletonAlert()
{
    NUI_SKELETON_FRAME SkeletonFrame = {0};

    HRESULT hr = NuiSkeletonGetNextFrame(0, &SkeletonFrame);
    if(FAILED(hr))
    {
        return;
    }

    for( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )
    {
        if( SkeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED &&
            NUI_SKELETON_POSITION_TRACKED == SkeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_HEAD] &&
            NUI_SKELETON_POSITION_TRACKED == SkeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_SHOULDER_CENTER])
        {
            m_SkeletonTracked[i] = true;
            m_HeadPoint[i].x = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HEAD].x;
            m_HeadPoint[i].y = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HEAD].y;
            m_HeadPoint[i].z = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HEAD].z;
            m_NeckPoint[i].x = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].x;
            m_NeckPoint[i].y = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].y;
            m_NeckPoint[i].z = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].z;
        }
        else
        {
            m_HeadPoint[i] = m_NeckPoint[i] = FT_VECTOR3D(0, 0, 0);
            m_SkeletonTracked[i] = false;
        }
    }
}

HRESULT KinectSensor::GetClosestHint(FT_VECTOR3D* pHint3D)
{
    int selectedSkeleton = -1;
    float smallestDistance = 0;

    if (!pHint3D)
    {
        return(E_POINTER);
    }

    if (pHint3D[1].x == 0 && pHint3D[1].y == 0 && pHint3D[1].z == 0)
    {
        // Get the skeleton closest to the camera
        for (int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )
        {
            if (m_SkeletonTracked[i] && (smallestDistance == 0 || m_HeadPoint[i].z < smallestDistance))
            {
                smallestDistance = m_HeadPoint[i].z;
                selectedSkeleton = i;
            }
        }
    }
    else
    {   // Get the skeleton closest to the previous position
        for (int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )
        {
            if (m_SkeletonTracked[i])
            {
                float d = abs(m_HeadPoint[i].x - pHint3D[1].x) +
                    abs(m_HeadPoint[i].y - pHint3D[1].y) +
                    abs(m_HeadPoint[i].z - pHint3D[1].z);
                if (smallestDistance == 0 || d < smallestDistance)
                {
                    smallestDistance = d;
                    selectedSkeleton = i;
                }
            }
        }
    }
    if (selectedSkeleton == -1)
    {
        return E_FAIL;
    }

    pHint3D[0] = m_NeckPoint[selectedSkeleton];
    pHint3D[1] = m_HeadPoint[selectedSkeleton];

    return S_OK;
}

