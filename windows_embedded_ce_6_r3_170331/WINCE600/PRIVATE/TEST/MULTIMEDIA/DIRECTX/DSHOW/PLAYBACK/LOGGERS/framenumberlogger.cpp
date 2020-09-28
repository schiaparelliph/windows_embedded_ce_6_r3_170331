//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft shared
// source or premium shared source license agreement under which you licensed
// this source code. If you did not accept the terms of the license agreement,
// you are not authorized to use this source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the SOURCE.RTF on your install media or the root of your tools installation.
// THE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
////////////////////////////////////////////////////////////////////////////////
//
//  Frame number logger 
//
//
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>

#include <windows.h>
#include <winuser.h>
#include <wingdi.h>
#include <stdio.h>
#include <tchar.h>
#include <streams.h>

#include "globals.h"
#include "logging.h"
#include "EventLogger.h"
#include "GraphEvent.h"
#include "FilterDesc.h"
#include "TestGraph.h"
#include "utility.h"

HRESULT CreateFrameNumberLogger(EventLoggerType type, void* pEventLoggerData, TestGraph* pTestGraph, IEventLogger** ppEventLogger)
{
	HRESULT hr = S_OK;
	IEventLogger* pEventLogger = NULL;

	switch(type)
	{
	case LogFrameNumbers:
		pEventLogger = new FrameNumberLogger();
		if (!pEventLogger)
			hr = E_OUTOFMEMORY;
		break;
	
	default:
		hr = E_NOTIMPL;
		break;
	};
	
	if (SUCCEEDED(hr))
	{
		hr = pEventLogger->Init(pTestGraph, type, pEventLoggerData);
		if (FAILED(hr))
			delete pEventLogger;
		else 
			*ppEventLogger = pEventLogger;
	}
	
	return hr;
}


// Decoder output timestamp verifier
FrameNumberLogger::FrameNumberLogger() : 
	m_EventLoggerType(EventLoggerEndMarker),
	m_nSamples(0),
	m_hr(S_OK)

{
	memset(m_szLogFile, 0, sizeof(m_szLogFile));
}

FrameNumberLogger::~FrameNumberLogger()
{
}

HRESULT FrameNumberLogger::Init(TestGraph* pTestGraph, EventLoggerType type, void* pEventLoggerData)
{
	ITapFilter* pTapFilter = NULL;
	HRESULT hr = S_OK;

	if (type != LogFrameNumbers)
		return E_NOTIMPL;

	FrameNumberLoggerData *pLoggerData = (FrameNumberLoggerData*) pEventLoggerData;

	hr = pTestGraph->InsertTapFilter(pLoggerData->filterType, pLoggerData->pindir, pLoggerData->mediaType, &pTapFilter);
	if (FAILED_F(hr))
	{
		LOG(TEXT("Failed to insert tap filter\n"));
		return hr;
	}

	// Save the media type
	pTapFilter->GetMediaType(&m_mt, PINDIR_INPUT);
	LOG(TEXT("Tapped into types: \n"));
	PrintMediaType(&m_mt);
	// Register a callback
	hr = pTapFilter->RegisterEventCallback(GenericLoggerCallback, pEventLoggerData, (void*)this);
	if (FAILED_F(hr))
	{
		LOG(TEXT("Failed to register callback\n"));
		return hr;
	}

	// Store the verification requested
	m_EventLoggerType = type;

	// Init the actual recognizer
	hr = m_Recognizer.Init(&m_mt);

	return hr;
}

HRESULT FrameNumberLogger::ProcessEvent(GraphEvent event, void* pEventData, void* pCallBackData)
{
	HRESULT hr = S_OK;

	if (FAILED_F(m_hr))
		return m_hr;

	if (m_EventLoggerType == LogFrameNumbers)
	{
		if (event == SAMPLE)
		{
			GraphSample* pSample = (GraphSample*)pEventData;
			IMediaSample* pMediaSample = pSample->pMediaSample;

			DWORD framenum = 0;
			hr = m_Recognizer.Recognize(pMediaSample, &framenum);
			
			LOG(TEXT("\nFrameNumber: %d Recognized: %d \n"), m_nSamples, framenum);
			if (m_nSamples != framenum)
				LOG(TEXT("ERROR: frame no mismatch. \n"));

			m_nSamples++;
		}
	}

	return hr;
}

HRESULT FrameNumberLogger::Start()
{
	return S_OK;
}

void FrameNumberLogger::Stop()
{
}

void FrameNumberLogger::Reset()
{
}