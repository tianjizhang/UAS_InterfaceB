
#pragma once

#include "glog/logging.h"
#include "eXosip2/eXosip.h"
#include "jthread.h"
#include "jmutex.h"

using namespace jthread;

class UAS_InterfaceB : public JThread
{
public:
	UAS_InterfaceB(int port);
	~UAS_InterfaceB();

	int sip_init();


private:
	void *Thread();

private:
	eXosip_event_t *je;
	osip_message_t *ack ;
	osip_message_t *invite ;
	osip_message_t *response ;
	sdp_message_t *remote_sdp ;

	struct eXosip_t *excontext;
	int port;
};

