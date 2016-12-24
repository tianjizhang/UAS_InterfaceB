#pragma once
#include "rtpsession.h"
#include "rtppacket.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include "rtpsourcedata.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "glog/logging.h"
using namespace jrtplib;

class RTPSessionS :public RTPSession
{
public:

	RTPSessionS();

	virtual ~RTPSessionS();

	//
	// This function checks if there was a RTP error. If so, it displays an error
	// message and exists.
	//
	void checkerror(int rtperr)
	{
		if (rtperr < 0)
		{
			LOG(ERROR) << RTPGetErrorString(rtperr);
			exit(-1);
		}
	}

protected:
	void OnNewSource(RTPSourceData *dat);

	void OnBYEPacket(RTPSourceData *dat);

	void OnRemoveSource(RTPSourceData *dat);
};

